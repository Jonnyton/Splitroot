param(
    [string]$ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..\..')).Path,
    [string]$LogPath = '',
    [int]$TailLines = 10000,
    [string]$OutputPath = '',
    [string]$HistoryPath = ''
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
    $OutputPath = Join-Path $ProjectRoot 'Saved\Proof\last-vector-bot-replay-diagnosis.json'
}
if (-not $HistoryPath) {
    $HistoryPath = Join-Path $ProjectRoot 'Saved\Proof\vector-bot-replay-diagnosis-history.jsonl'
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

function Get-CountValue([hashtable]$Table, [string]$Key) {
    if (-not $Table.ContainsKey($Key)) { return 0 }
    return [int]$Table[$Key]
}

function Get-ZoneKey([double]$X, [double]$Y, [double]$CellSize = 500.0) {
    $zoneX = [math]::Round($X / $CellSize) * $CellSize
    $zoneY = [math]::Round($Y / $CellSize) * $CellSize
    'x={0};y={1}' -f [int]$zoneX, [int]$zoneY
}

function Get-RoleForBot([int]$BotIndex, [object[]]$RoleCycle) {
    if (-not $RoleCycle -or $RoleCycle.Count -eq 0) {
        $RoleCycle = @('fighter', 'fighter', 'breacher')
    }
    $index = [math]::Abs($BotIndex) % $RoleCycle.Count
    $role = [string]$RoleCycle[$index]
    if ([string]::IsNullOrWhiteSpace($role)) { return 'fighter' }
    return $role.ToLowerInvariant()
}

function Get-TuningMovementInt([object]$Tuning, [string]$Name, [int]$DefaultValue) {
    if (-not $Tuning -or -not $Tuning.movement) {
        return $DefaultValue
    }
    $property = $Tuning.movement.PSObject.Properties[$Name]
    if (-not $property -or $null -eq $property.Value) {
        return $DefaultValue
    }
    return [int]$property.Value
}

function New-BotMetric([int]$BotIndex, [int]$TeamId, [object[]]$RoleCycle) {
    [pscustomobject]@{
        Bot = $BotIndex
        Team = $TeamId
        Role = Get-RoleForBot $BotIndex $RoleCycle
        StuckCount = 0
        MaxStuckAttempt = 0
        LatestStuckTarget = $null
        RouteWaypointAbandonedCount = 0
        ObjectiveLaneShiftCount = 0
        NativePerceptionCount = 0
        FallbackPerceptionCount = 0
        RespawnCount = 0
        StateCount = 0
        TopStates = @()
        TopStuckZones = @()
    }
}

$logExists = Test-Path -LiteralPath $LogPath -PathType Leaf
$lines = if ($logExists) { @(Get-Content -LiteralPath $LogPath -Tail $TailLines) } else { @() }
$logItem = if ($logExists) { Get-Item -LiteralPath $LogPath } else { $null }

$tuningPath = Join-Path $ProjectRoot 'games\splitroot\FactoryContracts\bot_strategy_tuning.json'
$tuning = $null
$tuningReadError = $null
if (Test-Path -LiteralPath $tuningPath -PathType Leaf) {
    try {
        $tuning = Get-Content -LiteralPath $tuningPath -Raw | ConvertFrom-Json
    } catch {
        $tuningReadError = $_.Exception.Message
    }
}
$roleCycle = if ($tuning -and $tuning.roles) { @($tuning.roles.role_cycle) } else { @('fighter', 'fighter', 'breacher') }
$routeStallMaxUnstickAttempts = Get-TuningMovementInt $tuning 'route_stall_max_unstick_attempts' 8
$objectiveStallLaneShiftAttempts = Get-TuningMovementInt $tuning 'objective_stall_lane_shift_attempts' 6
$objectiveStallMaxLaneShift = Get-TuningMovementInt $tuning 'objective_stall_max_lane_shift' 3

$stuckByBot = @{}
$stuckByTeam = @{}
$stuckByRole = @{}
$stuckZones = @{}
$stuckZonesByBot = @{}
$maxAttemptByBot = @{}
$stateByName = @{}
$stateByBot = @{}
$stateByRole = @{}
$configuredByRole = @{}
$configuredByRevision = @{}
$nativeByBot = @{}
$fallbackByBot = @{}
$nativeSense = @{}
$respawnByBot = @{}
$structureHitByTargetTeam = @{}
$routeWaypointAbandonedByBot = @{}
$objectiveLaneShiftByBot = @{}
$objectiveLaneShiftMaxByBot = @{}
$unstickSelection = @{}
$unstickByBot = @{}
$unstickSelectionByBot = @{}
$unstickSelectionByRole = @{}
$unstickAttemptMaxByBot = @{}
$stuckRecoveryContext = @()
$stuckContextByPhase = @{}
$stuckContextByPhaseRole = @{}
$stuckContextByObjectiveTarget = @{}
$stuckContextByPhaseTargetLaneShift = @{}
$stuckContextByLaneShift = @{}
$objectiveStuckContextByLaneShift = @{}
$featureCoverage = [ordered]@{}
$firingPositionByTargetLaneShift = @{}
$firingPositionByLaneShift = @{}
$objectiveLaneShiftByTarget = @{}
$objectiveLaneShiftByTargetLane = @{}

$weaponFired = 0
$towerFired = 0
$botStrategyLoaded = @()
$botFiringPosition = @()
$botUnstickQuery = @()
$botConfigured = @()
$botConfiguredBreacherFocusCount = 0
$routeWaypointAbandoned = @()
$objectiveLaneShift = @()
$matchEnds = @()
$buildFingerprints = @()

foreach ($line in $lines) {
    if ($line -match 'ArchonFactoryCanary: BotStrategyTuningLoaded[^\r\n]+') {
        $botStrategyLoaded += $Matches[0]
        continue
    }
    if ($line -match 'ArchonFactoryCanary: BuildFingerprint[^\r\n]+') {
        $buildFingerprints += $Matches[0]
        continue
    }
    if ($line -match 'ArchonFactoryCanary: BotFeatureCoverage feature=(\S+) attempted=(\d+) succeeded=(\d+) skipped=(\d+) unavailable=(\d+)') {
        $featureCoverage[$Matches[1]] = [pscustomobject]@{
            attempted = [int]$Matches[2]
            succeeded = [int]$Matches[3]
            skipped = [int]$Matches[4]
            unavailable = [int]$Matches[5]
        }
        continue
    }
    if ($line -match 'ArchonFactoryCanary: BotConfigured bot=(\d+) team=(\d+) role=(\S+) engageRange=([0-9.]+)(?: pursuitMemory=([0-9.]+) threatWindow=([0-9.]+))? tuningRevision=(\S+)') {
        $bot = [int]$Matches[1]
        $team = [int]$Matches[2]
        $role = $Matches[3]
        $pursuitMemory = if ($Matches[5]) { [double]$Matches[5] } else { $null }
        $threatWindow = if ($Matches[6]) { [double]$Matches[6] } else { $null }
        $revision = $Matches[7]
        $botConfigured += [pscustomobject]@{
            Bot = $bot
            Team = $team
            Role = $role
            EngageRange = [double]$Matches[4]
            PursuitMemory = $pursuitMemory
            ThreatWindow = $threatWindow
            Revision = $revision
        }
        Add-Count $configuredByRole $role
        Add-Count $configuredByRevision $revision
        if ($role -eq 'breacher' -and
            $null -ne $pursuitMemory -and
            $null -ne $threatWindow -and
            $pursuitMemory -le 2.5 -and
            $threatWindow -le 3.5) {
            $botConfiguredBreacherFocusCount++
        }
        continue
    }
    if ($line -match 'ArchonFactoryCanary: BotState bot=(\d+) team=(\d+) state=(\S+) detail=([^\r\n]+)') {
        $bot = [int]$Matches[1]
        $team = [int]$Matches[2]
        $state = $Matches[3]
        $role = Get-RoleForBot $bot $roleCycle
        Add-Count $stateByName $state
        Add-Count $stateByBot ("bot$bot|team$team|state=$state")
        Add-Count $stateByRole ("role=$role|state=$state")
        continue
    }
    if ($line -match 'ArchonFactoryCanary: BotStuckRecovery bot=(\d+) team=(\d+) attempt=(\d+) target=\((-?[0-9.]+),(-?[0-9.]+),(-?[0-9.]+)\)') {
        $bot = [int]$Matches[1]
        $team = [int]$Matches[2]
        $attempt = [int]$Matches[3]
        $x = [double]$Matches[4]
        $y = [double]$Matches[5]
        $z = [double]$Matches[6]
        $role = Get-RoleForBot $bot $roleCycle
        $zone = Get-ZoneKey $x $y
        Add-Count $stuckByBot ("bot$bot|team$team|role=$role")
        Add-Count $stuckByTeam ("team$team")
        Add-Count $stuckByRole $role
        Add-Count $stuckZones $zone
        Add-Count $stuckZonesByBot ("bot$bot|team$team|zone=$zone")
        if (-not $maxAttemptByBot.ContainsKey($bot) -or $attempt -gt $maxAttemptByBot[$bot].Attempt) {
            $maxAttemptByBot[$bot] = [pscustomobject]@{
                Bot = $bot
                Team = $team
                Role = $role
                Attempt = $attempt
                Target = [pscustomobject]@{ X = $x; Y = $y; Z = $z }
                Zone = $zone
            }
        }
        continue
    }
    if ($line -match 'ArchonFactoryCanary: BotNativePerception bot=(\d+) team=(\d+) target=\S+ sense=(\S+)') {
        $bot = [int]$Matches[1]
        $team = [int]$Matches[2]
        $sense = $Matches[3]
        Add-Count $nativeByBot ("bot$bot|team$team")
        Add-Count $nativeSense $sense
        continue
    }
    if ($line -match 'ArchonFactoryCanary: BotFallbackPerception bot=(\d+) team=(\d+)') {
        $bot = [int]$Matches[1]
        $team = [int]$Matches[2]
        Add-Count $fallbackByBot ("bot$bot|team$team")
        continue
    }
    if ($line -match 'ArchonFactoryCanary: BotStuckRecoveryContext bot=(\d+) team=(\d+) attempt=(\d+) phase=(\S+) routeActive=(\d+) routeReached=(\d+) routeStallAttempts=(\d+) routeThreshold=(\d+) objectiveTarget=(\S+) objectiveStallAttempts=(\d+) laneShift=(\d+) distance=(-?[0-9.]+)') {
        $bot = [int]$Matches[1]
        $team = [int]$Matches[2]
        $phase = $Matches[4]
        $objectiveTarget = $Matches[9]
        $role = Get-RoleForBot $bot $roleCycle
        $stuckRecoveryContext += [pscustomobject]@{
            Bot = $bot
            Team = $team
            Role = $role
            Attempt = [int]$Matches[3]
            Phase = $phase
            RouteActive = [bool]([int]$Matches[5])
            RouteReached = [bool]([int]$Matches[6])
            RouteStallAttempts = [int]$Matches[7]
            RouteThreshold = [int]$Matches[8]
            ObjectiveTarget = $objectiveTarget
            ObjectiveStallAttempts = [int]$Matches[10]
            LaneShift = [int]$Matches[11]
            Distance = [double]$Matches[12]
        }
        Add-Count $stuckContextByPhase $phase
        Add-Count $stuckContextByPhaseRole ("phase=$phase|role=$role")
        Add-Count $stuckContextByObjectiveTarget ("phase=$phase|target=$objectiveTarget")
        Add-Count $stuckContextByPhaseTargetLaneShift ("phase=$phase|target=$objectiveTarget|laneShift=$($Matches[11])")
        Add-Count $stuckContextByLaneShift ("laneShift=$($Matches[11])")
        if ($phase -eq 'objective') {
            Add-Count $objectiveStuckContextByLaneShift ("laneShift=$($Matches[11])")
        }
        continue
    }
    if ($line -match 'ArchonFactoryCanary: BotRespawned bot=(\d+) team=(\d+)') {
        $bot = [int]$Matches[1]
        $team = [int]$Matches[2]
        Add-Count $respawnByBot ("bot$bot|team$team")
        continue
    }
    if ($line -match 'ArchonFactoryCanary: BotStructureHit bot=\d+ team=\d+ targetTeam=(\d+)') {
        Add-Count $structureHitByTargetTeam ("targetTeam$($Matches[1])")
        continue
    }
    if ($line -match 'ArchonFactoryCanary: BotFiringPosition bot=(\d+) team=(\d+) target=(\S+) location=\((-?[0-9.]+),(-?[0-9.]+),(-?[0-9.]+)\) standOff=(-?[0-9.]+) laneSpacing=(-?[0-9.]+) laneShift=(\d+)') {
        $bot = [int]$Matches[1]
        $team = [int]$Matches[2]
        $target = $Matches[3]
        $laneShift = [int]$Matches[9]
        $botFiringPosition += [pscustomobject]@{
            Bot = $bot
            Team = $team
            Role = Get-RoleForBot $bot $roleCycle
            Target = $target
            Location = [pscustomobject]@{
                X = [double]$Matches[4]
                Y = [double]$Matches[5]
                Z = [double]$Matches[6]
            }
            StandOff = [double]$Matches[7]
            LaneSpacing = [double]$Matches[8]
            LaneShift = $laneShift
            Raw = $Matches[0]
        }
        Add-Count $firingPositionByTargetLaneShift ("target=$target|laneShift=$laneShift|laneSpacing=$($Matches[8])")
        Add-Count $firingPositionByLaneShift ("laneShift=$laneShift")
        continue
    }
    if ($line -match 'ArchonFactoryCanary: BotFiringPosition[^\r\n]+') {
        $botFiringPosition += [pscustomobject]@{
            Bot = $null
            Team = $null
            Role = 'unknown'
            Target = 'unknown'
            Location = $null
            StandOff = $null
            LaneSpacing = $null
            LaneShift = $null
            Raw = $Matches[0]
        }
        continue
    }
    if ($line -match 'ArchonFactoryCanary: BotUnstickQuery bot=(\d+) team=(\d+) attempt=(\d+) candidates=(\d+) selected=(\S+) target=\((-?[0-9.]+),(-?[0-9.]+),(-?[0-9.]+)\)') {
        $bot = [int]$Matches[1]
        $team = [int]$Matches[2]
        $attempt = [int]$Matches[3]
        $candidateCount = [int]$Matches[4]
        $selection = $Matches[5].ToLowerInvariant()
        $x = [double]$Matches[6]
        $y = [double]$Matches[7]
        $z = [double]$Matches[8]
        $role = Get-RoleForBot $bot $roleCycle
        $zone = Get-ZoneKey $x $y
        $botKey = "bot$bot|team$team|role=$role"
        $botUnstickQuery += [pscustomobject]@{
            Bot = $bot
            Team = $team
            Role = $role
            Attempt = $attempt
            CandidateCount = $candidateCount
            Selection = $selection
            Target = [pscustomobject]@{ X = $x; Y = $y; Z = $z }
            Zone = $zone
            Raw = $Matches[0]
        }
        Add-Count $unstickSelection $selection
        Add-Count $unstickByBot $botKey
        Add-Count $unstickSelectionByBot ("$botKey|selected=$selection")
        Add-Count $unstickSelectionByRole ("role=$role|selected=$selection")
        if (-not $unstickAttemptMaxByBot.ContainsKey($botKey) -or $attempt -gt $unstickAttemptMaxByBot[$botKey].Attempt) {
            $unstickAttemptMaxByBot[$botKey] = [pscustomobject]@{
                Bot = $bot
                Team = $team
                Role = $role
                Attempt = $attempt
                Selection = $selection
                CandidateCount = $candidateCount
                Target = [pscustomobject]@{ X = $x; Y = $y; Z = $z }
                Zone = $zone
            }
        }
        continue
    }
    if ($line -match 'ArchonFactoryCanary: BotUnstickQuery[^\r\n]+') {
        $botUnstickQuery += [pscustomobject]@{
            Bot = $null
            Team = $null
            Role = 'unknown'
            Attempt = $null
            CandidateCount = $null
            Selection = 'unknown'
            Target = $null
            Zone = 'unknown'
            Raw = $Matches[0]
        }
        Add-Count $unstickSelection 'unknown'
        continue
    }
    if ($line -match 'ArchonFactoryCanary: BotRouteWaypointAbandoned bot=(\d+) team=(\d+) attempts=(\d+) distance=(-?[0-9.]+) waypoint=\((-?[0-9.]+),(-?[0-9.]+),(-?[0-9.]+)\)') {
        $bot = [int]$Matches[1]
        $team = [int]$Matches[2]
        $attempts = [int]$Matches[3]
        $distance = [double]$Matches[4]
        $routeWaypointAbandoned += [pscustomobject]@{
            Bot = $bot
            Team = $team
            Attempts = $attempts
            Distance = $distance
            Waypoint = [pscustomobject]@{
                X = [double]$Matches[5]
                Y = [double]$Matches[6]
                Z = [double]$Matches[7]
            }
        }
        Add-Count $routeWaypointAbandonedByBot ("bot$bot|team$team")
        continue
    }
    if ($line -match 'ArchonFactoryCanary: BotObjectiveLaneShift bot=(\d+) team=(\d+) target=(\S+) attempts=(\d+) laneShift=(\d+)') {
        $bot = [int]$Matches[1]
        $team = [int]$Matches[2]
        $target = $Matches[3]
        $objectiveLaneShift += [pscustomobject]@{
            Bot = $bot
            Team = $team
            Target = $target
            Attempts = [int]$Matches[4]
            LaneShift = [int]$Matches[5]
        }
        Add-Count $objectiveLaneShiftByBot ("bot$bot|team$team|target=$target")
        Add-Count $objectiveLaneShiftByTarget $target
        Add-Count $objectiveLaneShiftByTargetLane ("target=$target|laneShift=$($Matches[5])")
        $laneShiftBotKey = "bot$bot|team$team|target=$target"
        if (-not $objectiveLaneShiftMaxByBot.ContainsKey($laneShiftBotKey) -or [int]$Matches[5] -gt $objectiveLaneShiftMaxByBot[$laneShiftBotKey].LaneShift) {
            $objectiveLaneShiftMaxByBot[$laneShiftBotKey] = $objectiveLaneShift[-1]
        }
        continue
    }
    if ($line -match 'ArchonFactoryCanary: WeaponFired ') { $weaponFired++; continue }
    if ($line -match 'ArchonFactoryCanary: TowerFired ') { $towerFired++; continue }
    if ($line -match 'ArchonFactoryCanary: MatchEnd[^\r\n]+') { $matchEnds += $Matches[0]; continue }
}

$stuckTotal = ($stuckByBot.Values | Measure-Object -Sum).Sum
if ($null -eq $stuckTotal) { $stuckTotal = 0 }
$nativeTotal = ($nativeByBot.Values | Measure-Object -Sum).Sum
if ($null -eq $nativeTotal) { $nativeTotal = 0 }
$fallbackTotal = ($fallbackByBot.Values | Measure-Object -Sum).Sum
if ($null -eq $fallbackTotal) { $fallbackTotal = 0 }
$respawnTotal = ($respawnByBot.Values | Measure-Object -Sum).Sum
if ($null -eq $respawnTotal) { $respawnTotal = 0 }

$maxAttemptRows = @($maxAttemptByBot.Values | Sort-Object -Property @{ Expression = 'Attempt'; Descending = $true }, @{ Expression = 'Bot'; Ascending = $true } | Select-Object -First 10)
$maxUnstickAttemptRows = @($unstickAttemptMaxByBot.Values | Sort-Object -Property @{ Expression = 'Attempt'; Descending = $true }, @{ Expression = 'Bot'; Ascending = $true } | Select-Object -First 10)
$topStuckZones = Get-TopCounts $stuckZones 10
$maxObservedStuckAttempt = if ($maxAttemptRows.Count -gt 0) { [int]$maxAttemptRows[0].Attempt } else { 0 }
$hasCombatWindow = $weaponFired -gt 0 -or $towerFired -gt 0 -or $nativeTotal -gt 0 -or $fallbackTotal -gt 0 -or $featureCoverage.Count -gt 0
$botUnstickQueryClearCount = Get-CountValue $unstickSelection 'clear'
$botUnstickQueryFallbackCount = Get-CountValue $unstickSelection 'fallback'
$botUnstickQueryClearRate = if ($botUnstickQuery.Count -gt 0) { [math]::Round($botUnstickQueryClearCount / [double]$botUnstickQuery.Count, 3) } else { $null }
$fighterStuckCount = Get-CountValue $stuckByRole 'fighter'
$breacherStuckCount = Get-CountValue $stuckByRole 'breacher'
$fighterStuckDominant = $fighterStuckCount -ge 20 -and $fighterStuckCount -ge [math]::Ceiling([math]::Max(1, $breacherStuckCount) * 1.5)
$routeContextCount = Get-CountValue $stuckContextByPhase 'route'
$objectiveContextCount = Get-CountValue $stuckContextByPhase 'objective'
$moveOverrideContextCount = Get-CountValue $stuckContextByPhase 'move_override'
$objectiveContextDominant = $objectiveContextCount -ge 10 -and $objectiveContextCount -gt $routeContextCount -and $objectiveContextCount -gt $moveOverrideContextCount
$objectiveContextLaneZeroCount = Get-CountValue $objectiveStuckContextByLaneShift 'laneShift=0'
$objectiveContextShiftedCount = [math]::Max(0, $objectiveContextCount - $objectiveContextLaneZeroCount)
$objectiveContextLaneZeroDominant = $objectiveContextCount -ge 10 -and $objectiveContextLaneZeroCount -gt $objectiveContextShiftedCount
$objectiveContextAtMaxLaneShiftCount = 0
foreach ($context in $stuckRecoveryContext) {
    if ($context.Phase -eq 'objective' -and [int]$context.LaneShift -ge $objectiveStallMaxLaneShift -and $objectiveStallMaxLaneShift -gt 0) {
        $objectiveContextAtMaxLaneShiftCount++
    }
}
$localTuningRevision = if ($tuning) { [string]$tuning.revision } else { $null }
$latestLoadedTuningRevision = $null
if ($botStrategyLoaded.Count -gt 0 -and [string]$botStrategyLoaded[-1] -match 'revision=(\S+)') {
    $latestLoadedTuningRevision = $Matches[1]
}
$tuningRevisionAdopted = if ([string]::IsNullOrWhiteSpace($localTuningRevision) -or [string]::IsNullOrWhiteSpace($latestLoadedTuningRevision)) {
    $null
} else {
    $localTuningRevision -eq $latestLoadedTuningRevision
}

$coreFeatureKeys = @('perceive_enemy', 'march_objective', 'respawn', 'take_firing_position')
$movementFeatureKeys = @('unstick_reposition', 'route_waypoint_abandon', 'objective_lane_shift')
$objectiveFeatureKeys = @('attack_tower', 'attack_core', 'capture_site', 'react_to_base_attack')
$succeededFeatureKeys = @($featureCoverage.Keys | Where-Object { $featureCoverage[$_].succeeded -gt 0 })
$missingCoreFeatureKeys = @($coreFeatureKeys | Where-Object { $succeededFeatureKeys -notcontains $_ })
$missingMovementFeatureKeys = @($movementFeatureKeys | Where-Object { $succeededFeatureKeys -notcontains $_ })
$missingObjectiveFeatureKeys = @($objectiveFeatureKeys | Where-Object { $succeededFeatureKeys -notcontains $_ })
$featureCoverageCanaryStatus = if ($featureCoverage.Count -eq 0) {
    'no_feature_coverage'
} elseif ($matchEnds.Count -eq 0 -and $weaponFired -lt 200 -and $towerFired -eq 0) {
    'observe_more'
} elseif ($missingCoreFeatureKeys.Count -gt 0) {
    'missing_core_features'
} else {
    'core_features_covered'
}

$recommendations = @()
if ($botStrategyLoaded.Count -eq 0) {
    $recommendations += [pscustomobject]@{
        Priority = 1
        Code = 'wait_for_strategy_loader_adoption'
        Rationale = 'The tuning JSON exists, but the running DLL has not loaded UArchonBotStrategyTuningLibrary yet.'
        Action = 'Do not attribute JSON edits to live behavior until BotStrategyTuningLoaded appears.'
    }
}
if (-not $hasCombatWindow) {
    $recommendations += [pscustomobject]@{
        Priority = 2
        Code = 'wait_for_combat_window'
        Rationale = 'This log window has not yet reached weapon/perception/feature activity, so early movement noise is not enough for balance changes.'
        Action = 'Keep sampling until WeaponFired, BotNativePerception, or BotFeatureCoverage appears.'
    }
}
if ($botStrategyLoaded.Count -gt 0 -and $botConfigured.Count -eq 0) {
    $recommendations += [pscustomobject]@{
        Priority = 2
        Code = 'missing_bot_configured_role_windows'
        Rationale = 'The tuning loader is live, but the diagnosis window did not include BotConfigured role/window lines.'
        Action = 'Sample from match start or add a one-per-match BotStrategyRuntimePolicy summary before tuning role behavior.'
    }
}
if ($false -eq $tuningRevisionAdopted) {
    $recommendations += [pscustomobject]@{
        Priority = 2
        Code = 'pending_tuning_revision_adoption'
        Rationale = 'The local tuning JSON revision differs from the latest revision loaded by the running match.'
        Action = 'Wait for the next match/hot-load boundary before attributing behavior to the local JSON revision.'
    }
}
if ($botStrategyLoaded.Count -gt 0 -and $botConfigured.Count -gt 0 -and $botConfiguredBreacherFocusCount -eq 0) {
    $recommendations += [pscustomobject]@{
        Priority = 2
        Code = 'breacher_focus_not_confirmed'
        Rationale = 'Configured bot logs exist, but none prove breachers received the short pursuit/threat windows.'
        Action = 'Do not tune breacher pursuit behavior until BotConfiguredBreacherFocusCount is greater than zero.'
    }
}
if ($botUnstickQuery.Count -eq 0 -and ($stuckTotal -ge 20 -or $maxObservedStuckAttempt -ge 5)) {
    $recommendations += [pscustomobject]@{
        Priority = 3
        Code = 'adopt_unstick_query_before_tuning_distances'
        Rationale = 'The live tail has stuck recovery but no BotUnstickQuery, so the candidate-query C++ slice is not active.'
        Action = 'After hot reload/adoption, compare clear/fallback unstick query counts before increasing distances.'
    }
}
if ($botUnstickQuery.Count -gt 0 -and $botUnstickQueryFallbackCount -eq 0 -and $stuckTotal -ge 40 -and $maxObservedStuckAttempt -ge ($routeStallMaxUnstickAttempts * 2)) {
    $recommendations += [pscustomobject]@{
        Priority = 3
        Code = 'clear_unstick_still_stalling'
        Rationale = 'Unstick candidate queries are finding clear moves, but some bots still repeat high stuck attempts in the same match window.'
        Action = 'Prefer earlier route abandonment or objective lane shifting over larger unstick distances.'
    }
}
if ($botFiringPosition.Count -eq 0 -and ($stuckTotal -ge 20 -or $maxObservedStuckAttempt -ge 5)) {
    $recommendations += [pscustomobject]@{
        Priority = 4
        Code = 'adopt_firing_position_before_balance_changes'
        Rationale = 'Bots are still recovering near objective zones without take_firing_position evidence.'
        Action = 'Adopt the firing-position slice, then re-check whether objective-adjacent stuck zones fall.'
    }
}
if ($routeStallMaxUnstickAttempts -gt 0 -and $routeWaypointAbandoned.Count -eq 0 -and ($stuckTotal -ge 50 -or $maxObservedStuckAttempt -ge ($routeStallMaxUnstickAttempts * 2))) {
    $recommendations += [pscustomobject]@{
        Priority = 5
        Code = 'watch_route_stall_policy_adoption'
        Rationale = 'The log window has repeated stuck recovery but no BotRouteWaypointAbandoned marker.'
        Action = if ($botUnstickQuery.Count -eq 0) { 'Adopt the latest native AI build first; route-stall decisions depend on the same low-progress recovery path.' } else { 'If the marker remains absent after adoption, lower route_stall_max_unstick_attempts in the next tuning revision.' }
    }
}
if ($objectiveStallLaneShiftAttempts -gt 0 -and $objectiveLaneShift.Count -eq 0 -and $botFiringPosition.Count -gt 0 -and $maxObservedStuckAttempt -ge ($objectiveStallLaneShiftAttempts * 2)) {
    $recommendations += [pscustomobject]@{
        Priority = 6
        Code = 'watch_objective_lane_shift_adoption'
        Rationale = 'Bots are taking firing positions but repeated stuck attempts have not produced BotObjectiveLaneShift.'
        Action = 'After the route-stall build is adopted, compare lane-shift markers against objective-adjacent stuck zones before changing aggression ranges.'
    }
}
if ($objectiveLaneShift.Count -gt 0 -and $fighterStuckDominant -and $maxObservedStuckAttempt -ge ($objectiveStallLaneShiftAttempts * 2)) {
    $recommendations += [pscustomobject]@{
        Priority = 6
        Code = 'tune_fighter_approach_thresholds'
        Rationale = 'Objective lane shifts are live, but fighter stuck pressure still dominates after repeated clear unstick attempts.'
        Action = 'Try a JSON-only revision with earlier route-stall abandonment and earlier objective lane shifts; keep breacher pursuit windows unchanged.'
    }
}
if ($objectiveContextDominant -and $objectiveStallLaneShiftAttempts -gt 3 -and $maxObservedStuckAttempt -ge ($objectiveStallLaneShiftAttempts + 2)) {
    $recommendations += [pscustomobject]@{
        Priority = 6
        Code = 'objective_standoff_still_stalling'
        Rationale = 'Stuck-recovery context shows the current pressure is objective/tower standoff rather than route waypoint pursuit.'
        Action = 'Try an objective-lane-only JSON revision; keep route_stall_max_unstick_attempts and breacher windows unchanged.'
    }
}
if ($topStuckZones.Count -gt 0 -and $stuckTotal -gt 20) {
    $recommendations += [pscustomobject]@{
        Priority = 7
        Code = 'watch_stuck_hot_zones'
        Rationale = 'Stuck recovery is spatially clustered, so terrain/route probes may matter more than global aggression.'
        Action = 'Track top stuck zones across match windows; send stable terrain-route hotspots to Quarry if they persist after unstick query adoption.'
    }
}

$tuningGateReasons = @()
$preMatchOrRotatedWindow = -not $hasCombatWindow -and
    $botStrategyLoaded.Count -eq 0 -and
    $botConfigured.Count -eq 0 -and
    $weaponFired -eq 0 -and
    $stuckTotal -eq 0

if ($preMatchOrRotatedWindow) {
    $tuningGateStatus = 'observe_more'
    $tuningGateReasons += 'match_window_not_started_or_log_rotated'
} else {
    if ($botStrategyLoaded.Count -eq 0) { $tuningGateReasons += 'strategy_loader_not_live' }
    if ($false -eq $tuningRevisionAdopted) { $tuningGateReasons += 'tuning_revision_pending_adoption' }
    if ($botConfigured.Count -eq 0) { $tuningGateReasons += 'bot_configured_window_missing' }
    if ($botConfigured.Count -gt 0 -and $botConfiguredBreacherFocusCount -eq 0) { $tuningGateReasons += 'breacher_focus_not_confirmed' }
    if (-not $hasCombatWindow) { $tuningGateReasons += 'combat_window_not_reached' }
    if ($botUnstickQuery.Count -eq 0 -and ($stuckTotal -ge 20 -or $maxObservedStuckAttempt -ge 5)) { $tuningGateReasons += 'unstick_query_missing_under_stuck_pressure' }
    if ($botFiringPosition.Count -eq 0 -and ($stuckTotal -ge 20 -or $maxObservedStuckAttempt -ge 5)) { $tuningGateReasons += 'firing_position_missing_under_stuck_pressure' }

    $tuningGateStatus = 'blocked'
    if ($tuningGateReasons.Count -eq 0) {
        if ($matchEnds.Count -eq 0 -and $weaponFired -lt 200 -and $stuckTotal -lt 20) {
            $tuningGateStatus = 'observe_more'
            $tuningGateReasons += 'early_match_window'
        } else {
            $tuningGateStatus = 'ready_for_tuning_review'
        }
    } elseif ($tuningGateReasons.Count -eq 1 -and $tuningGateReasons[0] -eq 'tuning_revision_pending_adoption') {
        $tuningGateStatus = 'observe_more'
    }
}

$suggestedNextTuning = $null
$suggestedNextTuningBlockedReason = $null
if ($preMatchOrRotatedWindow) {
    $suggestedNextTuningBlockedReason = 'match_window_not_started_or_log_rotated'
} elseif ($botStrategyLoaded.Count -eq 0) {
    $suggestedNextTuningBlockedReason = 'strategy_loader_not_live'
} elseif ($false -eq $tuningRevisionAdopted) {
    $suggestedNextTuningBlockedReason = 'pending_tuning_revision_adoption'
} elseif (-not $hasCombatWindow) {
    $suggestedNextTuningBlockedReason = 'combat_window_not_reached'
} else {
    $nextRouteStallMax = $routeStallMaxUnstickAttempts
    $suggestedRationale = @()
    if ($routeWaypointAbandoned.Count -eq 0 -and $routeStallMaxUnstickAttempts -gt 4 -and $maxObservedStuckAttempt -ge ($routeStallMaxUnstickAttempts * 3)) {
        $nextRouteStallMax = [math]::Max(4, $routeStallMaxUnstickAttempts - 2)
        $suggestedRationale += 'no_route_abandon_markers_under_repeated_stuck'
    } elseif ($botUnstickQuery.Count -gt 0 -and $botUnstickQueryFallbackCount -eq 0 -and $routeWaypointAbandoned.Count -gt 0 -and $routeStallMaxUnstickAttempts -gt 5 -and $maxObservedStuckAttempt -ge ($routeStallMaxUnstickAttempts * 2)) {
        $nextRouteStallMax = [math]::Max(5, $routeStallMaxUnstickAttempts - 2)
        $suggestedRationale += 'clear_unstick_queries_still_reached_high_route_stalls'
    }

    $nextLaneShiftAttempts = $objectiveStallLaneShiftAttempts
    if ($objectiveLaneShift.Count -eq 0 -and $objectiveStallLaneShiftAttempts -gt 3 -and $maxObservedStuckAttempt -ge ($objectiveStallLaneShiftAttempts * 3)) {
        $nextLaneShiftAttempts = [math]::Max(3, $objectiveStallLaneShiftAttempts - 1)
        $suggestedRationale += 'no_objective_lane_shift_markers_under_repeated_stuck'
    } elseif ($objectiveLaneShift.Count -gt 0 -and $botFiringPosition.Count -gt 0 -and $fighterStuckDominant -and $objectiveStallLaneShiftAttempts -gt 4 -and $maxObservedStuckAttempt -ge ($objectiveStallLaneShiftAttempts * 2)) {
        $nextLaneShiftAttempts = [math]::Max(4, $objectiveStallLaneShiftAttempts - 2)
        $suggestedRationale += 'fighter_stuck_dominates_after_live_lane_shifts'
    } elseif ($objectiveContextLaneZeroDominant -and $objectiveStallLaneShiftAttempts -gt 2 -and $maxObservedStuckAttempt -ge ($objectiveStallLaneShiftAttempts + 2)) {
        $nextLaneShiftAttempts = [math]::Max(2, $objectiveStallLaneShiftAttempts - 1)
        $suggestedRationale += 'objective_stalls_still_cluster_before_first_lane_shift'
    }

    $suggestedNextTuning = [pscustomobject]@{
        Basis = 'loader_live_tail_window'
        Rationale = @($suggestedRationale)
        Roles = [pscustomobject]@{
            breacher_pursuit_memory_window_seconds = if ($tuning -and $tuning.roles) { $tuning.roles.breacher_pursuit_memory_window_seconds } else { $null }
            breacher_threat_window_seconds = if ($tuning -and $tuning.roles) { $tuning.roles.breacher_threat_window_seconds } else { $null }
        }
        Movement = [pscustomobject]@{
            route_stall_max_unstick_attempts = $nextRouteStallMax
            objective_stall_lane_shift_attempts = $nextLaneShiftAttempts
            objective_stall_max_lane_shift = $objectiveStallMaxLaneShift
        }
        Notes = if ($suggestedRationale.Count -gt 0) { 'JSON-only movement revision is supported by the current post-adoption combat window; keep role windows unchanged.' } else { 'No movement threshold change is suggested from this window.' }
    }
}

$runningUnreal = @(Get-Process UnrealEditor,UnrealEditor-Cmd,ArchonFactoryCanary -ErrorAction SilentlyContinue |
    Select-Object Id, ProcessName, MainWindowTitle, Path, StartTime)

$result = [pscustomobject]@{
    ProjectRoot = $ProjectRoot
    LogPath = $LogPath
    SnapshotUtc = (Get-Date).ToUniversalTime().ToString('o')
    LogExists = $logExists
    LogLastWriteTimeUtc = if ($logItem) { $logItem.LastWriteTimeUtc.ToString('o') } else { $null }
    TailLinesRequested = $TailLines
    TailLinesRead = $lines.Count
    RunningUnrealProcesses = $runningUnreal
    BotStrategyTuningPath = $tuningPath
    BotStrategyTuningRevision = $localTuningRevision
    BotStrategyTuningReadError = $tuningReadError
    BotStrategyBreacherPursuitMemoryWindowSeconds = if ($tuning -and $tuning.roles) { $tuning.roles.breacher_pursuit_memory_window_seconds } else { $null }
    BotStrategyBreacherThreatWindowSeconds = if ($tuning -and $tuning.roles) { $tuning.roles.breacher_threat_window_seconds } else { $null }
    BotStrategyRouteStallMaxUnstickAttempts = $routeStallMaxUnstickAttempts
    BotStrategyObjectiveStallLaneShiftAttempts = $objectiveStallLaneShiftAttempts
    BotStrategyObjectiveStallMaxLaneShift = $objectiveStallMaxLaneShift
    BotStrategyTuningLoadedCount = $botStrategyLoaded.Count
    LatestBotStrategyTuningLoaded = if ($botStrategyLoaded.Count -gt 0) { $botStrategyLoaded[-1] } else { $null }
    LatestBotStrategyTuningLoadedRevision = $latestLoadedTuningRevision
    BotStrategyTuningRevisionAdopted = $tuningRevisionAdopted
    BotConfiguredCount = $botConfigured.Count
    LatestBotConfigured = if ($botConfigured.Count -gt 0) { $botConfigured[-1] } else { $null }
    BotConfiguredBreacherFocusCount = $botConfiguredBreacherFocusCount
    BotConfiguredByRole = @(Get-TopCounts $configuredByRole 4)
    BotConfiguredByRevision = @(Get-TopCounts $configuredByRevision 4)
    LatestBuildFingerprint = if ($buildFingerprints.Count -gt 0) { $buildFingerprints[-1] } else { $null }
    MatchEndCount = $matchEnds.Count
    LatestMatchEnd = if ($matchEnds.Count -gt 0) { $matchEnds[-1] } else { $null }
    HasCombatWindow = $hasCombatWindow
    WeaponFiredCount = $weaponFired
    TowerFiredCount = $towerFired
    BotStuckRecoveryCount = [int]$stuckTotal
    MaxObservedStuckAttempt = $maxObservedStuckAttempt
    TopStuckByBot = @(Get-TopCounts $stuckByBot 12)
    TopStuckByTeam = @(Get-TopCounts $stuckByTeam 4)
    TopStuckByRole = @(Get-TopCounts $stuckByRole 4)
    TopStuckZones = @($topStuckZones)
    MaxStuckAttemptByBot = $maxAttemptRows
    BotUnstickQueryCount = $botUnstickQuery.Count
    BotUnstickQueryClearCount = $botUnstickQueryClearCount
    BotUnstickQueryFallbackCount = $botUnstickQueryFallbackCount
    BotUnstickQueryClearRate = $botUnstickQueryClearRate
    LatestBotUnstickQuery = if ($botUnstickQuery.Count -gt 0) { $botUnstickQuery[-1] } else { $null }
    TopUnstickQueryBySelection = @(Get-TopCounts $unstickSelection 5)
    TopUnstickQueryByBot = @(Get-TopCounts $unstickByBot 10)
    TopUnstickQuerySelectionByBot = @(Get-TopCounts $unstickSelectionByBot 10)
    TopUnstickQuerySelectionByRole = @(Get-TopCounts $unstickSelectionByRole 8)
    MaxUnstickQueryAttemptByBot = $maxUnstickAttemptRows
    BotStuckRecoveryContextCount = $stuckRecoveryContext.Count
    LatestBotStuckRecoveryContext = if ($stuckRecoveryContext.Count -gt 0) { $stuckRecoveryContext[-1] } else { $null }
    TopStuckRecoveryContextByPhase = @(Get-TopCounts $stuckContextByPhase 6)
    TopStuckRecoveryContextByPhaseRole = @(Get-TopCounts $stuckContextByPhaseRole 8)
    TopStuckRecoveryContextByObjectiveTarget = @(Get-TopCounts $stuckContextByObjectiveTarget 8)
    TopStuckRecoveryContextByPhaseTargetLaneShift = @(Get-TopCounts $stuckContextByPhaseTargetLaneShift 12)
    TopStuckRecoveryContextByLaneShift = @(Get-TopCounts $stuckContextByLaneShift 8)
    TopObjectiveStuckContextByLaneShift = @(Get-TopCounts $objectiveStuckContextByLaneShift 8)
    ObjectiveStuckContextLaneZeroCount = $objectiveContextLaneZeroCount
    ObjectiveStuckContextShiftedCount = $objectiveContextShiftedCount
    ObjectiveStuckContextAtMaxLaneShiftCount = $objectiveContextAtMaxLaneShiftCount
    BotRouteWaypointAbandonedCount = $routeWaypointAbandoned.Count
    LatestRouteWaypointAbandoned = if ($routeWaypointAbandoned.Count -gt 0) { $routeWaypointAbandoned[-1] } else { $null }
    TopRouteWaypointAbandonedByBot = @(Get-TopCounts $routeWaypointAbandonedByBot 10)
    BotObjectiveLaneShiftCount = $objectiveLaneShift.Count
    LatestObjectiveLaneShift = if ($objectiveLaneShift.Count -gt 0) { $objectiveLaneShift[-1] } else { $null }
    TopObjectiveLaneShiftByBot = @(Get-TopCounts $objectiveLaneShiftByBot 10)
    TopObjectiveLaneShiftByTarget = @(Get-TopCounts $objectiveLaneShiftByTarget 8)
    TopObjectiveLaneShiftByTargetLane = @(Get-TopCounts $objectiveLaneShiftByTargetLane 8)
    MaxObjectiveLaneShiftByBot = @($objectiveLaneShiftMaxByBot.Values | Sort-Object -Property @{ Expression = 'LaneShift'; Descending = $true }, @{ Expression = 'Bot'; Ascending = $true } | Select-Object -First 10)
    BotFiringPositionCount = $botFiringPosition.Count
    LatestBotFiringPosition = if ($botFiringPosition.Count -gt 0) { $botFiringPosition[-1] } else { $null }
    TopBotFiringPositionByTargetLaneShift = @(Get-TopCounts $firingPositionByTargetLaneShift 12)
    TopBotFiringPositionByLaneShift = @(Get-TopCounts $firingPositionByLaneShift 8)
    NativePerceptionCount = [int]$nativeTotal
    FallbackPerceptionCount = [int]$fallbackTotal
    TopNativePerceptionByBot = @(Get-TopCounts $nativeByBot 10)
    TopFallbackPerceptionByBot = @(Get-TopCounts $fallbackByBot 10)
    NativeSenseCounts = @(Get-TopCounts $nativeSense 5)
    RespawnCount = [int]$respawnTotal
    TopRespawnByBot = @(Get-TopCounts $respawnByBot 10)
    TopBotStates = @(Get-TopCounts $stateByName 10)
    TopStateByBot = @(Get-TopCounts $stateByBot 15)
    TopStateByRole = @(Get-TopCounts $stateByRole 12)
    StructureHitByTargetTeam = @(Get-TopCounts $structureHitByTargetTeam 4)
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
    VectorTuningGate = [pscustomobject]@{
        Status = $tuningGateStatus
        Reasons = @($tuningGateReasons)
    }
    SuggestedNextTuning = $suggestedNextTuning
    SuggestedNextTuningBlockedReason = $suggestedNextTuningBlockedReason
    Recommendations = $recommendations
}

$result | ConvertTo-Json -Depth 10 | Set-Content -LiteralPath $OutputPath -Encoding UTF8
$historyDir = Split-Path -Parent $HistoryPath
if ($historyDir) {
    New-Item -ItemType Directory -Force -Path $historyDir | Out-Null
}
$historyEvent = [pscustomobject]@{
    SnapshotUtc = $result.SnapshotUtc
    LogLastWriteTimeUtc = $result.LogLastWriteTimeUtc
    TailLinesRead = $result.TailLinesRead
    BotStrategyTuningRevision = $result.BotStrategyTuningRevision
    BotStrategyTuningLoadedCount = $result.BotStrategyTuningLoadedCount
    LatestBotStrategyTuningLoadedRevision = $result.LatestBotStrategyTuningLoadedRevision
    BotStrategyTuningRevisionAdopted = $result.BotStrategyTuningRevisionAdopted
    BotConfiguredCount = $result.BotConfiguredCount
    BotConfiguredBreacherFocusCount = $result.BotConfiguredBreacherFocusCount
    BotConfiguredByRole = @($result.BotConfiguredByRole)
    LatestBuildFingerprint = $result.LatestBuildFingerprint
    MatchEndCount = $result.MatchEndCount
    LatestMatchEnd = $result.LatestMatchEnd
    HasCombatWindow = $result.HasCombatWindow
    WeaponFiredCount = $result.WeaponFiredCount
    TowerFiredCount = $result.TowerFiredCount
    BotStuckRecoveryCount = $result.BotStuckRecoveryCount
    MaxObservedStuckAttempt = $result.MaxObservedStuckAttempt
    BotUnstickQueryCount = $result.BotUnstickQueryCount
    BotUnstickQueryClearCount = $result.BotUnstickQueryClearCount
    BotUnstickQueryFallbackCount = $result.BotUnstickQueryFallbackCount
    BotUnstickQueryClearRate = $result.BotUnstickQueryClearRate
    BotStuckRecoveryContextCount = $result.BotStuckRecoveryContextCount
    TopStuckRecoveryContextByPhase = @($result.TopStuckRecoveryContextByPhase)
    TopStuckRecoveryContextByPhaseTargetLaneShift = @($result.TopStuckRecoveryContextByPhaseTargetLaneShift)
    TopObjectiveStuckContextByLaneShift = @($result.TopObjectiveStuckContextByLaneShift)
    ObjectiveStuckContextLaneZeroCount = $result.ObjectiveStuckContextLaneZeroCount
    ObjectiveStuckContextShiftedCount = $result.ObjectiveStuckContextShiftedCount
    ObjectiveStuckContextAtMaxLaneShiftCount = $result.ObjectiveStuckContextAtMaxLaneShiftCount
    BotFiringPositionCount = $result.BotFiringPositionCount
    TopBotFiringPositionByTargetLaneShift = @($result.TopBotFiringPositionByTargetLaneShift)
    BotRouteWaypointAbandonedCount = $result.BotRouteWaypointAbandonedCount
    BotObjectiveLaneShiftCount = $result.BotObjectiveLaneShiftCount
    TopObjectiveLaneShiftByTargetLane = @($result.TopObjectiveLaneShiftByTargetLane)
    NativePerceptionCount = $result.NativePerceptionCount
    FallbackPerceptionCount = $result.FallbackPerceptionCount
    TopStuckZones = @($result.TopStuckZones | Select-Object -First 5)
    TopStuckByRole = @($result.TopStuckByRole)
    TopUnstickQueryBySelection = @($result.TopUnstickQueryBySelection)
    TopStateByRole = @($result.TopStateByRole | Select-Object -First 8)
    BotFeatureCoverageCanary = $result.BotFeatureCoverageCanary
    VectorTuningGate = $result.VectorTuningGate
    SuggestedNextTuning = $result.SuggestedNextTuning
    SuggestedNextTuningBlockedReason = $result.SuggestedNextTuningBlockedReason
}
$historyEvent | ConvertTo-Json -Depth 8 -Compress | Add-Content -LiteralPath $HistoryPath -Encoding UTF8
$result | ConvertTo-Json -Depth 10
