param(
    [string]$ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path,
    [string]$EngineRoot = 'C:\Program Files\Epic Games\UE_5.7'
)

$ErrorActionPreference = 'Stop'

$projectFile = Join-Path $ProjectRoot 'ArchonFactoryCanary.uproject'
$editorCmd = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$proofDir = Join-Path $ProjectRoot 'Saved\Proof'
$logPath = Join-Path $proofDir 'last-unreal-map-smoke.log'
$jsonPath = Join-Path $proofDir 'last-unreal-map-smoke.json'

New-Item -ItemType Directory -Force -Path $proofDir | Out-Null

$argsList = @(
    $projectFile,
    '/Game/FirstPerson/Lvl_FirstPerson',
    '-game',
    '-NullRHI',
    '-NoSound',
    '-NoSplash',
    '-unattended',
    '-nop4',
    '-stdout',
    '-FullStdOutLogOutput',
    '-ArchonRunRuntimeRtsProof',
    '-ArchonRunRuntimeWeaponProof',
    '-ArchonRunRuntimeCombatProof',
    '-ArchonPauseMenuProof',
    '-ArchonInteractProof',
    '-ExecCmds=quit'
)

$output = & $editorCmd @argsList 2>&1
$exitCode = $LASTEXITCODE
$text = ($output | ForEach-Object { $_.ToString() }) -join [Environment]::NewLine
$text | Set-Content -LiteralPath $logPath -Encoding UTF8

$result = [pscustomobject]@{
    ProjectRoot = $ProjectRoot
    EngineRoot = $EngineRoot
    ProjectFile = $projectFile
    UnrealEditorCmd = $editorCmd
    Map = '/Game/FirstPerson/Lvl_FirstPerson'
    ExitCode = $exitCode
    StartedArchonFactoryCanary = $text -match 'Running engine for game: ArchonFactoryCanary'
    GameStarted = $text -match 'Starting Game\.'
    MapLoadStarted = $text -match 'LoadMap: /Game/FirstPerson/Lvl_FirstPerson'
    MapLoadCompleted = $text -match 'Load map complete /Game/FirstPerson/Lvl_FirstPerson'
    FirstPersonGameModeLoaded = $text -match "Game class is 'BP_FirstPersonGameMode_C'"
    RuntimeMapTableSpawned = $text -match 'ArchonFactoryCanary: RuntimeMapTableSpawned'
    RuntimeRtsSquadSpawned = $text -match 'ArchonFactoryCanary: RuntimeRtsSquadSpawned'
    RuntimeTeamVisibilityInitialized = $text -match 'ArchonFactoryCanary: RuntimeTeamVisibilityInitialized'
    RuntimeTeamVisibilityHasLitAndFog = $text -match 'ArchonFactoryCanary: RuntimeTeamVisibilityInitialized.*lit=2.*fog=1.*black=0.*newlyLit=2'
    RuntimeFpsPawnPossessed = $text -match 'ArchonFactoryCanary: RuntimeFpsPawnPossessed'
    RuntimeInputBridgeInstalled = $text -match 'ArchonFactoryCanary: RuntimeInputBridgeInstalled'
    RuntimeMapTableWidgetOpened = $text -match 'ArchonFactoryCanary: RuntimeMapTableWidgetOpened.*opened=true'
    RuntimeMapTableWidgetSelected = $text -match 'ArchonFactoryCanary: RuntimeMapTableWidgetSelected.*selected=canary_squad'
    RuntimeMapTableWidgetOrderSubmitted = $text -match 'ArchonFactoryCanary: RuntimeMapTableWidgetOrder submitted=true.*target=canary_widget_rally'
    RuntimeMapTableWidgetProofSequenceRan = $text -match 'ArchonFactoryCanary: RuntimeMapTableWidgetProofSequence completed=true'
    RuntimeMapTableWidgetClosed = $text -match 'ArchonFactoryCanary: RuntimeMapTableWidgetClosed.*closed=true'
    RuntimeRtsProofSequenceRan = $text -match 'ArchonFactoryCanary: RuntimeRtsProofSequence completed=true'
    RuntimeRtsMoveOrderSubmitted = $text -match 'ArchonFactoryCanary: RuntimeMapTableOrder submitted=true order=0'
    RuntimeRtsAttackOrderSubmitted = $text -match 'ArchonFactoryCanary: RuntimeMapTableOrder submitted=true order=1'
    RuntimeRtsSquadAcceptedMove = $text -match 'ArchonFactoryCanary: RuntimeRtsSquadCommandAccepted.*order=0'
    RuntimeRtsSquadAcceptedAttack = $text -match 'ArchonFactoryCanary: RuntimeRtsSquadCommandAccepted.*order=1'
    RuntimeMapTableClosed = $text -match 'ArchonFactoryCanary: RuntimeMapTableClosed.*fpsControlActive=true'
    RuntimeWeaponFireRouted = $text -match 'ArchonFactoryCanary: RuntimeWeaponInput routed=true firePressed=true fired=true'
    RuntimeWeaponReloadRouted = $text -match 'ArchonFactoryCanary: RuntimeWeaponInput routed=true firePressed=false fired=false reloadPressed=true reloaded=true'
    RuntimeWeaponProofSequenceRan = $text -match 'ArchonFactoryCanary: RuntimeWeaponProofSequence completed=true'
    RuntimeWeaponProofRequested = $text -match 'ArchonFactoryCanary: RuntimeWeaponProofSequenceRequested result=true'
    RuntimeLenswrightBracewrightSpawned = $text -match 'ArchonFactoryCanary: RuntimeLenswrightBracewrightSpawned'
    RuntimeLenswrightSundialOpticSpawned = $text -match 'ArchonFactoryCanary: RuntimeLenswrightSundialOpticSpawned'
    RuntimeLenswrightTargetDamaged = $text -match 'ArchonFactoryCanary: RuntimeLenswrightTargetDamaged damaged=true'
    RuntimeLenswrightAiDecisionShouldFire = $text -match 'ArchonFactoryCanary: RuntimeLenswrightAiDecision.*shouldFire=true'
    RuntimeLenswrightAiFireRouted = $text -match 'ArchonFactoryCanary: RuntimeLenswrightAiFireRouted fired=true.*projectileHit=true'
    RuntimeLenswrightAiDamagedPlayer = $text -match 'ArchonFactoryCanary: RuntimeLenswrightAiDamagedPlayer damaged=true'
    RuntimeCombatProofSequenceRan = $text -match 'ArchonFactoryCanary: RuntimeCombatProofSequence completed=true'
    RuntimeCombatProofRequested = $text -match 'ArchonFactoryCanary: RuntimeCombatProofSequenceRequested result=true'
    PauseMenuOpened = $text -match 'ArchonFactoryCanary: PauseMenuOpened lookScale='
    PauseMenuLookScaleChanged = $text -match 'ArchonFactoryCanary: MouseLookScaleChanged scale=0\.120'
    PauseMenuClosed = $text -match 'ArchonFactoryCanary: PauseMenuClosed'
    PauseMenuProofCompleted = $text -match 'ArchonFactoryCanary: PauseMenuProofCompleted opened=true closed=true scale=0\.120'
    MapTableOverlayShown = $text -match 'ArchonFactoryCanary: MapTableOverlayShown visible=true'
    MapTableOverlayHidden = $text -match 'ArchonFactoryCanary: MapTableOverlayHidden'
    MapTableBodyLocked = $text -match 'ArchonFactoryCanary: MapTableBodyLock locked=true cursor=true'
    MapTableBodyReleased = $text -match 'ArchonFactoryCanary: MapTableBodyLock locked=false cursor=false'
    InteractPromptShown = $text -match 'ArchonFactoryCanary: InteractPromptShown target=map_table'
    InteractOpenedTable = $text -match 'ArchonFactoryCanary: InteractPressed opened=true target=map_table'
    CameraToggledThird = $text -match 'ArchonFactoryCanary: CameraViewToggled view=third'
    InteractProofCompleted = $text -match 'ArchonFactoryCanary: InteractProofCompleted farHidden=true nearShown=true opened=true suppressedWhileOpen=true thirdPerson=true backToFirst=true'
    QuitCommandHonored = $text -match 'UGameEngine::HandleExitCommand'
    LogPath = $logPath
}

$result | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath $jsonPath -Encoding UTF8
$result | ConvertTo-Json -Depth 4

if (
    $exitCode -ne 0 -or
    -not $result.StartedArchonFactoryCanary -or
    -not $result.GameStarted -or
    -not $result.MapLoadStarted -or
    -not $result.MapLoadCompleted -or
    -not $result.FirstPersonGameModeLoaded -or
    -not $result.RuntimeMapTableSpawned -or
    -not $result.RuntimeRtsSquadSpawned -or
    -not $result.RuntimeTeamVisibilityInitialized -or
    -not $result.RuntimeTeamVisibilityHasLitAndFog -or
    -not $result.RuntimeFpsPawnPossessed -or
    -not $result.RuntimeInputBridgeInstalled -or
    -not $result.RuntimeMapTableWidgetOpened -or
    -not $result.RuntimeMapTableWidgetSelected -or
    -not $result.RuntimeMapTableWidgetOrderSubmitted -or
    -not $result.RuntimeMapTableWidgetProofSequenceRan -or
    -not $result.RuntimeMapTableWidgetClosed -or
    -not $result.RuntimeRtsProofSequenceRan -or
    -not $result.RuntimeRtsMoveOrderSubmitted -or
    -not $result.RuntimeRtsAttackOrderSubmitted -or
    -not $result.RuntimeRtsSquadAcceptedMove -or
    -not $result.RuntimeRtsSquadAcceptedAttack -or
    -not $result.RuntimeMapTableClosed -or
    -not $result.RuntimeWeaponFireRouted -or
    -not $result.RuntimeWeaponReloadRouted -or
    -not $result.RuntimeWeaponProofSequenceRan -or
    -not $result.RuntimeWeaponProofRequested -or
    -not $result.RuntimeLenswrightBracewrightSpawned -or
    -not $result.RuntimeLenswrightSundialOpticSpawned -or
    -not $result.RuntimeLenswrightTargetDamaged -or
    -not $result.RuntimeLenswrightAiDecisionShouldFire -or
    -not $result.RuntimeLenswrightAiFireRouted -or
    -not $result.RuntimeLenswrightAiDamagedPlayer -or
    -not $result.RuntimeCombatProofSequenceRan -or
    -not $result.RuntimeCombatProofRequested -or
    -not $result.PauseMenuOpened -or
    -not $result.PauseMenuLookScaleChanged -or
    -not $result.PauseMenuClosed -or
    -not $result.PauseMenuProofCompleted -or
    -not $result.MapTableOverlayShown -or
    -not $result.MapTableOverlayHidden -or
    -not $result.MapTableBodyLocked -or
    -not $result.MapTableBodyReleased -or
    -not $result.InteractPromptShown -or
    -not $result.InteractOpenedTable -or
    -not $result.CameraToggledThird -or
    -not $result.InteractProofCompleted -or
    -not $result.QuitCommandHonored
) {
    exit 1
}
