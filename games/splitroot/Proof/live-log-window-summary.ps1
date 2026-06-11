param(
    [string]$ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..\..')).Path,
    [string]$LogPath = '',
    [int]$TailLines = 500,
    [string]$OutputPath = ''
)

$ErrorActionPreference = 'Stop'

function Resolve-LiveCanaryLogPath([string]$Root) {
    $defaultPath = Join-Path $Root 'Saved\Logs\ArchonFactoryCanary.log'
    $processes = @()
    try {
        $processes = @(Get-CimInstance Win32_Process -Filter "Name = 'UnrealEditor.exe'" -ErrorAction SilentlyContinue |
            Where-Object { $_.CommandLine -like '*ArchonFactoryCanary.uproject*' } |
            Sort-Object CreationDate -Descending)
    } catch {
        return $defaultPath
    }

    foreach ($process in $processes) {
        $match = [regex]::Match([string]$process.CommandLine, '-abslog=(?:"([^"]+)"|(\S+))')
        if (-not $match.Success) {
            continue
        }

        $candidate = if ($match.Groups[1].Success) { $match.Groups[1].Value } else { $match.Groups[2].Value }
        if (-not [System.IO.Path]::IsPathRooted($candidate)) {
            $candidate = Join-Path $Root $candidate
        }
        if (Test-Path -LiteralPath $candidate -PathType Leaf) {
            return $candidate
        }
    }

    return $defaultPath
}

if (-not $LogPath) {
    $LogPath = Resolve-LiveCanaryLogPath $ProjectRoot
}
if (-not $OutputPath) {
    $OutputPath = Join-Path $ProjectRoot 'Saved\Proof\last-live-log-window-summary.json'
}

$proofDir = Split-Path -Parent $OutputPath
New-Item -ItemType Directory -Force -Path $proofDir | Out-Null

function Add-Count([hashtable]$Table, [string]$Key) {
    if ([string]::IsNullOrWhiteSpace($Key)) { $Key = 'unknown' }
    if (-not $Table.ContainsKey($Key)) { $Table[$Key] = 0 }
    $Table[$Key]++
}

function Get-TopCounts([hashtable]$Table, [int]$Limit = 10) {
    $items = foreach ($key in $Table.Keys) {
        [pscustomobject]@{ Key = $key; Count = [int]$Table[$key] }
    }
    @($items | Sort-Object -Property @{ Expression = 'Count'; Descending = $true }, @{ Expression = 'Key'; Ascending = $true } | Select-Object -First $Limit)
}

function Get-ZoneKey([double]$X, [double]$Y, [double]$CellSize = 500.0) {
    $zoneX = [math]::Round($X / $CellSize) * $CellSize
    $zoneY = [math]::Round($Y / $CellSize) * $CellSize
    'x={0};y={1}' -f [int]$zoneX, [int]$zoneY
}

$exists = Test-Path -LiteralPath $LogPath -PathType Leaf
$lines = if ($exists) { @(Get-Content -LiteralPath $LogPath -Tail $TailLines) } else { @() }
$logItem = if ($exists) { Get-Item -LiteralPath $LogPath } else { $null }

$featureCoverage = [ordered]@{}
$stuckByBot = @{}
$stuckByTeam = @{}
$stuckZones = @{}
$weaponFired = 0
$towerFired = 0
$respawn = 0
$matchEnds = @()
$nextPending = @()
$buildFingerprints = @()

foreach ($line in $lines) {
    if ($line -match 'ArchonFactoryCanary: BotFeatureCoverage feature=(\S+) attempted=(\d+) succeeded=(\d+) skipped=(\d+) unavailable=(\d+)') {
        $featureCoverage[$Matches[1]] = [pscustomobject]@{
            attempted = [int]$Matches[2]
            succeeded = [int]$Matches[3]
            skipped = [int]$Matches[4]
            unavailable = [int]$Matches[5]
        }
        continue
    }
    if ($line -match 'ArchonFactoryCanary: BotStuckRecovery bot=(\d+) team=(\d+) attempt=(\d+) target=\((-?[0-9.]+),(-?[0-9.]+),(-?[0-9.]+)\)') {
        Add-Count $stuckByBot ("bot$($Matches[1])")
        Add-Count $stuckByTeam ("team$($Matches[2])")
        Add-Count $stuckZones (Get-ZoneKey ([double]$Matches[4]) ([double]$Matches[5]))
        continue
    }
    if ($line -match 'ArchonFactoryCanary: WeaponFired ') { $weaponFired++; continue }
    if ($line -match 'ArchonFactoryCanary: TowerFired ') { $towerFired++; continue }
    if ($line -match 'ArchonFactoryCanary: BotRespawned ') { $respawn++; continue }
    if ($line -match 'ArchonFactoryCanary: BuildFingerprint ') { $buildFingerprints += $line; continue }
    if ($line -match 'ArchonFactoryCanary: NextMatchPending nextMap=(\S+) countdownSeconds=([0-9.]+)') {
        $nextPending += [pscustomobject]@{
            NextMap = $Matches[1]
            CountdownSeconds = [double]$Matches[2]
            Line = $line
        }
        continue
    }
    if ($line -match 'ArchonFactoryCanary: MatchEnd winner=(\w+) reason=(\w+) pointsA=(\d+) pointsB=(\d+) liveSeconds=([0-9.]+) sitesA=(\d+) sitesB=(\d+)') {
        $matchEnds += [pscustomobject]@{
            Winner = $Matches[1]
            Reason = $Matches[2]
            PointsA = [int]$Matches[3]
            PointsB = [int]$Matches[4]
            LiveSeconds = [double]$Matches[5]
            SitesA = [int]$Matches[6]
            SitesB = [int]$Matches[7]
            Line = $line
        }
    }
}

$runningUnreal = @(Get-Process UnrealEditor -ErrorAction SilentlyContinue |
    Select-Object Id, ProcessName, MainWindowTitle, Path, StartTime)

$stuckCount = ($stuckByBot.Values | Measure-Object -Sum).Sum
if ($null -eq $stuckCount) { $stuckCount = 0 }

$read = @()
if ($stuckCount -gt 0) {
    $read += 'BotStuckRecovery is active in the live tail window; prioritize route/firing-lane fixes before adding blockers.'
}
if ($featureCoverage.Contains('take_firing_position') -and $featureCoverage['take_firing_position'].succeeded -gt 0) {
    $read += 'The live tail has take_firing_position coverage.'
} else {
    $read += 'The live tail has no take_firing_position coverage; current live DLL likely predates that C++ slice.'
}
if ($matchEnds.Count -eq 0 -and $nextPending.Count -eq 0) {
    $read += 'No match boundary appeared in this tail window.'
}

$result = [pscustomobject]@{
    ProjectRoot = $ProjectRoot
    LogPath = $LogPath
    SnapshotUtc = (Get-Date).ToUniversalTime().ToString('o')
    LogExists = $exists
    LogLastWriteTimeUtc = if ($logItem) { $logItem.LastWriteTimeUtc.ToString('o') } else { $null }
    TailLinesRequested = $TailLines
    TailLinesRead = $lines.Count
    RunningUnrealProcesses = $runningUnreal
    LatestBuildFingerprint = if ($buildFingerprints.Count -gt 0) { $buildFingerprints[-1] } else { $null }
    MatchEndCount = $matchEnds.Count
    LatestMatchEnd = if ($matchEnds.Count -gt 0) { $matchEnds[-1] } else { $null }
    NextMatchPendingCount = $nextPending.Count
    LatestNextMatchPending = if ($nextPending.Count -gt 0) { $nextPending[-1] } else { $null }
    WeaponFiredCount = $weaponFired
    TowerFiredCount = $towerFired
    BotRespawnedCount = $respawn
    BotStuckRecoveryCount = [int]$stuckCount
    TopBotStuckByBot = Get-TopCounts $stuckByBot 10
    TopBotStuckByTeam = Get-TopCounts $stuckByTeam 4
    TopBotStuckZones = Get-TopCounts $stuckZones 10
    LatestFeatureCoverage = $featureCoverage
    FeatureCoverageKeys = @($featureCoverage.Keys)
    Read = $read
}

$result | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $OutputPath -Encoding UTF8
$result | ConvertTo-Json -Depth 8
