param(
    [string]$ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..\..')).Path,
    [string]$LogPath = '',
    [int]$TailLines = 5000,
    [switch]$FullLog,
    [switch]$RequireBotEvidence
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

$proofDir = Join-Path $ProjectRoot 'Saved\Proof'
$jsonPath = Join-Path $proofDir 'last-live-bot-match-log-snapshot.json'
New-Item -ItemType Directory -Force -Path $proofDir | Out-Null

$logExists = Test-Path -LiteralPath $LogPath -PathType Leaf
$lines = @()
if ($logExists) {
    if ($FullLog -or $TailLines -le 0) {
        $lines = @(Get-Content -LiteralPath $LogPath)
    } else {
        $lines = @(Get-Content -LiteralPath $LogPath -Tail $TailLines)
    }
}
$text = if ($lines.Count -gt 0) { $lines -join "`n" } else { '' }
$logItem = if ($logExists) { Get-Item -LiteralPath $LogPath } else { $null }
$tuningPath = Join-Path $ProjectRoot 'games\splitroot\FactoryContracts\bot_strategy_tuning.json'
$tuningExists = Test-Path -LiteralPath $tuningPath -PathType Leaf
$tuning = $null
$tuningReadError = $null
if ($tuningExists) {
    try {
        $tuning = Get-Content -LiteralPath $tuningPath -Raw | ConvertFrom-Json
    } catch {
        $tuningReadError = $_.Exception.Message
    }
}

$featureCoverageMatches = [regex]::Matches(
    $text,
    'ArchonFactoryCanary: BotFeatureCoverage feature=(\S+) attempted=(\d+) succeeded=(\d+) skipped=(\d+) unavailable=(\d+)')
$featureCoverage = [ordered]@{}
foreach ($match in $featureCoverageMatches) {
    $feature = $match.Groups[1].Value
    $featureCoverage[$feature] = [pscustomobject]@{
        attempted = [int]$match.Groups[2].Value
        succeeded = [int]$match.Groups[3].Value
        skipped = [int]$match.Groups[4].Value
        unavailable = [int]$match.Groups[5].Value
    }
}

function Get-LastMatchValue([string]$Pattern) {
    $matches = [regex]::Matches($text, $Pattern)
    if ($matches.Count -eq 0) {
        return $null
    }
    return $matches[$matches.Count - 1].Value
}

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

$nativePerceptionMatches = [regex]::Matches($text, 'ArchonFactoryCanary: BotNativePerception bot=')
$fallbackPerceptionMatches = [regex]::Matches($text, 'ArchonFactoryCanary: BotFallbackPerception bot=')
$controllerMatches = [regex]::Matches($text, 'ArchonFactoryCanary: BotAIControllerConfigured bot=')
$configuredMatches = [regex]::Matches($text, 'ArchonFactoryCanary: BotConfigured bot=(\d+) team=(\d+) role=(\S+) engageRange=([0-9.]+)(?: pursuitMemory=([0-9.]+) threatWindow=([0-9.]+))? tuningRevision=(\S+)')
$weaponFireMatches = [regex]::Matches($text, 'ArchonFactoryCanary: WeaponFired weapon=')
$towerFireMatches = [regex]::Matches($text, 'ArchonFactoryCanary: TowerFired team=')
$respawnMatches = [regex]::Matches($text, 'ArchonFactoryCanary: BotRespawned bot=')
$stuckMatches = [regex]::Matches($text, 'ArchonFactoryCanary: BotStuckRecovery bot=')
$stuckContextMatches = [regex]::Matches($text, 'ArchonFactoryCanary: BotStuckRecoveryContext bot=')
$unstickQueryMatches = [regex]::Matches($text, 'ArchonFactoryCanary: BotUnstickQuery bot=')
$clearUnstickQueryMatches = [regex]::Matches($text, 'ArchonFactoryCanary: BotUnstickQuery[^\r\n]+selected=clear')
$fallbackUnstickQueryMatches = [regex]::Matches($text, 'ArchonFactoryCanary: BotUnstickQuery[^\r\n]+selected=fallback')
$routeWaypointAbandonedMatches = [regex]::Matches($text, 'ArchonFactoryCanary: BotRouteWaypointAbandoned bot=')
$objectiveLaneShiftMatches = [regex]::Matches($text, 'ArchonFactoryCanary: BotObjectiveLaneShift bot=')
$firingPositionMatches = [regex]::Matches($text, 'ArchonFactoryCanary: BotFiringPosition bot=')
$strategyTuningLoadedMatches = [regex]::Matches($text, 'ArchonFactoryCanary: BotStrategyTuningLoaded[^\r\n]+')
$latestBotStrategyTuningLoaded = Get-LastMatchValue 'ArchonFactoryCanary: BotStrategyTuningLoaded[^\r\n]+'
$latestLoadedTuningRevision = $null
if ($latestBotStrategyTuningLoaded -and $latestBotStrategyTuningLoaded -match 'revision=(\S+)') {
    $latestLoadedTuningRevision = $Matches[1]
}
$localTuningRevision = if ($tuning) { [string]$tuning.revision } else { $null }
$tuningRevisionAdopted = if ([string]::IsNullOrWhiteSpace($localTuningRevision) -or [string]::IsNullOrWhiteSpace($latestLoadedTuningRevision)) {
    $null
} else {
    $localTuningRevision -eq $latestLoadedTuningRevision
}

$configuredByRole = @{}
$configuredBreacherFocusCount = 0
foreach ($match in $configuredMatches) {
    $role = $match.Groups[3].Value
    Add-Count $configuredByRole $role
    if ($role -eq 'breacher' -and
        $match.Groups[5].Success -and
        $match.Groups[6].Success -and
        [double]$match.Groups[5].Value -le 2.5 -and
        [double]$match.Groups[6].Value -le 3.5) {
        $configuredBreacherFocusCount++
    }
}

$stuckContextByPhaseTargetLaneShift = @{}
$objectiveStuckContextByLaneShift = @{}
foreach ($match in [regex]::Matches($text, 'ArchonFactoryCanary: BotStuckRecoveryContext bot=(\d+) team=(\d+) attempt=(\d+) phase=(\S+) routeActive=(\d+) routeReached=(\d+) routeStallAttempts=(\d+) routeThreshold=(\d+) objectiveTarget=(\S+) objectiveStallAttempts=(\d+) laneShift=(\d+) distance=(-?[0-9.]+)')) {
    $phase = $match.Groups[4].Value
    $objectiveTarget = $match.Groups[9].Value
    $laneShift = $match.Groups[11].Value
    Add-Count $stuckContextByPhaseTargetLaneShift ("phase=$phase|target=$objectiveTarget|laneShift=$laneShift")
    if ($phase -eq 'objective') {
        Add-Count $objectiveStuckContextByLaneShift ("laneShift=$laneShift")
    }
}

$firingPositionByTargetLaneShift = @{}
foreach ($match in [regex]::Matches($text, 'ArchonFactoryCanary: BotFiringPosition bot=(\d+) team=(\d+) target=(\S+) location=\((-?[0-9.]+),(-?[0-9.]+),(-?[0-9.]+)\) standOff=(-?[0-9.]+) laneSpacing=(-?[0-9.]+) laneShift=(\d+)')) {
    $target = $match.Groups[3].Value
    $laneSpacing = $match.Groups[8].Value
    $laneShift = $match.Groups[9].Value
    Add-Count $firingPositionByTargetLaneShift ("target=$target|laneShift=$laneShift|laneSpacing=$laneSpacing")
}

$objectiveLaneShiftByTargetLane = @{}
foreach ($match in [regex]::Matches($text, 'ArchonFactoryCanary: BotObjectiveLaneShift bot=(\d+) team=(\d+) target=(\S+) attempts=(\d+) laneShift=(\d+)')) {
    Add-Count $objectiveLaneShiftByTargetLane ("target=$($match.Groups[3].Value)|laneShift=$($match.Groups[5].Value)")
}

$takeFiringPositionCovered =
    $featureCoverage.Contains('take_firing_position') -and
    $featureCoverage['take_firing_position'].succeeded -gt 0

$coreFeatureKeys = @('perceive_enemy', 'march_objective', 'respawn', 'take_firing_position')
$movementFeatureKeys = @('unstick_reposition', 'route_waypoint_abandon', 'objective_lane_shift')
$objectiveFeatureKeys = @('attack_tower', 'attack_core', 'capture_site', 'react_to_base_attack')
$succeededFeatureKeys = @($featureCoverage.Keys | Where-Object { $featureCoverage[$_].succeeded -gt 0 })
$missingCoreFeatureKeys = @($coreFeatureKeys | Where-Object { $succeededFeatureKeys -notcontains $_ })
$missingMovementFeatureKeys = @($movementFeatureKeys | Where-Object { $succeededFeatureKeys -notcontains $_ })
$missingObjectiveFeatureKeys = @($objectiveFeatureKeys | Where-Object { $succeededFeatureKeys -notcontains $_ })
$featureCoverageCanaryStatus = if ($featureCoverage.Count -eq 0) {
    'no_feature_coverage'
} elseif ($weaponFireMatches.Count -lt 200 -and $towerFireMatches.Count -eq 0) {
    'observe_more'
} elseif ($missingCoreFeatureKeys.Count -gt 0) {
    'missing_core_features'
} else {
    'core_features_covered'
}

$runningUnreal = @(Get-Process UnrealEditor,UnrealEditor-Cmd,ArchonFactoryCanary -ErrorAction SilentlyContinue |
    Select-Object Id, ProcessName, MainWindowTitle, Path, StartTime)

$result = [pscustomobject]@{
    ProjectRoot = $ProjectRoot
    LogPath = $LogPath
    SnapshotUtc = (Get-Date).ToUniversalTime().ToString('o')
    LogExists = $logExists
    LogLastWriteTimeUtc = if ($logItem) { $logItem.LastWriteTimeUtc.ToString('o') } else { $null }
    ReadMode = if ($FullLog -or $TailLines -le 0) { 'full_log' } else { 'tail' }
    TailLinesRequested = if ($FullLog -or $TailLines -le 0) { $null } else { $TailLines }
    LinesRead = $lines.Count
    CountsScope = if ($FullLog -or $TailLines -le 0) { 'full_log' } else { 'tail_window' }
    RunningUnrealProcesses = $runningUnreal
    BotStrategyTuningPath = $tuningPath
    BotStrategyTuningExists = $tuningExists
    BotStrategyTuningReadError = $tuningReadError
    BotStrategyTuningRevision = $localTuningRevision
    BotStrategyRoleCycle = if ($tuning -and $tuning.roles) { @($tuning.roles.role_cycle) } else { @() }
    BotStrategyFighterEngageRange = if ($tuning -and $tuning.roles) { $tuning.roles.fighter_engage_range } else { $null }
    BotStrategyBreacherEngageRange = if ($tuning -and $tuning.roles) { $tuning.roles.breacher_engage_range } else { $null }
    BotStrategyBreacherPursuitMemoryWindowSeconds = if ($tuning -and $tuning.roles) { $tuning.roles.breacher_pursuit_memory_window_seconds } else { $null }
    BotStrategyBreacherThreatWindowSeconds = if ($tuning -and $tuning.roles) { $tuning.roles.breacher_threat_window_seconds } else { $null }
    BotStrategyUnstickLateralDistance = if ($tuning -and $tuning.movement) { $tuning.movement.unstick_lateral_distance } else { $null }
    BotStrategyUnstickBackstepDistance = if ($tuning -and $tuning.movement) { $tuning.movement.unstick_backstep_distance } else { $null }
    BotStrategyRouteStallMaxUnstickAttempts = if ($tuning -and $tuning.movement) { $tuning.movement.route_stall_max_unstick_attempts } else { $null }
    BotStrategyObjectiveStallLaneShiftAttempts = if ($tuning -and $tuning.movement) { $tuning.movement.objective_stall_lane_shift_attempts } else { $null }
    BotStrategyObjectiveStallMaxLaneShift = if ($tuning -and $tuning.movement) { $tuning.movement.objective_stall_max_lane_shift } else { $null }
    BotStrategyTuningLoadedCount = $strategyTuningLoadedMatches.Count
    MatchLoopActive = $text -match 'ArchonFactoryCanary: MatchLoopActive sites='
    BotMatchSpawned = $text -match 'ArchonFactoryCanary: BotMatchSpawned perTeam='
    NativeBotControllerConfiguredCount = $controllerMatches.Count
    BotConfiguredCount = $configuredMatches.Count
    BotConfiguredRoleCounts = @(Get-TopCounts $configuredByRole 4)
    BotConfiguredBreacherFocusCount = $configuredBreacherFocusCount
    NativeBotPerceptionCount = $nativePerceptionMatches.Count
    FallbackPerceptionCount = $fallbackPerceptionMatches.Count
    WeaponFiredCount = $weaponFireMatches.Count
    TowerFiredCount = $towerFireMatches.Count
    BotRespawnedCount = $respawnMatches.Count
    BotStuckRecoveryCount = $stuckMatches.Count
    BotStuckRecoveryContextCount = $stuckContextMatches.Count
    BotUnstickQueryCount = $unstickQueryMatches.Count
    BotUnstickQueryClearCount = $clearUnstickQueryMatches.Count
    BotUnstickQueryFallbackCount = $fallbackUnstickQueryMatches.Count
    BotUnstickQueryClearRate = if ($unstickQueryMatches.Count -gt 0) { [math]::Round($clearUnstickQueryMatches.Count / [double]$unstickQueryMatches.Count, 3) } else { $null }
    BotRouteWaypointAbandonedCount = $routeWaypointAbandonedMatches.Count
    BotObjectiveLaneShiftCount = $objectiveLaneShiftMatches.Count
    BotFiringPositionCount = $firingPositionMatches.Count
    TopStuckRecoveryContextByPhaseTargetLaneShift = @(Get-TopCounts $stuckContextByPhaseTargetLaneShift 12)
    TopObjectiveStuckContextByLaneShift = @(Get-TopCounts $objectiveStuckContextByLaneShift 8)
    TopBotFiringPositionByTargetLaneShift = @(Get-TopCounts $firingPositionByTargetLaneShift 12)
    TopObjectiveLaneShiftByTargetLane = @(Get-TopCounts $objectiveLaneShiftByTargetLane 8)
    TakeFiringPositionCovered = $takeFiringPositionCovered
    LatestBuildFingerprint = Get-LastMatchValue 'ArchonFactoryCanary: BuildFingerprint[^\r\n]+'
    LatestBotStrategyTuningLoaded = $latestBotStrategyTuningLoaded
    LatestBotStrategyTuningLoadedRevision = $latestLoadedTuningRevision
    BotStrategyTuningRevisionAdopted = $tuningRevisionAdopted
    LatestBotConfigured = Get-LastMatchValue 'ArchonFactoryCanary: BotConfigured[^\r\n]+'
    LatestBotMatchSpawned = Get-LastMatchValue 'ArchonFactoryCanary: BotMatchSpawned[^\r\n]+'
    LatestNativePerception = Get-LastMatchValue 'ArchonFactoryCanary: BotNativePerception[^\r\n]+'
    LatestFallbackPerception = Get-LastMatchValue 'ArchonFactoryCanary: BotFallbackPerception[^\r\n]+'
    LatestUnstickQuery = Get-LastMatchValue 'ArchonFactoryCanary: BotUnstickQuery[^\r\n]+'
    LatestBotStuckRecoveryContext = Get-LastMatchValue 'ArchonFactoryCanary: BotStuckRecoveryContext[^\r\n]+'
    LatestRouteWaypointAbandoned = Get-LastMatchValue 'ArchonFactoryCanary: BotRouteWaypointAbandoned[^\r\n]+'
    LatestObjectiveLaneShift = Get-LastMatchValue 'ArchonFactoryCanary: BotObjectiveLaneShift[^\r\n]+'
    LatestBotFiringPosition = Get-LastMatchValue 'ArchonFactoryCanary: BotFiringPosition[^\r\n]+'
    LatestFeatureCoverage = $featureCoverage
    FeatureCoverageKeys = @($featureCoverage.Keys)
    BotFeatureCoverageCanary = [pscustomobject]@{
        Status = $featureCoverageCanaryStatus
        CoreExpected = @($coreFeatureKeys)
        MovementExpected = @($movementFeatureKeys)
        ObjectiveExpected = @($objectiveFeatureKeys)
        Succeeded = @($succeededFeatureKeys)
        MissingCore = @($missingCoreFeatureKeys)
        MissingMovement = @($missingMovementFeatureKeys)
        MissingObjective = @($missingObjectiveFeatureKeys)
    }
    EvidenceReady = $logExists -and
        $controllerMatches.Count -gt 0 -and
        $nativePerceptionMatches.Count -gt 0 -and
        $weaponFireMatches.Count -gt 0 -and
        $respawnMatches.Count -gt 0
}

$result | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $jsonPath -Encoding UTF8
$result | ConvertTo-Json -Depth 8

if ($RequireBotEvidence -and -not $result.EvidenceReady) {
    exit 1
}
