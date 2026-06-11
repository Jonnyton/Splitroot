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

function Get-LastRegexValue([string]$Text, [string]$Pattern, [int]$GroupIndex = 0) {
    $matches = [regex]::Matches($Text, $Pattern)
    if ($matches.Count -eq 0) { return '' }
    return $matches[$matches.Count - 1].Groups[$GroupIndex].Value
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

$pending = $Action -eq 'write'
$state = [pscustomobject]@{
    Pending = $pending
    ObservedUtc = (Get-Date).ToUniversalTime().ToString('o')
    Reason = if ($Reason) { $Reason } elseif ($pending) { 'fresh_build_pending' } else { 'snapshot' }
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
