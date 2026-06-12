param(
    [string]$ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path,
    [switch]$NoRefreshSnapshot
)

$ErrorActionPreference = 'Stop'

$proofDir = Join-Path $ProjectRoot 'Saved\Proof'
$statusPath = Join-Path $proofDir 'current-build-hot-reload-status.json'
$pendingStateScript = Join-Path $ProjectRoot 'Proof\host-refresh-pending-state.ps1'
$pendingPath = Join-Path $proofDir 'host-refresh-pending.json'
$supervisorSessionPath = Join-Path $proofDir 'host-supervisor-session.json'
$supervisorPidPath = Join-Path $proofDir 'host-supervisor-loop.pid.json'
$supervisorLogPath = Join-Path $proofDir 'host-supervisor-loop.log'
$refreshProofPath = Join-Path $proofDir 'last-host-supervisor-refresh.json'
$blockedPath = Join-Path $proofDir 'host-supervisor-boundary-build-blocked.json'
$boundaryBuildLogPath = Join-Path $proofDir 'host-supervisor-boundary-build.log'
$moduleDllPath = Join-Path $ProjectRoot 'Binaries\Win64\UnrealEditor-ArchonFactoryCanary.dll'
$unrealLogPath = Join-Path $ProjectRoot 'Saved\Logs\ArchonFactoryCanary.log'
$desktopShortcut = 'C:\Users\Jonathan\Desktop\SPLITROOT Current Build.lnk'
$currentBuildLauncher = Join-Path $ProjectRoot 'Launchers\Play-CurrentBuild.cmd'
$splitrootLauncher = Join-Path $ProjectRoot 'games\splitroot\Launchers\Play-ArchonFactoryCanary.cmd'

New-Item -ItemType Directory -Force -Path $proofDir | Out-Null

function Read-JsonOrNull([string]$Path) {
    if (-not $Path -or -not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        return $null
    }
    try {
        return Get-Content -LiteralPath $Path -Raw | ConvertFrom-Json
    } catch {
        return $null
    }
}

function Get-JsonProp($Object, [string]$Name, $DefaultValue = $null) {
    if (-not $Object) { return $DefaultValue }
    $prop = $Object.PSObject.Properties[$Name]
    if (-not $prop) { return $DefaultValue }
    if ($null -eq $prop.Value) { return $DefaultValue }
    return $prop.Value
}

function Convert-ToBool($Value) {
    if ($null -eq $Value) { return $false }
    if ($Value -is [bool]) { return $Value }
    $text = ([string]$Value).Trim()
    return $text -in @('1', 'true', 'True', 'TRUE', '$true')
}

function Normalize-PathString($Path) {
    if ([string]::IsNullOrWhiteSpace($Path)) { return '' }
    return [System.IO.Path]::GetFullPath($Path).TrimEnd('\')
}

function Get-TextOrEmpty([string]$Path) {
    if (-not $Path -or -not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        return ''
    }
    $content = Get-Content -LiteralPath $Path -Raw
    if ($null -eq $content) { return '' }
    return $content
}

function Get-ProcessByIdOrNull([int]$ProcessId) {
    if ($ProcessId -le 0) { return $null }
    try {
        return Get-CimInstance Win32_Process -Filter "ProcessId = $ProcessId" -ErrorAction SilentlyContinue
    } catch {
        return $null
    }
}

function Get-CurrentBuildProcesses {
    try {
        return @(
            Get-CimInstance Win32_Process |
                Where-Object {
                    $_.Name -eq 'UnrealEditor.exe' -and
                    $_.CommandLine -match 'ArchonFactoryCanary\.uproject'
                } |
                Sort-Object ProcessId |
                ForEach-Object {
                    [pscustomobject]@{
                        ProcessId = [int]$_.ProcessId
                        CommandLine = $_.CommandLine
                    }
                }
        )
    } catch {
        return @()
    }
}

function Find-SupervisorProcess([int]$ExpectedPid) {
    $process = Get-ProcessByIdOrNull $ExpectedPid
    if ($process) { return $process }

    try {
        return Get-CimInstance Win32_Process -ErrorAction SilentlyContinue |
            Where-Object {
                $_.Name -eq 'powershell.exe' -and
                $_.CommandLine -match 'host-supervisor-loop\.ps1' -and
                $_.CommandLine -match [regex]::Escape($ProjectRoot)
            } |
            Sort-Object ProcessId |
            Select-Object -First 1
    } catch {
        return $null
    }
}

function Get-ShortcutInfo {
    $exists = Test-Path -LiteralPath $desktopShortcut -PathType Leaf
    if (-not $exists) {
        return [pscustomobject]@{
            Exists = $false
            TargetPath = ''
            WorkingDirectory = ''
            TargetMatchesCurrentBuildLauncher = $false
            WorkingDirectoryMatchesProjectRoot = $false
        }
    }

    $shell = New-Object -ComObject WScript.Shell
    $shortcut = $shell.CreateShortcut($desktopShortcut)
    [pscustomobject]@{
        Exists = $true
        TargetPath = $shortcut.TargetPath
        WorkingDirectory = $shortcut.WorkingDirectory
        TargetMatchesCurrentBuildLauncher = [string]::Equals(
            (Normalize-PathString $shortcut.TargetPath),
            (Normalize-PathString $currentBuildLauncher),
            [System.StringComparison]::OrdinalIgnoreCase
        )
        WorkingDirectoryMatchesProjectRoot = [string]::Equals(
            (Normalize-PathString $shortcut.WorkingDirectory),
            (Normalize-PathString $ProjectRoot),
            [System.StringComparison]::OrdinalIgnoreCase
        )
    }
}

function Get-LastRegexValue([string]$Text, [string]$Pattern, [int]$GroupIndex = 0) {
    $matches = [regex]::Matches($Text, $Pattern)
    if ($matches.Count -eq 0) { return '' }
    return $matches[$matches.Count - 1].Groups[$GroupIndex].Value
}

function Get-FingerprintField([string]$Fingerprint, [string]$Name) {
    if (-not $Fingerprint) { return '' }
    $match = [regex]::Match($Fingerprint, ([regex]::Escape($Name) + '=([^\s]+)'))
    if (-not $match.Success) { return '' }
    return $match.Groups[1].Value
}

$snapshot = $null
if (-not $NoRefreshSnapshot -and (Test-Path -LiteralPath $pendingStateScript -PathType Leaf)) {
    $snapshotRaw = (& powershell -NoProfile -ExecutionPolicy Bypass -File $pendingStateScript -Action snapshot -ProjectRoot $ProjectRoot 2>$null) | Out-String
    if ($LASTEXITCODE -eq 0 -and $snapshotRaw.Trim()) {
        try {
            $snapshot = $snapshotRaw | ConvertFrom-Json
        } catch {
            $snapshot = $null
        }
    }
}

$pending = if ($snapshot) { $snapshot } else { Read-JsonOrNull $pendingPath }
$session = Read-JsonOrNull $supervisorSessionPath
$pidState = Read-JsonOrNull $supervisorPidPath
$refreshProof = Read-JsonOrNull $refreshProofPath
$blocked = Read-JsonOrNull $blockedPath
$liveProcesses = @(Get-CurrentBuildProcesses)
$shortcutInfo = Get-ShortcutInfo

$currentLauncherText = Get-TextOrEmpty $currentBuildLauncher
$splitrootLauncherText = Get-TextOrEmpty $splitrootLauncher
$launcherWired = [bool](
    (Test-Path -LiteralPath $currentBuildLauncher -PathType Leaf) -and
    (Test-Path -LiteralPath $splitrootLauncher -PathType Leaf) -and
    $currentLauncherText -match 'games\\splitroot\\Launchers\\Play-ArchonFactoryCanary\.cmd' -and
    $splitrootLauncherText -match 'host-supervisor-loop\.ps1' -and
    $splitrootLauncherText -match '-AllowBoundaryRefresh' -and
    $splitrootLauncherText -match '-BuildAtBoundary' -and
    $splitrootLauncherText -match '-AdoptExternalAtBoundary' -and
    $splitrootLauncherText -match '-RestartOnCrash' -and
    $splitrootLauncherText -match '-ForceLaunch' -and
    $shortcutInfo.Exists -and
    $shortcutInfo.TargetMatchesCurrentBuildLauncher -and
    $shortcutInfo.WorkingDirectoryMatchesProjectRoot
)

$supervisorPid = [int](Get-JsonProp $pidState 'Pid' 0)
$supervisorProcess = Find-SupervisorProcess $supervisorPid
if ($supervisorProcess) {
    $supervisorPid = [int]$supervisorProcess.ProcessId
}
$supervisorRunning = [bool]$supervisorProcess
$supervisorFlagsReady = [bool](
    (Convert-ToBool (Get-JsonProp $pidState 'AllowBoundaryRefresh' $false)) -and
    (Convert-ToBool (Get-JsonProp $pidState 'BuildAtBoundary' $false)) -and
    (Convert-ToBool (Get-JsonProp $pidState 'AdoptExternalAtBoundary' $false))
)

$sessionHostPid = [int](Get-JsonProp $session 'Pid' 0)
$sessionHostAlive = [bool](Get-ProcessByIdOrNull $sessionHostPid)
$sessionOwnsHost = Convert-ToBool (Get-JsonProp $session 'SupervisorOwnsHost' $false)
$supervisorOwnsLiveHost = [bool]($sessionOwnsHost -and $sessionHostAlive)
$hasCurrentBuildProcess = [bool]($liveProcesses.Count -gt 0 -or $sessionHostAlive)
$pendingActive = Convert-ToBool (Get-JsonProp $pending 'Pending' $false)
$buildBlocked = [bool]($blocked -and (Convert-ToBool (Get-JsonProp $blocked 'BuildBlocked' $false)))

$effectiveLogPath = [string](Get-JsonProp $pending 'LogPath' '')
if (-not $effectiveLogPath) {
    $effectiveLogPath = [string](Get-JsonProp $session 'PlaytestLogPath' '')
}
if (-not $effectiveLogPath) {
    $effectiveLogPath = $unrealLogPath
}
$logText = Get-TextOrEmpty $effectiveLogPath
$effectiveFingerprint = Get-LastRegexValue $logText 'ArchonFactoryCanary: BuildFingerprint[^\r\n]+'
if (-not $effectiveFingerprint) {
    $effectiveFingerprint = [string](Get-JsonProp $pending 'CurrentBuildFingerprint' '')
}

$latestRefreshGreen = [bool](
    $refreshProof -and
    (Convert-ToBool (Get-JsonProp $refreshProof 'BuildSucceeded' $false)) -and
    -not (Convert-ToBool (Get-JsonProp $refreshProof 'TouchedExternalLiveProcess' $false)) -and
    (
        (Get-JsonProp $refreshProof 'ProcessBoundary' '') -in @(
            'fresh_process_relaunch',
            'host_exit_fresh_process_relaunch'
        )
    )
)

$status = 'hot_reload_ready'
$exitCode = 0
if (-not $launcherWired) {
    $status = 'hot_reload_launcher_not_wired'
    $exitCode = 1
} elseif (-not $hasCurrentBuildProcess) {
    $status = 'current_build_not_running'
    $exitCode = 1
} elseif (-not $supervisorRunning) {
    $status = 'hot_reload_no_supervisor'
    $exitCode = 1
} elseif (-not $supervisorFlagsReady) {
    $status = 'hot_reload_supervisor_missing_boundary_flags'
    $exitCode = 1
} elseif ($buildBlocked) {
    $status = 'hot_reload_blocked_boundary_build_failed'
    $exitCode = 1
} elseif ($pendingActive) {
    $status = 'hot_reload_pending_next_boundary'
} elseif (-not $supervisorOwnsLiveHost) {
    $status = 'hot_reload_armed_observing_external_until_needed'
}

$result = [pscustomobject]@{
    Status = $status
    CheckedUtc = (Get-Date).ToUniversalTime().ToString('o')
    ProjectRoot = $ProjectRoot
    StatusPath = $statusPath
    DesktopShortcut = $desktopShortcut
    Shortcut = $shortcutInfo
    Wiring = [pscustomobject]@{
        LauncherWired = $launcherWired
        CurrentBuildLauncher = $currentBuildLauncher
        SplitrootLauncher = $splitrootLauncher
        CurrentBuildLauncherExists = Test-Path -LiteralPath $currentBuildLauncher -PathType Leaf
        SplitrootLauncherExists = Test-Path -LiteralPath $splitrootLauncher -PathType Leaf
        StartsHostSupervisor = [bool]($splitrootLauncherText -match 'host-supervisor-loop\.ps1')
        AllowBoundaryRefresh = [bool]($splitrootLauncherText -match '-AllowBoundaryRefresh')
        BuildAtBoundary = [bool]($splitrootLauncherText -match '-BuildAtBoundary')
        AdoptExternalAtBoundary = [bool]($splitrootLauncherText -match '-AdoptExternalAtBoundary')
        RestartOnCrash = [bool]($splitrootLauncherText -match '-RestartOnCrash')
        ForceLaunch = [bool]($splitrootLauncherText -match '-ForceLaunch')
    }
    Supervisor = [pscustomobject]@{
        Running = $supervisorRunning
        ProcessId = $supervisorPid
        FlagsReady = $supervisorFlagsReady
        LoopPidStatePath = $supervisorPidPath
        SessionPath = $supervisorSessionPath
        LogPath = $supervisorLogPath
        OwnsLiveHost = $supervisorOwnsLiveHost
        SessionHostPid = $sessionHostPid
        SessionHostAlive = $sessionHostAlive
        AllowBoundaryRefresh = Convert-ToBool (Get-JsonProp $pidState 'AllowBoundaryRefresh' $false)
        BuildAtBoundary = Convert-ToBool (Get-JsonProp $pidState 'BuildAtBoundary' $false)
        AdoptExternalAtBoundary = Convert-ToBool (Get-JsonProp $pidState 'AdoptExternalAtBoundary' $false)
    }
    LiveCurrentBuild = [pscustomobject]@{
        Running = $hasCurrentBuildProcess
        Processes = $liveProcesses
        EffectiveLogPath = $effectiveLogPath
        EffectiveBuildFingerprint = $effectiveFingerprint
        EffectiveVersion = Get-FingerprintField $effectiveFingerprint 'version'
        EffectiveUtc = Get-FingerprintField $effectiveFingerprint 'effectiveUtc'
        EffectiveModuleDllUtc = Get-FingerprintField $effectiveFingerprint 'moduleDllUtc'
        EffectiveProcessStartUtc = Get-FingerprintField $effectiveFingerprint 'processStartUtc'
        ModuleDllPath = $moduleDllPath
        ModuleDllUtc = if (Test-Path -LiteralPath $moduleDllPath -PathType Leaf) { (Get-Item -LiteralPath $moduleDllPath).LastWriteTimeUtc.ToString('o') } else { '' }
    }
    Pending = [pscustomobject]@{
        Pending = $pendingActive
        Reason = [string](Get-JsonProp $pending 'Reason' '')
        SupervisorMode = [string](Get-JsonProp $pending 'SupervisorMode' '')
        CurrentProcessId = [int](Get-JsonProp $pending 'CurrentProcessId' 0)
        CurrentModuleDllUtc = [string](Get-JsonProp $pending 'CurrentModuleDllUtc' '')
        PendingModuleDllUtc = [string](Get-JsonProp $pending 'PendingModuleDllUtc' '')
        NextMap = [string](Get-JsonProp $pending 'NextMap' '')
        StatePath = $pendingPath
    }
    LastRefresh = [pscustomobject]@{
        Path = $refreshProofPath
        Exists = [bool]$refreshProof
        Green = $latestRefreshGreen
        ProofUtc = [string](Get-JsonProp $refreshProof 'ProofUtc' '')
        ProcessBoundary = [string](Get-JsonProp $refreshProof 'ProcessBoundary' '')
        RefreshTrigger = [string](Get-JsonProp $refreshProof 'RefreshTrigger' '')
        PendingReason = [string](Get-JsonProp $refreshProof 'PendingReason' '')
        BeforePid = [int](Get-JsonProp $refreshProof 'BeforePid' 0)
        AfterPid = [int](Get-JsonProp $refreshProof 'AfterPid' 0)
        BuildAttempted = Convert-ToBool (Get-JsonProp $refreshProof 'BuildAttempted' $false)
        BuildRequired = Convert-ToBool (Get-JsonProp $refreshProof 'BuildRequired' $false)
        BuildSucceeded = Convert-ToBool (Get-JsonProp $refreshProof 'BuildSucceeded' $false)
        ModuleTimestampAdvanced = Convert-ToBool (Get-JsonProp $refreshProof 'ModuleTimestampAdvanced' $false)
        AutomationSucceeded = Convert-ToBool (Get-JsonProp $refreshProof 'AutomationSucceeded' $false)
        TouchedExternalLiveProcess = Convert-ToBool (Get-JsonProp $refreshProof 'TouchedExternalLiveProcess' $false)
    }
    BoundaryBuild = [pscustomobject]@{
        LogPath = $boundaryBuildLogPath
        BlockedPath = $blockedPath
        Blocked = $buildBlocked
        BlockedReason = [string](Get-JsonProp $blocked 'Reason' '')
    }
    Interpretation = [pscustomobject]@{
        ReadyMeans = 'launcher and supervisor are wired so completed work is adopted at the next match boundary'
        PendingMeans = 'newer source/runtime input exists and the next match boundary should build/relaunch before play continues'
        BlockedMeans = 'the latest boundary build failed; sessions must fix the build before claiming live adoption'
    }
}

$result | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $statusPath -Encoding UTF8
$result | ConvertTo-Json -Depth 8
exit $exitCode
