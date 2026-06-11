param(
    [string]$ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..\..')).Path,
    [string]$EngineRoot = 'C:\Program Files\Epic Games\UE_5.7'
)

$ErrorActionPreference = 'Stop'

$projectFile = Join-Path $ProjectRoot 'ArchonFactoryCanary.uproject'
$editorCmd = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$proofDir = Join-Path $ProjectRoot 'Saved\Proof'
$logPath = Join-Path $proofDir 'last-first-60-seconds-smoke.log'
$jsonPath = Join-Path $proofDir 'last-first-60-seconds-smoke.json'

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
    '-ArchonRunFirst60SecondsProof',
    '-ExecCmds=quit'
)

$output = & $editorCmd @argsList 2>&1
$exitCode = $LASTEXITCODE
$text = ($output | ForEach-Object { $_.ToString() }) -join [Environment]::NewLine
$text | Set-Content -LiteralPath $logPath -Encoding UTF8

$result = [pscustomobject]@{
    ProjectRoot = $ProjectRoot
    EngineRoot = $EngineRoot
    Map = '/Game/FirstPerson/Lvl_FirstPerson'
    ExitCode = $exitCode

    StartedArchonFactoryCanary       = $text -match 'Running engine for game: ArchonFactoryCanary'
    GameStarted                      = $text -match 'Starting Game\.'
    MapLoadCompleted                 = $text -match 'Load map complete /Game/FirstPerson/Lvl_FirstPerson'

    BlockoutVerdantOutpostPresent    = $text -match 'ArchonFactoryCanary: BlockoutActor name=VerdantOutpost count=1'
    BlockoutSplitrootTreePresent     = $text -match 'ArchonFactoryCanary: BlockoutActor name=SplitrootTreeCentral count=1'
    BlockoutLenswrightGhostPresent   = $text -match 'ArchonFactoryCanary: BlockoutActor name=LenswrightOutpostGhost count=1 ghost=true'
    BlockoutCoverStoneCount          = $text -match 'ArchonFactoryCanary: BlockoutActor name=CoverStoneRootVault count=12'

    First60VisibilityConfigured      = $text -match 'ArchonFactoryCanary: First60Visibility configured=true team=0 initialLit=true'
    First60WidgetOpened              = $text -match 'ArchonFactoryCanary: First60Widget opened=true'
    First60WidgetSelectedCanarySquad = $text -match 'ArchonFactoryCanary: First60Widget selected=canary_squad'
    First60WidgetIssuedMoveOrder     = $text -match 'ArchonFactoryCanary: First60Widget order=MoveSquad target=splitroot_central submitted=true'
    First60WidgetClosed              = $text -match 'ArchonFactoryCanary: First60Widget closed=true'

    First60SquadAcceptedOrder        = $text -match 'ArchonFactoryCanary: First60Squad accepted=true sequence=\d+'
    First60SquadTransitionedToMoving = $text -match 'ArchonFactoryCanary: First60Squad state=Moving'

    First60RootVaultLaunch1          = $text -match 'ArchonFactoryCanary: First60RootVault launchIndex=1 magnitudeForward=850 magnitudeUp=450'
    First60RootVaultLaunch2          = $text -match 'ArchonFactoryCanary: First60RootVault launchIndex=2'
    First60RootVaultLaunch3          = $text -match 'ArchonFactoryCanary: First60RootVault launchIndex=3'
    First60RootVaultInputBridgeRouted = $text -match 'ArchonFactoryCanary: RuntimeRootVaultInput routed=true.*vaultPending=true.*standardJumpSuppressed=true'
    First60RootVaultCooldownEnforced = $text -match 'ArchonFactoryCanary: First60RootVault cooldownEnforced=true blocked=\d+'

    First60ArcCompleted              = $text -match 'ArchonFactoryCanary: First60Arc completed=true rootVaults=\d+ squadMoving=true'

    QuitCommandHonored               = $text -match 'UGameEngine::HandleExitCommand'
    LogPath                          = $logPath
}

$result | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath $jsonPath -Encoding UTF8
$result | ConvertTo-Json -Depth 4

if (
    $exitCode -ne 0 -or
    -not $result.MapLoadCompleted -or
    -not $result.BlockoutVerdantOutpostPresent -or
    -not $result.BlockoutSplitrootTreePresent -or
    -not $result.BlockoutLenswrightGhostPresent -or
    -not $result.BlockoutCoverStoneCount -or
    -not $result.First60VisibilityConfigured -or
    -not $result.First60WidgetOpened -or
    -not $result.First60WidgetSelectedCanarySquad -or
    -not $result.First60WidgetIssuedMoveOrder -or
    -not $result.First60WidgetClosed -or
    -not $result.First60SquadAcceptedOrder -or
    -not $result.First60SquadTransitionedToMoving -or
    -not $result.First60RootVaultLaunch1 -or
    -not $result.First60RootVaultLaunch2 -or
    -not $result.First60RootVaultLaunch3 -or
    -not $result.First60RootVaultInputBridgeRouted -or
    -not $result.First60RootVaultCooldownEnforced -or
    -not $result.First60ArcCompleted -or
    -not $result.QuitCommandHonored
) {
    exit 1
}
