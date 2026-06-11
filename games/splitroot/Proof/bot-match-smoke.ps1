param(
    [string]$ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..\..')).Path,
    [string]$EngineRoot = 'C:\Program Files\Epic Games\UE_5.7',
    # 5v5 over 180s: lane spread dilutes contact below kill density at
    # 3v3, and the defense towers now gate the cores (attackers grind
    # the 600hp tower first — found 2026-06-10) so the assault arc needs
    # the longer window.
    [int]$BotsPerTeam = 5,
    [int]$ObserveSeconds = 180
)

$ErrorActionPreference = 'Stop'

$projectFile = Join-Path $ProjectRoot 'ArchonFactoryCanary.uproject'
$editorCmd = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$proofDir = Join-Path $ProjectRoot 'Saved\Proof'
$logPath = Join-Path $proofDir 'last-bot-match.log'
$jsonPath = Join-Path $proofDir 'last-bot-match-smoke.json'

New-Item -ItemType Directory -Force -Path $proofDir | Out-Null

# The all-AI match (decision 9): two bot teams spawn at their cores,
# march on the enemy base, fight on contact, die, respawn, and keep
# playing. This smoke proves the brain loop end-to-end headlessly; the
# WATCHED match (recorder frames + marker timeline) is the feel gate.
$mapUrl = '/Engine/Maps/Entry?ArchonSplitrootValley?ArchonMatchLoop?game=/Script/ArchonFactoryCanary.ArchonMatchGameMode?ArchonMapId=splitroot_valley'
$args = @(
    "`"$projectFile`"", $mapUrl,
    '-game', '-NullRHI', '-NoSound', '-NoSplash', '-unattended', '-nop4',
    '-ArchonBotMatch', "-ArchonBotCountPerTeam=$BotsPerTeam",
    '-port=7880',
    "-abslog=`"$logPath`""
)

Remove-Item -LiteralPath $logPath -Force -ErrorAction SilentlyContinue
$proc = Start-Process -FilePath $editorCmd -ArgumentList $args -PassThru -WindowStyle Hidden
Start-Sleep -Seconds $ObserveSeconds
if ($proc -and -not $proc.HasExited) {
    Stop-Process -Id $proc.Id -Force -Confirm:$false -ErrorAction SilentlyContinue
}
Start-Sleep -Seconds 5

$text = if (Test-Path -LiteralPath $logPath) { Get-Content -LiteralPath $logPath -Raw } else { '' }

$featureUseMatches = [regex]::Matches(
    $text,
    'ArchonFactoryCanary: BotFeatureUse bot=(-?\d+) team=(-?\d+) feature=(\S+) decision=(\S+) result=(\S+) reason=(\S+)')
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
function Test-FeatureSucceeded([string]$FeatureName) {
    return $featureCoverage.Contains($FeatureName) -and $featureCoverage[$FeatureName].succeeded -gt 0
}

$marchObjectiveCovered = Test-FeatureSucceeded 'march_objective'
$perceiveEnemyCovered = Test-FeatureSucceeded 'perceive_enemy'
$attackTowerCovered = Test-FeatureSucceeded 'attack_tower'
$attackCoreCovered = Test-FeatureSucceeded 'attack_core'
$respawnFeatureCovered = Test-FeatureSucceeded 'respawn'
$captureSiteCovered = Test-FeatureSucceeded 'capture_site'
$reactToBaseAttackCovered = Test-FeatureSucceeded 'react_to_base_attack'
$takeFiringPositionCovered = Test-FeatureSucceeded 'take_firing_position'
$botStrategyTuningLoaded = $text -match 'ArchonFactoryCanary: BotStrategyTuningLoaded revision='

$result = [pscustomobject]@{
    ProjectRoot = $ProjectRoot
    BotsPerTeam = $BotsPerTeam
    ObserveSeconds = $ObserveSeconds

    BotMatchSpawned   = $text -match "ArchonFactoryCanary: BotMatchSpawned perTeam=$BotsPerTeam total=$($BotsPerTeam * 2)"
    MatchLoopActive   = $text -match 'ArchonFactoryCanary: MatchLoopActive sites=3'
    SpectatorStart    = $text -match 'ArchonFactoryCanary: SpectatorStart pawn='
    MatchHudShown     = $text -match 'ArchonFactoryCanary: MatchHudShown visible=true'
    NativeBotControllerConfigured = ([regex]::Matches($text, 'ArchonFactoryCanary: BotAIControllerConfigured bot=')).Count -ge ($BotsPerTeam * 2)
    BotStrategyTuningLoaded = $botStrategyTuningLoaded
    NativeBotPerceptionObserved = $text -match 'ArchonFactoryCanary: BotNativePerception bot='
    FallbackPerceptionObserved = $text -match 'ArchonFactoryCanary: BotFallbackPerception bot='
    # -ge 2 (not -eq): a match can END and rotate to a fresh valley inside
    # the window — first happened 2026-06-10 (110s core race, TeamA won).
    TowersConfigured  = ([regex]::Matches($text, 'ArchonFactoryCanary: DefenseTowerConfigured team=')).Count -ge 2
    TowerFired        = $text -match 'ArchonFactoryCanary: TowerFired team='
    BotWaypointReached = $text -match 'ArchonFactoryCanary: BotWaypointReached bot='
    BotRouteWaypointAbandonedLogged = $text -match 'ArchonFactoryCanary: BotRouteWaypointAbandoned bot='
    BotObjectiveLaneShiftLogged = $text -match 'ArchonFactoryCanary: BotObjectiveLaneShift bot='
    BotFiringPositionLogged = $text -match 'ArchonFactoryCanary: BotFiringPosition bot='
    BotsEngaged       = $text -match 'ArchonFactoryCanary: BotState .*state=EngageEnemy'
    BotsFired         = $text -match 'ArchonFactoryCanary: WeaponFired weapon='
    BotRespawned      = $text -match 'ArchonFactoryCanary: BotRespawned bot='
    # Stakes gate: attackers must damage base STRUCTURES (towers gate the
    # cores now, so a window can end mid-siege with the core untouched).
    CoreDamaged       = $text -match 'ArchonFactoryCanary: BaseCoreDamaged team='
    TowerDamaged      = $text -match 'ArchonFactoryCanary: DefenseTowerDamaged team='
    # Bots siege the gate tower BEFORE the core (AttackTower state,
    # 2026-06-11), so a 180s window honestly ends mid-siege. The gated
    # arc claim is "bots push the enemy base objective": tower or core.
    # CoreAttackReached stays as a non-gating observation (it appears in
    # longer windows after the tower falls - first live kill was 328s).
    ObjectivePushReached = $text -match 'ArchonFactoryCanary: BotState .*state=(AttackTower|AttackCore)'
    CoreAttackReached = $text -match 'ArchonFactoryCanary: BotState .*state=AttackCore'
    BothTeamsFought   = ($text -match 'BotState bot=\d+ team=0 state=EngageEnemy') -and ($text -match 'BotState bot=\d+ team=1 state=EngageEnemy')
    BotFeatureUseLogged = $featureUseMatches.Count -gt 0
    BotFeatureCoverageLogged = $featureCoverageMatches.Count -gt 0
    FeatureCoverage = $featureCoverage
    FeatureCoverageKeys = @($featureCoverage.Keys)
    MarchObjectiveCovered = $marchObjectiveCovered
    PerceiveEnemyCovered = $perceiveEnemyCovered
    AttackTowerCovered = $attackTowerCovered
    AttackCoreCovered = $attackCoreCovered
    RespawnFeatureCovered = $respawnFeatureCovered
    CaptureSiteCovered = $captureSiteCovered
    ReactToBaseAttackCovered = $reactToBaseAttackCovered
    TakeFiringPositionCovered = $takeFiringPositionCovered
    NonGatingUnavailableFeatureKeys = @(
        'purchase_reinforcement',
        'use_shop',
        'use_map_table_order',
        'build_tech'
    )

    LogPath = $logPath
}

$result | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $jsonPath -Encoding UTF8
$result | ConvertTo-Json -Depth 8

if (
    -not $result.BotMatchSpawned -or
    -not $result.MatchLoopActive -or
    -not $result.SpectatorStart -or
    -not $result.MatchHudShown -or
    -not $result.NativeBotControllerConfigured -or
    -not $result.BotStrategyTuningLoaded -or
    -not $result.NativeBotPerceptionObserved -or
    -not $result.TowersConfigured -or
    -not $result.TowerFired -or
    -not $result.BotWaypointReached -or
    -not $result.BotFiringPositionLogged -or
    -not $result.BotsEngaged -or
    -not $result.BotsFired -or
    -not $result.BotRespawned -or
    -not ($result.CoreDamaged -or $result.TowerDamaged) -or
    -not $result.ObjectivePushReached -or
    -not $result.BothTeamsFought -or
    -not $result.BotFeatureUseLogged -or
    -not $result.BotFeatureCoverageLogged -or
    -not $result.MarchObjectiveCovered -or
    -not $result.PerceiveEnemyCovered -or
    -not ($result.AttackTowerCovered -or $result.AttackCoreCovered) -or
    -not $result.RespawnFeatureCovered -or
    -not $result.CaptureSiteCovered -or
    -not $result.TakeFiringPositionCovered
) {
    exit 1
}
