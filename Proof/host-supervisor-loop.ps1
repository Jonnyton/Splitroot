<#
.SYNOPSIS
Supervisor-owned host loop for fresh-build adoption at match boundaries.

.DESCRIPTION
This script is the safe boundary between "keep the visible canary alive" and
"fresh C++ needs a new Unreal process." It only refreshes a host process that
it launched and recorded in Saved/Proof/host-supervisor-session.json.

If Jonathan already has the desktop current build running, this supervisor
observes and writes pending-refresh metadata. With -AdoptExternalAtBoundary it
can take over that unmanaged host only after the live log reaches the next-match
boundary, then future refreshes are supervisor-owned.
#>
param(
    [string]$ProjectRoot = '',
    [int]$PollSeconds = 5,
    [int]$MaxIterations = 0,
    [int]$BotsPerTeam = 8,
    [string]$HostEndpoint = 'listen://localhost',
    [int]$ReconnectWindowSeconds = 30,
    [int]$LiveStaleSeconds = 960,
    [switch]$AllowBoundaryRefresh,
    [switch]$BuildAtBoundary,
    [string]$BuildScriptPath = '',
    [switch]$AdoptExternalAtBoundary,
    [switch]$RestartOnCrash,
    [switch]$ForceLaunch,
    [switch]$HarnessIgnoreExternalCurrentBuild,
    [int]$HarnessExternalProcessId = 0,
    [string]$HarnessExternalLogPath = ''
)

$ErrorActionPreference = 'Continue'
if (-not $ProjectRoot) {
    $ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
}

$drive = Join-Path $ProjectRoot 'Proof\playtest-drive.ps1'
$pendingScript = Join-Path $ProjectRoot 'Proof\host-refresh-pending-state.ps1'
if (-not $BuildScriptPath) {
    $BuildScriptPath = Join-Path $ProjectRoot 'Proof\build-locked.ps1'
}
$proofDir = Join-Path $ProjectRoot 'Saved\Proof'
$supervisorStatePath = Join-Path $proofDir 'host-supervisor-session.json'
$supervisorPidPath = Join-Path $proofDir 'host-supervisor-loop.pid.json'
$supervisorLogPath = Join-Path $proofDir 'host-supervisor-loop.log'
$refreshProofPath = Join-Path $proofDir 'last-host-supervisor-refresh.json'
$boundaryBuildLogPath = Join-Path $proofDir 'host-supervisor-boundary-build.log'
$boundaryBuildBlockedPath = Join-Path $proofDir 'host-supervisor-boundary-build-blocked.json'
$playtestStatePath = Join-Path $proofDir 'playtest-drive-session.json'
$playtestLogPath = Join-Path $proofDir 'playtest-drive.log'
$unrealLogPath = if ($HarnessExternalLogPath) { $HarnessExternalLogPath } else { Join-Path $ProjectRoot 'Saved\Logs\ArchonFactoryCanary.log' }
$moduleDllPath = Join-Path $ProjectRoot 'Binaries\Win64\UnrealEditor-ArchonFactoryCanary.dll'
$matchUrl = '/Engine/Maps/Entry?ArchonSplitrootValley?ArchonMatchLoop?game=/Script/ArchonFactoryCanary.ArchonMatchGameMode?ArchonMapId=splitroot_valley?listen'

New-Item -ItemType Directory -Force -Path $proofDir | Out-Null

function Write-SupervisorEvent([string]$Message) {
    $line = "HostSupervisorEvent: $Message"
    Write-Host $line
    $stamp = (Get-Date).ToUniversalTime().ToString('o')
    "$stamp $line" | Add-Content -LiteralPath $supervisorLogPath -Encoding UTF8
}

function Test-MaxIterationsReached([int]$Iteration) {
    return ($MaxIterations -gt 0 -and $Iteration -ge $MaxIterations)
}

function Get-JsonFile([string]$Path) {
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) { return $null }
    try {
        return Get-Content -LiteralPath $Path -Raw | ConvertFrom-Json
    } catch {
        Write-SupervisorEvent "json read failed path=$Path error=$($_.Exception.Message)"
        return $null
    }
}

function Enter-SupervisorLoopLock {
    $existing = Get-JsonFile $supervisorPidPath
    if ($existing -and $existing.Pid) {
        $existingPid = [int]$existing.Pid
        if ($existingPid -ne $PID -and (Get-Process -Id $existingPid -ErrorAction SilentlyContinue)) {
            Write-SupervisorEvent "supervisor already running pid=$existingPid; exiting duplicate"
            return $false
        }
    }

    [pscustomobject]@{
        Pid = $PID
        StartedUtc = (Get-Date).ToUniversalTime().ToString('o')
        ProjectRoot = $ProjectRoot
        AllowBoundaryRefresh = [bool]$AllowBoundaryRefresh
        BuildAtBoundary = [bool]$BuildAtBoundary
        AdoptExternalAtBoundary = [bool]$AdoptExternalAtBoundary
        LiveStaleSeconds = $LiveStaleSeconds
        BuildScriptPath = $BuildScriptPath
    } | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $supervisorPidPath -Encoding UTF8
    return $true
}

function Exit-SupervisorLoopLock {
    $existing = Get-JsonFile $supervisorPidPath
    if ($existing -and $existing.Pid -and [int]$existing.Pid -eq $PID) {
        Remove-Item -LiteralPath $supervisorPidPath -Force -ErrorAction SilentlyContinue
    }
}

function Get-ProcessAlive([int]$ProcessId) {
    if ($ProcessId -le 0) { return $false }
    return [bool](Get-Process -Id $ProcessId -ErrorAction SilentlyContinue)
}

function Get-ExistingCurrentBuildProcess {
    if ($HarnessExternalProcessId -gt 0) {
        $harnessProcess = Get-Process -Id $HarnessExternalProcessId -ErrorAction SilentlyContinue
        if ($harnessProcess) {
            return [pscustomobject]@{
                ProcessId = $HarnessExternalProcessId
                Name = 'UnrealEditor.exe'
                CommandLine = "HARNESS_EXTERNAL ArchonFactoryCanary.uproject pid=$HarnessExternalProcessId"
                HarnessExternal = $true
            }
        }
        return $null
    }

    try {
        return Get-CimInstance Win32_Process |
            Where-Object {
                $_.Name -eq 'UnrealEditor.exe' -and
                $_.CommandLine -match 'ArchonFactoryCanary\.uproject'
            } |
            Sort-Object ProcessId |
            Select-Object -First 1
    } catch {
        return $null
    }
}

function Stop-ExternalCurrentBuildProcess([int]$ProcessId) {
    $result = [ordered]@{
        StopAttempted = $true
        StopSucceeded = $false
        ProcessId = $ProcessId
        HarnessExternal = $false
        Verification = 'not_verified'
        WaitedSeconds = 0
    }

    if ($ProcessId -le 0) {
        $result.Verification = 'invalid_pid'
        return [pscustomobject]$result
    }

    if ($HarnessExternalProcessId -gt 0 -and $ProcessId -eq $HarnessExternalProcessId) {
        $result.HarnessExternal = $true
        $result.Verification = 'harness_external_pid'
    } else {
        try {
            $process = Get-CimInstance Win32_Process -Filter "ProcessId = $ProcessId" -ErrorAction Stop
        } catch {
            $process = $null
        }
        if (-not $process) {
            $result.Verification = 'missing_process'
            return [pscustomobject]$result
        }
        if ($process.Name -ne 'UnrealEditor.exe' -or $process.CommandLine -notmatch 'ArchonFactoryCanary\.uproject') {
            $result.Verification = 'not_current_build_unreal_editor'
            return [pscustomobject]$result
        }
        $result.Verification = 'targeted_current_build_pid'
    }

    Stop-Process -Id $ProcessId -Force -ErrorAction SilentlyContinue
    $deadline = (Get-Date).AddSeconds(30)
    while ((Get-Date) -lt $deadline -and (Get-Process -Id $ProcessId -ErrorAction SilentlyContinue)) {
        Start-Sleep -Seconds 1
        $result.WaitedSeconds = [int]$result.WaitedSeconds + 1
    }

    $result.StopSucceeded = -not [bool](Get-Process -Id $ProcessId -ErrorAction SilentlyContinue)
    return [pscustomobject]$result
}

function Get-PlaytestLogText {
    if (-not (Test-Path -LiteralPath $playtestLogPath -PathType Leaf)) {
        return ''
    }
    $text = Get-Content -LiteralPath $playtestLogPath -Raw
    if ($null -eq $text) { return '' }
    return $text
}

function Get-UnrealLogText {
    if (-not (Test-Path -LiteralPath $unrealLogPath -PathType Leaf)) {
        return ''
    }
    $text = Get-Content -LiteralPath $unrealLogPath -Raw
    if ($null -eq $text) { return '' }
    return $text
}

function Get-LatestFingerprint([string]$Text) {
    $matches = [regex]::Matches($Text, 'ArchonFactoryCanary: BuildFingerprint[^\r\n]+')
    if ($matches.Count -eq 0) { return '' }
    return $matches[$matches.Count - 1].Value
}

function Get-LatestNextMap([string]$Text) {
    $matches = [regex]::Matches($Text, 'ArchonFactoryCanary: NextMatchPending nextMap=(\S+)')
    if ($matches.Count -eq 0) { return 'splitroot_valley' }
    return $matches[$matches.Count - 1].Groups[1].Value
}

function Get-LogLineForIndex([string]$Text, [int]$Index) {
    if (-not $Text -or $Index -lt 0) { return '' }
    $start = $Text.LastIndexOf("`n", [Math]::Min($Index, [Math]::Max(0, $Text.Length - 1)))
    if ($start -lt 0) { $start = 0 } else { $start++ }
    $end = $Text.IndexOf("`n", $Index)
    if ($end -lt 0) { $end = $Text.Length }
    return $Text.Substring($start, $end - $start)
}

function Get-LogTimestampUtcForIndex([string]$Text, [int]$Index) {
    $line = Get-LogLineForIndex -Text $Text -Index $Index
    $match = [regex]::Match($line, '^\[(\d{4})\.(\d{2})\.(\d{2})-(\d{2})\.(\d{2})\.(\d{2}):')
    if (-not $match.Success) { return $null }
    try {
        return [datetime]::SpecifyKind(
            [datetime]::new(
                [int]$match.Groups[1].Value,
                [int]$match.Groups[2].Value,
                [int]$match.Groups[3].Value,
                [int]$match.Groups[4].Value,
                [int]$match.Groups[5].Value,
                [int]$match.Groups[6].Value
            ),
            [System.DateTimeKind]::Utc
        )
    } catch {
        return $null
    }
}

function Get-LatestLifecycleMarker([string]$Text) {
    if (-not $Text) {
        return [pscustomobject]@{ Found = $false; Kind = ''; IsBoundary = $false; TimestampUtc = $null; Index = -1 }
    }

    $latestIndex = -1
    $latestKind = ''
    $latestIsBoundary = $false
    $markers = @(
        [pscustomobject]@{ Pattern = 'ArchonFactoryCanary: BuildFingerprint\b'; Kind = 'BuildFingerprint'; IsBoundary = $false },
        [pscustomobject]@{ Pattern = 'ArchonFactoryCanary: MatchLoopActive\b'; Kind = 'MatchLoopActive'; IsBoundary = $false },
        [pscustomobject]@{ Pattern = 'ArchonFactoryCanary: MatchPhase phase=Warmup\b'; Kind = 'Warmup'; IsBoundary = $false },
        [pscustomobject]@{ Pattern = 'ArchonFactoryCanary: MatchPhase phase=Live\b'; Kind = 'Live'; IsBoundary = $false },
        [pscustomobject]@{ Pattern = 'ArchonFactoryCanary: MatchPhase phase=MatchEnd\b'; Kind = 'MatchEnd'; IsBoundary = $false },
        [pscustomobject]@{ Pattern = 'ArchonFactoryCanary: MatchPhase phase=Traveling\b'; Kind = 'Traveling'; IsBoundary = $false },
        [pscustomobject]@{ Pattern = 'ArchonFactoryCanary: NextMatchLoading\b'; Kind = 'NextMatchLoading'; IsBoundary = $false },
        [pscustomobject]@{ Pattern = 'ArchonFactoryCanary: TravelingTo\b'; Kind = 'TravelingTo'; IsBoundary = $false },
        [pscustomobject]@{ Pattern = 'ArchonFactoryCanary: NextMatchPending nextMap='; Kind = 'NextMatchPending'; IsBoundary = $true }
    )

    foreach ($marker in $markers) {
        foreach ($match in [regex]::Matches($Text, $marker.Pattern)) {
            if ($match.Index -gt $latestIndex) {
                $latestIndex = $match.Index
                $latestKind = $marker.Kind
                $latestIsBoundary = [bool]$marker.IsBoundary
            }
        }
    }

    return [pscustomobject]@{
        Found = $latestIndex -ge 0
        Kind = $latestKind
        IsBoundary = $latestIsBoundary
        TimestampUtc = Get-LogTimestampUtcForIndex -Text $Text -Index $latestIndex
        Index = $latestIndex
    }
}

function Test-AtMatchBoundary([string]$Text) {
    $marker = Get-LatestLifecycleMarker $Text
    return $marker.Found -and $marker.IsBoundary
}

function Test-LiveMatchStale([string]$Text, [int]$StaleSeconds) {
    if ($StaleSeconds -le 0) { return $false }
    $marker = Get-LatestLifecycleMarker $Text
    if (-not $marker.Found -or $marker.Kind -ne 'Live' -or $null -eq $marker.TimestampUtc) {
        return $false
    }
    $ageSeconds = ((Get-Date).ToUniversalTime() - ([datetime]$marker.TimestampUtc).ToUniversalTime()).TotalSeconds
    return $ageSeconds -ge $StaleSeconds
}

function Get-FingerprintField([string]$Fingerprint, [string]$Name) {
    if (-not $Fingerprint) { return '' }
    $pattern = [regex]::Escape($Name) + '=([^\s]+)'
    $match = [regex]::Match($Fingerprint, $pattern)
    if (-not $match.Success) { return '' }
    return $match.Groups[1].Value
}

function ConvertTo-UtcDate($Value) {
    if ($null -eq $Value) { return $null }
    $text = ([string]$Value).Trim()
    if (-not $text -or $text -eq 'missing') { return $null }
    try {
        return ([datetime]::Parse(
            $text,
            [System.Globalization.CultureInfo]::InvariantCulture,
            [System.Globalization.DateTimeStyles]::AssumeUniversal
        )).ToUniversalTime()
    } catch {
        return $null
    }
}

function Format-UtcDate($Value) {
    if ($null -eq $Value) { return '' }
    return ([datetime]$Value).ToUniversalTime().ToString('o')
}

function Get-MaxUtcDate($A, $B) {
    if ($null -eq $A) { return $B }
    if ($null -eq $B) { return $A }
    if (([datetime]$B).ToUniversalTime() -gt ([datetime]$A).ToUniversalTime()) { return $B }
    return $A
}

function Get-FileUtcString([string]$Path) {
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) { return '' }
    return (Get-Item -LiteralPath $Path).LastWriteTimeUtc.ToString('o')
}

function Test-BoundaryBuildBlockedForInputs($CompileNewestUtc, $NativeRequiredUtc, [string]$PendingModuleText) {
    $blocked = Get-JsonFile $boundaryBuildBlockedPath
    if (-not $blocked -or -not $blocked.BuildBlocked) { return $false }
    if ((($null -eq $CompileNewestUtc) -and ($null -eq $NativeRequiredUtc)) -or -not $PendingModuleText) { return $false }

    $blockedCompileUtc = ConvertTo-UtcDate $blocked.CompileNewestUtc
    $blockedNativeUtc = ConvertTo-UtcDate $blocked.NativeRequiredAfterUtc

    $compileMatches = [bool](
        $blockedCompileUtc -and $CompileNewestUtc -and
        [math]::Abs(($blockedCompileUtc.ToUniversalTime() - $CompileNewestUtc.ToUniversalTime()).TotalSeconds) -lt 1.0
    )
    $nativeMatches = [bool](
        $blockedNativeUtc -and $NativeRequiredUtc -and
        [math]::Abs(($blockedNativeUtc.ToUniversalTime() - $NativeRequiredUtc.ToUniversalTime()).TotalSeconds) -lt 1.0
    )

    return [bool]($blocked.PendingModuleDllUtc -eq $PendingModuleText -and ($compileMatches -or $nativeMatches))
}

function Write-BoundaryBuildBlocked(
    [string]$Reason,
    [string]$PendingModuleText,
    [string]$CompileNewestText,
    [string]$NativeRequiredText,
    [object]$BuildResult
) {
    [pscustomobject]@{
        BuildBlocked = $true
        BlockedUtc = (Get-Date).ToUniversalTime().ToString('o')
        Reason = $Reason
        PendingModuleDllUtc = $PendingModuleText
        CompileNewestUtc = $CompileNewestText
        NativeRequiredAfterUtc = $NativeRequiredText
        BuildExitCode = $BuildResult.BuildExitCode
        PolicyBuildExitCode = $BuildResult.PolicyBuildExitCode
        AutomationExitCode = $BuildResult.AutomationExitCode
        AutomationSucceeded = $BuildResult.AutomationSucceeded
        AutomationFailedCount = $BuildResult.AutomationFailedCount
        ModuleTimestampAdvanced = [bool]$BuildResult.ModuleTimestampAdvanced
        BuildLogPath = $BuildResult.BuildLogPath
    } | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $boundaryBuildBlockedPath -Encoding UTF8
}

function Clear-BoundaryBuildBlocked {
    Remove-Item -LiteralPath $boundaryBuildBlockedPath -Force -ErrorAction SilentlyContinue
}

function Test-ExcludedInputPath([string]$Path, [string[]]$ExcludePathFragments) {
    if (-not $Path -or -not $ExcludePathFragments) { return $false }
    $normalized = $Path.Replace('/', '\')
    foreach ($fragment in $ExcludePathFragments) {
        if (-not $fragment) { continue }
        if ($normalized.IndexOf($fragment.Replace('/', '\'), [System.StringComparison]::OrdinalIgnoreCase) -ge 0) {
            return $true
        }
    }
    return $false
}

function Get-NewestInputUtc([string[]]$RelativeRoots, [string[]]$Extensions, [string[]]$ExcludePathFragments = @()) {
    $newest = $null
    foreach ($relative in $RelativeRoots) {
        $path = Join-Path $ProjectRoot $relative
        if (-not (Test-Path -LiteralPath $path)) { continue }

        $items = @()
        $rootItem = Get-Item -LiteralPath $path -ErrorAction SilentlyContinue
        if ($rootItem -and -not $rootItem.PSIsContainer) {
            $items = @($rootItem)
        } else {
            $items = @(Get-ChildItem -LiteralPath $path -Recurse -File -ErrorAction SilentlyContinue)
        }

        foreach ($item in $items) {
            $ext = $item.Extension.ToLowerInvariant()
            if ($Extensions -notcontains $ext) { continue }
            if (Test-ExcludedInputPath -Path $item.FullName -ExcludePathFragments $ExcludePathFragments) { continue }
            if ($null -eq $newest -or $item.LastWriteTimeUtc -gt $newest) {
                $newest = $item.LastWriteTimeUtc
            }
        }
    }
    return $newest
}

function Get-BotStrategyNativeRequirementUtc {
    $path = Join-Path $ProjectRoot 'games\splitroot\FactoryContracts\bot_strategy_tuning.json'
    $tuning = Get-JsonFile $path
    if (-not $tuning -or -not $tuning.PSObject.Properties['native_requires_module_after_utc']) {
        return $null
    }
    ConvertTo-UtcDate $tuning.native_requires_module_after_utc
}

function Get-PendingAdoption([string]$Fingerprint) {
    $currentModuleText = Get-FingerprintField $Fingerprint 'moduleDllUtc'
    $processStartText = Get-FingerprintField $Fingerprint 'processStartUtc'
    $currentModuleUtc = ConvertTo-UtcDate $currentModuleText
    $processStartUtc = ConvertTo-UtcDate $processStartText
    $pendingModuleText = Get-FileUtcString $moduleDllPath
    $pendingModuleUtc = ConvertTo-UtcDate $pendingModuleText

    $compileNewestUtc = Get-NewestInputUtc `
        -RelativeRoots @('Source') `
        -Extensions @('.cpp', '.h', '.cs')
    $runtimeContractNewestUtc = Get-NewestInputUtc `
        -RelativeRoots @('Config', 'FactoryContracts', 'games\splitroot\FactoryContracts', 'ArchonFactoryCanary.uproject') `
        -Extensions @('.ini', '.json', '.uproject') `
        -ExcludePathFragments @('\FactoryContracts\thumbs\generated\')
    $runtimeContentNewestUtc = Get-NewestInputUtc `
        -RelativeRoots @('Content', 'games\splitroot\Content') `
        -Extensions @('.uasset', '.umap', '.png', '.wav', '.obj', '.glb', '.fbx')
    $runtimeNewestUtc = Get-MaxUtcDate $runtimeContractNewestUtc $runtimeContentNewestUtc
    $nativeRequiredUtc = Get-BotStrategyNativeRequirementUtc

    $sourceRequiresBuild = [bool]($compileNewestUtc -and $pendingModuleUtc -and $compileNewestUtc -gt $pendingModuleUtc.AddSeconds(1))
    $nativeRequiresBuild = [bool]($nativeRequiredUtc -and $pendingModuleUtc -and $nativeRequiredUtc -gt $pendingModuleUtc.AddSeconds(1))
    $requiresBuild = [bool]($sourceRequiresBuild -or $nativeRequiresBuild)
    $moduleNewerThanRunning = [bool]($currentModuleUtc -and $pendingModuleUtc -and $pendingModuleUtc -gt $currentModuleUtc.AddSeconds(1))
    $runtimeNewerThanProcess = [bool]($processStartUtc -and $runtimeNewestUtc -and $runtimeNewestUtc -gt $processStartUtc.AddSeconds(1))
    $boundaryBuildBlocked = $requiresBuild -and (Test-BoundaryBuildBlockedForInputs -CompileNewestUtc $compileNewestUtc -NativeRequiredUtc $nativeRequiredUtc -PendingModuleText $pendingModuleText)

    $reason = ''
    if ($boundaryBuildBlocked) {
        $reason = 'boundary_build_failed_latest_not_adopted'
    } elseif ($sourceRequiresBuild) {
        $reason = 'source_newer_than_module_boundary_build_pending'
    } elseif ($nativeRequiresBuild) {
        $reason = 'native_requirement_newer_than_module_boundary_build_pending'
    } elseif ($moduleNewerThanRunning) {
        $reason = 'newer_module_boundary_refresh_pending'
    } elseif ($runtimeNewerThanProcess) {
        $reason = 'runtime_contract_newer_than_process_boundary_refresh_pending'
    }

    return [pscustomobject]@{
        Pending = [bool]($reason -and -not $boundaryBuildBlocked)
        Reason = $reason
        RequiresBuild = $requiresBuild
        SourceRequiresBuild = $sourceRequiresBuild
        NativeRequiresBuild = $nativeRequiresBuild
        BoundaryBuildBlocked = [bool]$boundaryBuildBlocked
        CurrentModuleDllUtc = $currentModuleText
        CurrentProcessStartUtc = $processStartText
        PendingModuleDllUtc = $pendingModuleText
        CompileNewestUtc = Format-UtcDate $compileNewestUtc
        NativeRequiredAfterUtc = Format-UtcDate $nativeRequiredUtc
        RuntimeNewestUtc = Format-UtcDate $runtimeNewestUtc
    }
}

function Invoke-BoundaryBuild([string]$Reason, [bool]$RequiresBuild) {
    $beforeModuleUtc = Get-FileUtcString $moduleDllPath
    $result = [ordered]@{
        BuildAtBoundary = [bool]$BuildAtBoundary
        BuildAttempted = $false
        BuildRequired = $RequiresBuild
        BuildReason = $Reason
        BuildScriptPath = $BuildScriptPath
        BuildExitCode = $null
        PolicyBuildExitCode = $null
        AutomationExitCode = $null
        AutomationSucceeded = $null
        AutomationFailedCount = $null
        ModuleTimestampAdvanced = $false
        BuildSucceeded = $false
        ModuleDllUtcBeforeBuild = $beforeModuleUtc
        ModuleDllUtcAfterBuild = $beforeModuleUtc
        BuildLogPath = $boundaryBuildLogPath
    }

    if (-not $RequiresBuild) {
        $result.BuildSucceeded = $true
        return [pscustomobject]$result
    }
    if (-not $BuildAtBoundary) {
        Write-SupervisorEvent "boundary build required but BuildAtBoundary=false reason=$Reason"
        return [pscustomobject]$result
    }
    if (-not (Test-Path -LiteralPath $BuildScriptPath -PathType Leaf)) {
        Write-SupervisorEvent "boundary build script missing path=$BuildScriptPath"
        $result.BuildExitCode = -1
        return [pscustomobject]$result
    }

    Write-SupervisorEvent "boundary build starting reason=$Reason script=$BuildScriptPath"
    $output = powershell -ExecutionPolicy Bypass -File $BuildScriptPath -ProjectRoot $ProjectRoot 2>&1 | Out-String
    $exitCode = $LASTEXITCODE
    $output | Set-Content -LiteralPath $boundaryBuildLogPath -Encoding UTF8
    $afterModuleUtc = Get-FileUtcString $moduleDllPath
    $beforeModuleDate = ConvertTo-UtcDate $beforeModuleUtc
    $afterModuleDate = ConvertTo-UtcDate $afterModuleUtc
    $moduleAdvanced = [bool]($beforeModuleDate -and $afterModuleDate -and $afterModuleDate -gt $beforeModuleDate.AddSeconds(1))
    $summary = Get-JsonFile (Join-Path $proofDir 'last-policy-build-and-test.json')
    $policyBuildExitCode = if ($summary -and $null -ne $summary.BuildExitCode) { [int]$summary.BuildExitCode } else { $null }
    $automationExitCode = if ($summary -and $null -ne $summary.AutomationExitCode) { [int]$summary.AutomationExitCode } else { $null }
    $automationSucceeded = if ($summary -and $null -ne $summary.AutomationSucceeded) { [bool]$summary.AutomationSucceeded } else { $null }
    $automationFailedCount = if ($summary -and $null -ne $summary.AutomationFailedCount) { [int]$summary.AutomationFailedCount } else { $null }

    $result.BuildAttempted = $true
    $result.BuildExitCode = if ($null -eq $exitCode) { 0 } else { $exitCode }
    $result.PolicyBuildExitCode = $policyBuildExitCode
    $result.AutomationExitCode = $automationExitCode
    $result.AutomationSucceeded = $automationSucceeded
    $result.AutomationFailedCount = $automationFailedCount
    $result.ModuleTimestampAdvanced = $moduleAdvanced
    $result.BuildSucceeded = [bool](
        ($policyBuildExitCode -eq 0) -or
        ($null -eq $policyBuildExitCode -and $result.BuildExitCode -eq 0) -or
        $moduleAdvanced
    )
    $result.ModuleDllUtcAfterBuild = $afterModuleUtc
    if ($result.BuildSucceeded) {
        Clear-BoundaryBuildBlocked
    }
    Write-SupervisorEvent "boundary build finished exit=$($result.BuildExitCode) policyBuildExit=$policyBuildExitCode automationSucceeded=$automationSucceeded moduleBefore=$beforeModuleUtc moduleAfter=$afterModuleUtc moduleAdvanced=$moduleAdvanced"
    return [pscustomobject]$result
}

function Write-PendingState(
    [string]$Reason,
    [int]$ProcessId,
    [string]$Fingerprint,
    [string]$NextMap,
    [bool]$OwnsHost,
    [string]$Mode,
    [string]$ClientState,
    [string]$CurrentModuleDllUtc = '',
    [string]$PendingModuleDllUtc = '',
    [string]$ObservedLogPath = ''
) {
    $logPath = if ($ObservedLogPath) { $ObservedLogPath } else { $playtestLogPath }
    $args = @(
        '-ExecutionPolicy', 'Bypass',
        '-File', $pendingScript,
        '-Action', 'write',
        '-ProjectRoot', $ProjectRoot,
        '-Reason', $Reason,
        '-CurrentProcessId', $ProcessId,
        '-CurrentBuildFingerprint', $Fingerprint,
        '-NextMap', $NextMap,
        '-HostEndpoint', $HostEndpoint,
        '-ReconnectWindowSeconds', $ReconnectWindowSeconds,
        '-SupervisorOwnsHost', ([string]$OwnsHost),
        '-SupervisorMode', $Mode,
        '-ClientState', $ClientState,
        '-ObservedLogPath', $logPath
    )
    if ($CurrentModuleDllUtc) {
        $args += @('-CurrentModuleDllUtc', $CurrentModuleDllUtc)
    }
    if ($PendingModuleDllUtc) {
        $args += @('-PendingModuleDllUtc', $PendingModuleDllUtc)
    }
    powershell @args | Out-Null
}

function Clear-PendingState([string]$Reason) {
    powershell -ExecutionPolicy Bypass -File $pendingScript -Action clear -ProjectRoot $ProjectRoot -Reason $Reason | Out-Null
}

function Start-SupervisedHost {
    $launchArgs = @(
        '-ExecutionPolicy', 'Bypass',
        '-File', $drive,
        '-Action', 'launch',
        '-MapUrl', $matchUrl,
        '-ExtraArgs', "-ArchonBotMatch -ArchonBotCountPerTeam=$BotsPerTeam",
        '-ProjectRoot', $ProjectRoot
    )
    if ($ForceLaunch) { $launchArgs += '-Force' }

    $out = powershell @launchArgs 2>&1 | Out-String
    if ($LASTEXITCODE -ne 0) {
        Write-SupervisorEvent "launch deferred exit=$LASTEXITCODE output=$($out.Trim())"
        return $null
    }

    $playtestState = Get-JsonFile $playtestStatePath
    if (-not $playtestState -or -not $playtestState.Pid) {
        Write-SupervisorEvent 'launch reported success but no playtest state was written'
        return $null
    }

    $state = [pscustomobject]@{
        SupervisorOwnsHost = $true
        Pid = [int]$playtestState.Pid
        StartedUtc = (Get-Date).ToUniversalTime().ToString('o')
        MapUrl = $matchUrl
        HostEndpoint = $HostEndpoint
        PlaytestStatePath = $playtestStatePath
        PlaytestLogPath = $playtestLogPath
        AllowBoundaryRefresh = [bool]$AllowBoundaryRefresh
        BuildAtBoundary = [bool]$BuildAtBoundary
        BuildScriptPath = $BuildScriptPath
    }
    $state | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $supervisorStatePath -Encoding UTF8
    Clear-PendingState 'supervisor_host_started_or_refreshed'
    Write-SupervisorEvent "host launched supervisorOwned=true pid=$($state.Pid)"
    return $state
}

$iteration = 0
$observedExternalLivePid = 0

if (-not (Enter-SupervisorLoopLock)) {
    exit 0
}

try {
while ($true) {
    $iteration++
    $state = Get-JsonFile $supervisorStatePath
    $pendingPath = Join-Path $proofDir 'host-refresh-pending.json'
    $pending = Get-JsonFile $pendingPath
    $logText = Get-PlaytestLogText
    $fingerprint = Get-LatestFingerprint $logText
    $nextMap = Get-LatestNextMap $logText

    if ($state -and $state.SupervisorOwnsHost -and (Get-ProcessAlive ([int]$state.Pid))) {
        $detected = Get-PendingAdoption $fingerprint
        if ($detected -and $detected.BoundaryBuildBlocked) {
            Clear-PendingState $detected.Reason
            $pending = $null
            Write-SupervisorEvent "boundary build blocked for current inputs; keeping supervisor-owned host running pid=$($state.Pid) reason=$($detected.Reason)"
        }
        if ($detected -and $detected.Pending) {
            Write-PendingState `
                -Reason $detected.Reason `
                -ProcessId ([int]$state.Pid) `
                -Fingerprint $fingerprint `
                -NextMap $nextMap `
                -OwnsHost $true `
                -Mode 'supervisor_owned_waiting_for_match_boundary' `
                -ClientState 'host_refresh_pending_reconnect_placeholder' `
                -CurrentModuleDllUtc $detected.CurrentModuleDllUtc `
                -PendingModuleDllUtc $detected.PendingModuleDllUtc
            $pending = Get-JsonFile $pendingPath
        }

        $hasPending = [bool](($pending -and $pending.Pending) -or ($detected -and $detected.Pending))
        if ($hasPending) {
            $refreshReason = if ($detected -and $detected.Pending) { $detected.Reason } else { $pending.Reason }
            $refreshRequiresBuild = [bool]($detected -and $detected.Pending -and $detected.RequiresBuild)
            $currentModuleUtc = if ($detected -and $detected.CurrentModuleDllUtc) { $detected.CurrentModuleDllUtc } else { '' }
            $pendingModuleUtc = if ($detected -and $detected.PendingModuleDllUtc) { $detected.PendingModuleDllUtc } elseif ($pending -and $pending.PendingModuleDllUtc) { $pending.PendingModuleDllUtc } else { '' }
            Write-PendingState `
                -Reason $refreshReason `
                -ProcessId ([int]$state.Pid) `
                -Fingerprint $fingerprint `
                -NextMap $nextMap `
                -OwnsHost $true `
                -Mode 'supervisor_owned_waiting_for_match_boundary' `
                -ClientState 'host_refresh_pending_reconnect_placeholder' `
                -CurrentModuleDllUtc $currentModuleUtc `
                -PendingModuleDllUtc $pendingModuleUtc

            $atBoundary = Test-AtMatchBoundary $logText
            $liveStale = Test-LiveMatchStale -Text $logText -StaleSeconds $LiveStaleSeconds
            if ($AllowBoundaryRefresh -and ($atBoundary -or $liveStale)) {
                if ($refreshRequiresBuild -and -not $BuildAtBoundary) {
                    Write-SupervisorEvent "boundary refresh deferred because source build is pending but BuildAtBoundary=false pid=$($state.Pid)"
                } else {
                    $beforePid = [int]$state.Pid
                    $beforeFingerprint = $fingerprint
                    $refreshTrigger = if ($atBoundary) { 'next_match_boundary' } else { 'stale_live_timeout' }
                    Write-PendingState `
                        -Reason ('fresh_build_{0}_refresh' -f $refreshTrigger) `
                        -ProcessId $beforePid `
                        -Fingerprint $beforeFingerprint `
                        -NextMap $nextMap `
                        -OwnsHost $true `
                        -Mode 'supervisor_refreshing_at_match_boundary' `
                        -ClientState 'host_refreshing_build_reconnecting' `
                        -CurrentModuleDllUtc $currentModuleUtc `
                        -PendingModuleDllUtc $pendingModuleUtc
                    Write-SupervisorEvent "boundary refresh stopping supervisor-owned host pid=$beforePid nextMap=$nextMap reason=$refreshReason trigger=$refreshTrigger"
                    powershell -ExecutionPolicy Bypass -File $drive -Action stop -AllowLiveStop -ProjectRoot $ProjectRoot | Out-Null
                    Start-Sleep -Seconds 1
                    $buildResult = Invoke-BoundaryBuild -Reason $refreshReason -RequiresBuild $refreshRequiresBuild
                    if ($buildResult.BuildRequired -and -not $buildResult.BuildSucceeded) {
                        Write-BoundaryBuildBlocked `
                            -Reason 'boundary_build_failed_latest_not_adopted' `
                            -PendingModuleText $buildResult.ModuleDllUtcAfterBuild `
                            -CompileNewestText $(if ($detected) { $detected.CompileNewestUtc } else { '' }) `
                            -NativeRequiredText $(if ($detected) { $detected.NativeRequiredAfterUtc } else { '' }) `
                            -BuildResult $buildResult
                    }
                    $newState = Start-SupervisedHost
                    $afterPid = if ($newState) { [int]$newState.Pid } else { 0 }
                    $proof = [pscustomobject]@{
                        ProofUtc = (Get-Date).ToUniversalTime().ToString('o')
                        ProcessBoundary = if ($afterPid -and $afterPid -ne $beforePid) { 'fresh_process_relaunch' } else { 'unknown' }
                        BeforePid = $beforePid
                        AfterPid = $afterPid
                        BeforeBuildFingerprint = $beforeFingerprint
                        NextMap = $nextMap
                        RefreshTrigger = $refreshTrigger
                        PendingReason = $refreshReason
                        PendingDetected = [bool]($detected -and $detected.Pending)
                        BuildAtBoundary = [bool]$BuildAtBoundary
                        BuildAttempted = [bool]$buildResult.BuildAttempted
                        BuildRequired = [bool]$buildResult.BuildRequired
                        BuildExitCode = $buildResult.BuildExitCode
                        PolicyBuildExitCode = $buildResult.PolicyBuildExitCode
                        AutomationExitCode = $buildResult.AutomationExitCode
                        AutomationSucceeded = $buildResult.AutomationSucceeded
                        AutomationFailedCount = $buildResult.AutomationFailedCount
                        BuildSucceeded = [bool]$buildResult.BuildSucceeded
                        ModuleTimestampAdvanced = [bool]$buildResult.ModuleTimestampAdvanced
                        ModuleDllUtcBeforeBuild = $buildResult.ModuleDllUtcBeforeBuild
                        ModuleDllUtcAfterBuild = $buildResult.ModuleDllUtcAfterBuild
                        CompileNewestUtc = if ($detected) { $detected.CompileNewestUtc } else { '' }
                        NativeRequiredAfterUtc = if ($detected) { $detected.NativeRequiredAfterUtc } else { '' }
                        RuntimeNewestUtc = if ($detected) { $detected.RuntimeNewestUtc } else { '' }
                        BoundaryBuildLogPath = $buildResult.BuildLogPath
                        PendingStatePath = $pendingPath
                        SupervisorStatePath = $supervisorStatePath
                        ClientReconnectPlaceholder = $true
                        TouchedExternalLiveProcess = $false
                    }
                    $proof | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $refreshProofPath -Encoding UTF8

                    if ($buildResult.BuildRequired -and -not $buildResult.BuildSucceeded -and $afterPid) {
                        Clear-PendingState 'boundary_build_failed_latest_not_adopted'
                    }
                }
            }
        }
        if (Test-MaxIterationsReached $iteration) { break }
        Start-Sleep -Seconds $PollSeconds
        continue
    }

    if ($state -and $state.SupervisorOwnsHost -and -not (Get-ProcessAlive ([int]$state.Pid))) {
        if ($pending -and $pending.Pending -and $AllowBoundaryRefresh) {
            $beforePid = [int]$state.Pid
            $pendingFingerprint = if ($pending.CurrentBuildFingerprint) { [string]$pending.CurrentBuildFingerprint } else { $fingerprint }
            $exitDetected = Get-PendingAdoption $pendingFingerprint
            $refreshReason = if ($exitDetected -and $exitDetected.Pending) { $exitDetected.Reason } else { [string]$pending.Reason }
            $refreshRequiresBuild = [bool]($exitDetected -and $exitDetected.Pending -and $exitDetected.RequiresBuild)
            $currentModuleUtc = if ($exitDetected -and $exitDetected.CurrentModuleDllUtc) { $exitDetected.CurrentModuleDllUtc } elseif ($pending.CurrentModuleDllUtc) { [string]$pending.CurrentModuleDllUtc } else { '' }
            $pendingModuleUtc = if ($exitDetected -and $exitDetected.PendingModuleDllUtc) { $exitDetected.PendingModuleDllUtc } elseif ($pending.PendingModuleDllUtc) { [string]$pending.PendingModuleDllUtc } else { '' }
            Write-PendingState `
                -Reason 'fresh_build_host_exit_refresh' `
                -ProcessId $beforePid `
                -Fingerprint $pendingFingerprint `
                -NextMap $nextMap `
                -OwnsHost $true `
                -Mode 'supervisor_refreshing_after_host_exit' `
                -ClientState 'host_refreshing_build_reconnecting' `
                -CurrentModuleDllUtc $currentModuleUtc `
                -PendingModuleDllUtc $pendingModuleUtc

            Write-SupervisorEvent "owned host exited during pending refresh; building before relaunch oldPid=$beforePid reason=$refreshReason"
            $buildResult = Invoke-BoundaryBuild -Reason $refreshReason -RequiresBuild $refreshRequiresBuild
            if ($buildResult.BuildRequired -and -not $buildResult.BuildSucceeded) {
                Write-BoundaryBuildBlocked `
                    -Reason 'boundary_build_failed_latest_not_adopted' `
                    -PendingModuleText $buildResult.ModuleDllUtcAfterBuild `
                    -CompileNewestText $(if ($exitDetected) { $exitDetected.CompileNewestUtc } else { '' }) `
                    -NativeRequiredText $(if ($exitDetected) { $exitDetected.NativeRequiredAfterUtc } else { '' }) `
                    -BuildResult $buildResult
            }
            $newState = Start-SupervisedHost
            $afterPid = if ($newState) { [int]$newState.Pid } else { 0 }
            $proof = [pscustomobject]@{
                ProofUtc = (Get-Date).ToUniversalTime().ToString('o')
                ProcessBoundary = if ($afterPid) { 'host_exit_fresh_process_relaunch' } else { 'unknown' }
                BeforePid = $beforePid
                AfterPid = $afterPid
                BeforeBuildFingerprint = $pendingFingerprint
                NextMap = $nextMap
                RefreshTrigger = 'host_exit_pending_refresh'
                PendingReason = $refreshReason
                PendingDetected = [bool]($exitDetected -and $exitDetected.Pending)
                BuildAtBoundary = [bool]$BuildAtBoundary
                BuildAttempted = [bool]$buildResult.BuildAttempted
                BuildRequired = [bool]$buildResult.BuildRequired
                BuildExitCode = $buildResult.BuildExitCode
                PolicyBuildExitCode = $buildResult.PolicyBuildExitCode
                AutomationExitCode = $buildResult.AutomationExitCode
                AutomationSucceeded = $buildResult.AutomationSucceeded
                AutomationFailedCount = $buildResult.AutomationFailedCount
                BuildSucceeded = [bool]$buildResult.BuildSucceeded
                ModuleTimestampAdvanced = [bool]$buildResult.ModuleTimestampAdvanced
                ModuleDllUtcBeforeBuild = $buildResult.ModuleDllUtcBeforeBuild
                ModuleDllUtcAfterBuild = $buildResult.ModuleDllUtcAfterBuild
                CompileNewestUtc = if ($exitDetected) { $exitDetected.CompileNewestUtc } else { '' }
                NativeRequiredAfterUtc = if ($exitDetected) { $exitDetected.NativeRequiredAfterUtc } else { '' }
                RuntimeNewestUtc = if ($exitDetected) { $exitDetected.RuntimeNewestUtc } else { '' }
                BoundaryBuildLogPath = $buildResult.BuildLogPath
                PendingStatePath = $pendingPath
                SupervisorStatePath = $supervisorStatePath
                ClientReconnectPlaceholder = $true
                TouchedExternalLiveProcess = $false
            }
            $proof | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $refreshProofPath -Encoding UTF8

            if ($buildResult.BuildRequired -and -not $buildResult.BuildSucceeded -and $afterPid) {
                Clear-PendingState 'boundary_build_failed_latest_not_adopted'
            }
        } elseif ($RestartOnCrash) {
            Write-SupervisorEvent "owned host exited; RestartOnCrash=true so relaunching oldPid=$($state.Pid)"
            Start-SupervisedHost | Out-Null
        } else {
            Write-SupervisorEvent "owned host exited without pending refresh; treating as intentional quit and stopping supervisor oldPid=$($state.Pid)"
            break
        }
        if (Test-MaxIterationsReached $iteration) { break }
        Start-Sleep -Seconds $PollSeconds
        continue
    }

    $external = if ($HarnessIgnoreExternalCurrentBuild) { $null } else { Get-ExistingCurrentBuildProcess }
    if ($external) {
        $externalPid = [int]$external.ProcessId
        if ($observedExternalLivePid -ne $externalPid) {
            Write-SupervisorEvent "observing existing live current-build pid=$externalPid; supervisor will not attach or stop it"
            $observedExternalLivePid = $externalPid
        }
        $externalLogText = Get-UnrealLogText
        $externalFingerprint = Get-LatestFingerprint $externalLogText
        if (-not $externalFingerprint) { $externalFingerprint = $fingerprint }
        $externalNextMap = Get-LatestNextMap $externalLogText
        if (-not $externalNextMap) { $externalNextMap = $nextMap }
        $externalDetected = Get-PendingAdoption $externalFingerprint
        if ($externalDetected -and $externalDetected.BoundaryBuildBlocked) {
            Clear-PendingState $externalDetected.Reason
            $pending = $null
            Write-SupervisorEvent "boundary build blocked for current inputs; observing existing live current-build pid=$externalPid reason=$($externalDetected.Reason)"
        }
        $hasExternalPending = [bool](($pending -and $pending.Pending) -or ($externalDetected -and $externalDetected.Pending))
        if ($hasExternalPending) {
            $externalReason = if ($externalDetected -and $externalDetected.Pending) { $externalDetected.Reason } else { $pending.Reason }
            $externalRequiresBuild = [bool]($externalDetected -and $externalDetected.Pending -and $externalDetected.RequiresBuild)
            $externalCurrentModuleUtc = if ($externalDetected -and $externalDetected.CurrentModuleDllUtc) { $externalDetected.CurrentModuleDllUtc } else { '' }
            $externalPendingModuleUtc = if ($externalDetected -and $externalDetected.PendingModuleDllUtc) { $externalDetected.PendingModuleDllUtc } elseif ($pending -and $pending.PendingModuleDllUtc) { $pending.PendingModuleDllUtc } else { '' }
            Write-PendingState `
                -Reason $externalReason `
                -ProcessId $externalPid `
                -Fingerprint $externalFingerprint `
                -NextMap $externalNextMap `
                -OwnsHost $false `
                -Mode 'observing_existing_live_session_no_stop' `
                -ClientState 'host_refresh_pending_reconnect_placeholder' `
                -CurrentModuleDllUtc $externalCurrentModuleUtc `
                -PendingModuleDllUtc $externalPendingModuleUtc `
                -ObservedLogPath $unrealLogPath

            $externalAtBoundary = Test-AtMatchBoundary $externalLogText
            $externalLiveStale = Test-LiveMatchStale -Text $externalLogText -StaleSeconds $LiveStaleSeconds
            if ($AllowBoundaryRefresh -and $AdoptExternalAtBoundary -and ($externalAtBoundary -or $externalLiveStale)) {
                if ($externalRequiresBuild -and -not $BuildAtBoundary) {
                    Write-SupervisorEvent "external boundary takeover deferred because source build is pending but BuildAtBoundary=false pid=$externalPid"
                } else {
                    $externalTrigger = if ($externalAtBoundary) { 'next_match_boundary' } else { 'stale_live_timeout' }
                    Write-PendingState `
                        -Reason ('fresh_build_external_{0}_takeover' -f $externalTrigger) `
                        -ProcessId $externalPid `
                        -Fingerprint $externalFingerprint `
                        -NextMap $externalNextMap `
                        -OwnsHost $false `
                        -Mode ('supervisor_adopting_external_at_{0}' -f $externalTrigger) `
                        -ClientState 'host_refreshing_build_reconnecting' `
                        -CurrentModuleDllUtc $externalCurrentModuleUtc `
                        -PendingModuleDllUtc $externalPendingModuleUtc `
                        -ObservedLogPath $unrealLogPath

                    Write-SupervisorEvent "external boundary takeover stopping current-build pid=$externalPid nextMap=$externalNextMap reason=$externalReason trigger=$externalTrigger"
                    $stopResult = Stop-ExternalCurrentBuildProcess -ProcessId $externalPid
                    if (-not $stopResult.StopSucceeded) {
                        Write-SupervisorEvent "external boundary takeover stop failed pid=$externalPid verification=$($stopResult.Verification)"
                        Write-PendingState `
                            -Reason 'external_boundary_takeover_stop_failed' `
                            -ProcessId $externalPid `
                            -Fingerprint $externalFingerprint `
                            -NextMap $externalNextMap `
                            -OwnsHost $false `
                            -Mode 'observing_existing_live_session_no_stop' `
                            -ClientState 'host_refresh_pending_reconnect_placeholder' `
                            -CurrentModuleDllUtc $externalCurrentModuleUtc `
                            -PendingModuleDllUtc $externalPendingModuleUtc `
                            -ObservedLogPath $unrealLogPath
                    } else {
                        Start-Sleep -Seconds 1
                        $buildResult = Invoke-BoundaryBuild -Reason $externalReason -RequiresBuild $externalRequiresBuild
                        if ($buildResult.BuildRequired -and -not $buildResult.BuildSucceeded) {
                            Write-BoundaryBuildBlocked `
                                -Reason 'boundary_build_failed_latest_not_adopted' `
                                -PendingModuleText $buildResult.ModuleDllUtcAfterBuild `
                                -CompileNewestText $(if ($externalDetected) { $externalDetected.CompileNewestUtc } else { '' }) `
                                -NativeRequiredText $(if ($externalDetected) { $externalDetected.NativeRequiredAfterUtc } else { '' }) `
                                -BuildResult $buildResult
                        }
                        $newState = Start-SupervisedHost
                        $afterPid = if ($newState) { [int]$newState.Pid } else { 0 }
                        $proof = [pscustomobject]@{
                            ProofUtc = (Get-Date).ToUniversalTime().ToString('o')
                            ProcessBoundary = if ($afterPid -and $afterPid -ne $externalPid) { 'fresh_process_relaunch' } else { 'unknown' }
                            AdoptedExternalAtBoundary = $true
                            BeforePid = $externalPid
                            AfterPid = $afterPid
                            BeforeBuildFingerprint = $externalFingerprint
                            NextMap = $externalNextMap
                            RefreshTrigger = $externalTrigger
                            PendingReason = $externalReason
                            PendingDetected = [bool]($externalDetected -and $externalDetected.Pending)
                            BuildAtBoundary = [bool]$BuildAtBoundary
                            BuildAttempted = [bool]$buildResult.BuildAttempted
                            BuildRequired = [bool]$buildResult.BuildRequired
                            BuildExitCode = $buildResult.BuildExitCode
                            PolicyBuildExitCode = $buildResult.PolicyBuildExitCode
                            AutomationExitCode = $buildResult.AutomationExitCode
                            AutomationSucceeded = $buildResult.AutomationSucceeded
                            AutomationFailedCount = $buildResult.AutomationFailedCount
                            BuildSucceeded = [bool]$buildResult.BuildSucceeded
                            ModuleTimestampAdvanced = [bool]$buildResult.ModuleTimestampAdvanced
                            ModuleDllUtcBeforeBuild = $buildResult.ModuleDllUtcBeforeBuild
                            ModuleDllUtcAfterBuild = $buildResult.ModuleDllUtcAfterBuild
                            CompileNewestUtc = if ($externalDetected) { $externalDetected.CompileNewestUtc } else { '' }
                            NativeRequiredAfterUtc = if ($externalDetected) { $externalDetected.NativeRequiredAfterUtc } else { '' }
                            RuntimeNewestUtc = if ($externalDetected) { $externalDetected.RuntimeNewestUtc } else { '' }
                            ExternalStopAttempted = [bool]$stopResult.StopAttempted
                            ExternalStopSucceeded = [bool]$stopResult.StopSucceeded
                            ExternalStopVerification = $stopResult.Verification
                            BoundaryBuildLogPath = $buildResult.BuildLogPath
                            PendingStatePath = $pendingPath
                            SupervisorStatePath = $supervisorStatePath
                            ClientReconnectPlaceholder = $true
                            TouchedExternalLiveProcess = -not [bool]$stopResult.HarnessExternal
                        }
                        $proof | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $refreshProofPath -Encoding UTF8

                        if ($buildResult.BuildRequired -and -not $buildResult.BuildSucceeded -and $afterPid) {
                            Clear-PendingState 'boundary_build_failed_latest_not_adopted'
                        }
                    }
                }
            }
        }
        if (Test-MaxIterationsReached $iteration) { break }
        Start-Sleep -Seconds $PollSeconds
        continue
    }

    Start-SupervisedHost | Out-Null
    if (Test-MaxIterationsReached $iteration) { break }
    Start-Sleep -Seconds $PollSeconds
}
}
finally {
    Exit-SupervisorLoopLock
}
