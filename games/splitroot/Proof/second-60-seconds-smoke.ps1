param(
    [string]$ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..\..')).Path,
    [string]$EngineRoot = 'C:\Program Files\Epic Games\UE_5.7'
)

$ErrorActionPreference = 'Stop'

$projectFile = Join-Path $ProjectRoot 'ArchonFactoryCanary.uproject'
$editorCmd = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$proofDir = Join-Path $ProjectRoot 'Saved\Proof'
$logPath = Join-Path $proofDir 'last-second-60-seconds-smoke.log'
$jsonPath = Join-Path $proofDir 'last-second-60-seconds-smoke.json'

New-Item -ItemType Directory -Force -Path $proofDir | Out-Null

# -ArchonRunSecond60SecondsProof requires -ArchonRunFirst60SecondsProof:
# the second-60 arc starts from the end state of the first-60 arc.
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
    '-ArchonRunSecond60SecondsProof',
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

    First60ArcCompleted               = $text -match 'ArchonFactoryCanary: First60Arc completed=true'

    Second60EnemiesSpawned            = $text -match 'ArchonFactoryCanary: Second60Enemies spawned=true bracewright=2 sundial=1'
    Second60PlayerWeaponReady         = $text -match 'ArchonFactoryCanary: Second60Player weapon=VerdantThornsproutBow ammo=\d+/\d+ ready=true'
    Second60PlayerFiredAtEnemy        = $text -match 'ArchonFactoryCanary: Second60Player firedShots=[1-9]\d* enemyHitsA=[1-9]\d* enemyHitsB=[1-9]\d* engaged=true'
    Second60BracewrightFiredAtPlayer  = $text -match 'ArchonFactoryCanary: Second60Bracewright firedShots=[1-9]\d* stance=\d+ shouldFire=true'
    Second60PlayerTookDamage          = $text -match 'ArchonFactoryCanary: Second60Player damageTaken=[1-9]\d* health=\d'
    Second60PlayerDied                = $text -match 'ArchonFactoryCanary: Second60Player died=true cause=lenswright_pressure_bolt'
    Second60ObserverPawnPossessed     = $text -match 'ArchonFactoryCanary: Second60Observer pawnPossessed=true'
    Second60CommandWhileWaitOpened    = $text -match 'ArchonFactoryCanary: Second60CommandWhileWait tableOpened=true lifeState=Dead'
    Second60CommandWhileWaitSubmitted = $text -match 'ArchonFactoryCanary: Second60CommandWhileWait orderSubmitted=true sequence=\d+'
    Second60PlayerRespawned           = $text -match 'ArchonFactoryCanary: Second60Player respawned=true lifeState=Alive'
    Second60DeadStateOrderSurvived    = $text -match 'ArchonFactoryCanary: Second60SquadOrderSurvivedRespawn lastSequence=\d+ stillMoving=true'
    Second60ArcCompleted              = $text -match 'ArchonFactoryCanary: Second60Arc completed=true durationSeconds=\d'

    QuitCommandHonored                = $text -match 'UGameEngine::HandleExitCommand'
    LogPath                           = $logPath
}

$result | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath $jsonPath -Encoding UTF8
$result | ConvertTo-Json -Depth 4

if (
    $exitCode -ne 0 -or
    -not $result.First60ArcCompleted -or
    -not $result.Second60EnemiesSpawned -or
    -not $result.Second60PlayerWeaponReady -or
    -not $result.Second60PlayerFiredAtEnemy -or
    -not $result.Second60BracewrightFiredAtPlayer -or
    -not $result.Second60PlayerTookDamage -or
    -not $result.Second60PlayerDied -or
    -not $result.Second60ObserverPawnPossessed -or
    -not $result.Second60CommandWhileWaitOpened -or
    -not $result.Second60CommandWhileWaitSubmitted -or
    -not $result.Second60PlayerRespawned -or
    -not $result.Second60DeadStateOrderSurvived -or
    -not $result.Second60ArcCompleted -or
    -not $result.QuitCommandHonored
) {
    exit 1
}
