param(
    [ValidateSet('write', 'clear', 'snapshot')]
    [string]$Action = 'snapshot',
    [string]$ProjectRoot = '',
    [string]$Reason = '',
    [int]$CurrentProcessId = 0,
    [string]$CurrentBuildFingerprint = '',
    [string]$CurrentModuleDllUtc = '',
    [string]$PendingModuleDllUtc = '',
    [string]$NextMap = '',
    [string]$HostEndpoint = 'listen://localhost',
    [int]$ReconnectWindowSeconds = 30,
    [string]$BoundaryPolicy = 'restart_on_next_match_when_supervisor_owns_host',
    [object]$SupervisorOwnsHost = $false,
    [string]$SupervisorMode = 'not_running',
    [string]$ClientState = 'host_refresh_pending_reconnect_placeholder',
    [string]$ObservedLogPath = ''
)

$ErrorActionPreference = 'Stop'
if (-not $ProjectRoot) {
    $ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
}

$proofDir = Join-Path $ProjectRoot 'Saved\Proof'
$pendingPath = Join-Path $proofDir 'host-refresh-pending.json'
$historyPath = Join-Path $proofDir 'host-refresh-pending-history.jsonl'
$supervisorSessionPath = Join-Path $proofDir 'host-supervisor-session.json'
$supervisorPidPath = Join-Path $proofDir 'host-supervisor-loop.pid.json'
$dllPath = Join-Path $ProjectRoot 'Binaries\Win64\UnrealEditor-ArchonFactoryCanary.dll'
$defaultLogPath = Join-Path $ProjectRoot 'Saved\Logs\ArchonFactoryCanary.log'
New-Item -ItemType Directory -Force -Path $proofDir | Out-Null

function Get-TextOrEmpty([string]$Path) {
    if (-not $Path -or -not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        return ''
    }
    $content = Get-Content -LiteralPath $Path -Raw
    if ($null -eq $content) { return '' }
    return $content
}

function Read-JsonOrNull([string]$Path) {
    if (-not $Path -or -not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        return $null
    }
    try {
        return Get-Content -Raw -LiteralPath $Path | ConvertFrom-Json
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

function Get-LastRegexValue([string]$Text, [string]$Pattern, [int]$GroupIndex = 0) {
    $matches = [regex]::Matches($Text, $Pattern)
    if ($matches.Count -eq 0) { return '' }
    return $matches[$matches.Count - 1].Groups[$GroupIndex].Value
}

function Get-ProcessByIdOrNull([int]$ProcessId) {
    if ($ProcessId -le 0) { return $null }
    try {
        return Get-CimInstance Win32_Process -Filter "ProcessId = $ProcessId" -ErrorAction SilentlyContinue
    } catch {
        return $null
    }
}

function Get-LiveCurrentBuildPid {
    try {
        $process = Get-CimInstance Win32_Process |
            Where-Object {
                $_.Name -eq 'UnrealEditor.exe' -and
                $_.CommandLine -match 'ArchonFactoryCanary\.uproject'
            } |
            Sort-Object ProcessId |
            Select-Object -First 1
        if ($process) { return [int]$process.ProcessId }
    } catch {}
    return 0
}

function Convert-ToBool($Value) {
    if ($null -eq $Value) { return $false }
    if ($Value -is [bool]) { return $Value }
    $text = ([string]$Value).Trim()
    return $text -in @('1', 'true', 'True', 'TRUE', '$true')
}

function Get-MapIdFromUrl([string]$MapUrl) {
    if (-not $MapUrl) { return '' }
    $match = [regex]::Match($MapUrl, 'ArchonMapId=([^?&]+)')
    if (-not $match.Success) { return '' }
    return $match.Groups[1].Value
}

function Get-SupervisorSnapshot {
    $session = Read-JsonOrNull $supervisorSessionPath
    $pidState = Read-JsonOrNull $supervisorPidPath

    $supervisorPid = [int](Get-JsonProp $pidState 'Pid' 0)
    $supervisorProcess = Get-ProcessByIdOrNull $supervisorPid
    if (-not $supervisorProcess) {
        try {
            $supervisorProcess = Get-CimInstance Win32_Process -ErrorAction SilentlyContinue |
                Where-Object {
                    $_.Name -eq 'powershell.exe' -and
                    $_.CommandLine -match 'host-supervisor-loop\.ps1' -and
                    $_.CommandLine -match [regex]::Escape($ProjectRoot)
                } |
                Sort-Object ProcessId |
                Select-Object -First 1
            if ($supervisorProcess) {
                $supervisorPid = [int]$supervisorProcess.ProcessId
            }
        } catch {}
    }

    $hostPid = [int](Get-JsonProp $session 'Pid' 0)
    $hostProcess = Get-ProcessByIdOrNull $hostPid
    $sessionOwnsHost = Convert-ToBool (Get-JsonProp $session 'SupervisorOwnsHost' $false)
    $ownsLiveHost = $sessionOwnsHost -and $hostProcess

    $mode = 'not_running'
    if ($supervisorProcess -and $ownsLiveHost) {
        $mode = 'supervisor_owned_waiting_for_match_boundary'
    } elseif ($supervisorProcess) {
        $mode = 'supervisor_running_without_owned_host'
    } elseif ($ownsLiveHost) {
        $mode = 'supervisor_session_host_running_without_loop_process'
    }

    [pscustomobject]@{
        SupervisorProcessId = $supervisorPid
        SupervisorRunning = [bool]$supervisorProcess
        SupervisorOwnsHost = [bool]$ownsLiveHost
        SupervisorMode = $mode
        CurrentProcessId = $hostPid
        PlaytestLogPath = [string](Get-JsonProp $session 'PlaytestLogPath' '')
        HostEndpoint = [string](Get-JsonProp $session 'HostEndpoint' '')
        NextMap = Get-MapIdFromUrl ([string](Get-JsonProp $session 'MapUrl' ''))
    }
}

$existingState = Read-JsonOrNull $pendingPath
$existingPending = $Action -eq 'snapshot' -and $existingState -and (Convert-ToBool (Get-JsonProp $existingState 'Pending' $false))
$supervisorSnapshot = Get-SupervisorSnapshot

if ($Action -eq 'snapshot') {
    if (-not (Convert-ToBool $SupervisorOwnsHost) -and $SupervisorMode -eq 'not_running' -and $supervisorSnapshot.SupervisorOwnsHost) {
        $SupervisorOwnsHost = $true
        $SupervisorMode = $supervisorSnapshot.SupervisorMode
    } elseif ($SupervisorMode -eq 'not_running' -and $supervisorSnapshot.SupervisorRunning) {
        $SupervisorMode = $supervisorSnapshot.SupervisorMode
    }
    if (-not $CurrentProcessId -and $supervisorSnapshot.CurrentProcessId) {
        $CurrentProcessId = $supervisorSnapshot.CurrentProcessId
    }
    if (-not $ObservedLogPath -and $supervisorSnapshot.PlaytestLogPath) {
        $ObservedLogPath = $supervisorSnapshot.PlaytestLogPath
    }
    if ($HostEndpoint -eq 'listen://localhost' -and $supervisorSnapshot.HostEndpoint) {
        $HostEndpoint = $supervisorSnapshot.HostEndpoint
    }
    if (-not $NextMap -and $supervisorSnapshot.NextMap) {
        $NextMap = $supervisorSnapshot.NextMap
    }
    if ($existingState) {
        if (-not $CurrentBuildFingerprint) {
            $CurrentBuildFingerprint = [string](Get-JsonProp $existingState 'CurrentBuildFingerprint' '')
        }
        if (-not $CurrentModuleDllUtc) {
            $CurrentModuleDllUtc = [string](Get-JsonProp $existingState 'CurrentModuleDllUtc' '')
        }
        if (-not $PendingModuleDllUtc) {
            $PendingModuleDllUtc = [string](Get-JsonProp $existingState 'PendingModuleDllUtc' '')
        }
    }
}

$logPathForRead = if ($ObservedLogPath) { $ObservedLogPath } else { $defaultLogPath }
$logText = Get-TextOrEmpty $logPathForRead

if (-not $CurrentProcessId) {
    $CurrentProcessId = Get-LiveCurrentBuildPid
}
if (-not $CurrentBuildFingerprint) {
    $CurrentBuildFingerprint = Get-LastRegexValue $logText 'ArchonFactoryCanary: BuildFingerprint[^\r\n]+'
}
if (-not $NextMap) {
    $NextMap = Get-LastRegexValue $logText 'ArchonFactoryCanary: NextMatchPending nextMap=(\S+)' 1
}
if (-not $NextMap) {
    $NextMap = 'splitroot_valley'
}
if (-not $CurrentModuleDllUtc -and $CurrentBuildFingerprint -match 'moduleDllUtc=(\S+)') {
    $CurrentModuleDllUtc = $Matches[1]
}
if (-not $PendingModuleDllUtc -and (Test-Path -LiteralPath $dllPath -PathType Leaf)) {
    $PendingModuleDllUtc = (Get-Item -LiteralPath $dllPath).LastWriteTimeUtc.ToString('o')
}

if ($Action -eq 'clear') {
    $clearState = [pscustomobject]@{
        Pending = $false
        ClearedUtc = (Get-Date).ToUniversalTime().ToString('o')
        Reason = if ($Reason) { $Reason } else { 'refresh_adopted_or_cancelled' }
        PreviousPath = $pendingPath
    }
    $clearState | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $pendingPath -Encoding UTF8
    ($clearState | ConvertTo-Json -Compress -Depth 6) | Add-Content -LiteralPath $historyPath -Encoding UTF8
    $clearState | ConvertTo-Json -Depth 6
    exit 0
}

$pending = ($Action -eq 'write') -or $existingPending
$stateReason = if ($Reason) {
    $Reason
} elseif ($existingPending) {
    [string](Get-JsonProp $existingState 'Reason' 'fresh_build_pending')
} elseif ($pending) {
    'fresh_build_pending'
} else {
    'snapshot'
}
$state = [pscustomobject]@{
    Pending = $pending
    ObservedUtc = (Get-Date).ToUniversalTime().ToString('o')
    Reason = $stateReason
    BoundaryPolicy = $BoundaryPolicy
    SupervisorMode = $SupervisorMode
    SupervisorOwnsHost = Convert-ToBool $SupervisorOwnsHost
    CurrentProcessId = $CurrentProcessId
    CurrentBuildFingerprint = $CurrentBuildFingerprint
    CurrentModuleDllUtc = $CurrentModuleDllUtc
    PendingModuleDllUtc = $PendingModuleDllUtc
    NextMap = $NextMap
    HostEndpoint = $HostEndpoint
    ReconnectWindowSeconds = $ReconnectWindowSeconds
    ClientReconnect = [pscustomobject]@{
        State = $ClientState
        MessageKey = 'host_refreshing_build_reconnecting'
        ExpectedReturn = 'same_host_next_process'
        PlaceholderOnly = $true
    }
    LogPath = $logPathForRead
    StatePath = $pendingPath
}

$state | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $pendingPath -Encoding UTF8
($state | ConvertTo-Json -Compress -Depth 8) | Add-Content -LiteralPath $historyPath -Encoding UTF8
$state | ConvertTo-Json -Depth 8
