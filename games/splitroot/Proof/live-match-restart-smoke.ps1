param(
    [string]$ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..\..')).Path,
    [string]$EngineRoot = 'C:\Program Files\Epic Games\UE_5.7'
)

$ErrorActionPreference = 'Stop'

$projectFile = Join-Path $ProjectRoot 'ArchonFactoryCanary.uproject'
$editorCmd = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$proofDir = Join-Path $ProjectRoot 'Saved\Proof'
$logPath = Join-Path $proofDir 'last-live-match-restart-smoke.log'
$jsonPath = Join-Path $proofDir 'last-live-match-restart-smoke.json'

New-Item -ItemType Directory -Force -Path $proofDir | Out-Null

# Desktop-loop proof: use URL flags so the first match gets the short proof
# clock, then travels like a live hosted match. The second world carries only
# ArchonMatchProofQuit so the smoke can finish after proving the next match
# booted instead of quitting at the scoreboard boundary.
$mapUrl = '/Engine/Maps/Entry?ArchonSplitrootValley?ArchonMatchLoop?ArchonMatchProof?ArchonMatchProofRestart?ArchonMatchProofQuitAfterTravel?ArchonBotMatch?game=/Script/ArchonFactoryCanary.ArchonMatchGameMode?ArchonMapId=splitroot_valley?listen'
$argsList = @(
    $projectFile,
    $mapUrl,
    '-game',
    '-NullRHI',
    '-NoSound',
    '-NoSplash',
    '-unattended',
    '-nop4',
    '-stdout',
    '-FullStdOutLogOutput'
)

$output = & $editorCmd @argsList 2>&1
$exitCode = $LASTEXITCODE
$text = ($output | ForEach-Object { $_.ToString() }) -join [Environment]::NewLine
$text | Set-Content -LiteralPath $logPath -Encoding UTF8

$matchLoopCount = ([regex]::Matches($text, 'ArchonFactoryCanary: MatchLoopActive sites=3')).Count
$botSpawnCount = ([regex]::Matches($text, 'ArchonFactoryCanary: BotMatchSpawned')).Count

$result = [pscustomobject]@{
    ProjectRoot = $ProjectRoot
    EngineRoot = $EngineRoot
    MapUrl = $mapUrl
    ExitCode = $exitCode

    FirstMatchLoopActive     = $text -match 'ArchonFactoryCanary: MatchLoopActive sites=3 proof=true'
    BuildFingerprintLogged   = $text -match 'ArchonFactoryCanary: BuildFingerprint moduleDllUtc=.+ processStartUtc=.+ modulePath=.+UnrealEditor-ArchonFactoryCanary\.dll'
    MatchConfigLogged        = $text -match 'ArchonFactoryCanary: MatchConfig warmupSeconds=2\.0 matchTimeLimitSeconds=900\.0 scoreboardSeconds=3\.0 maxCycleSeconds=905\.0'
    FirstMatchEnded          = $text -match 'ArchonFactoryCanary: MatchEnd winner=TeamA reason=core_destroyed'
    NextMatchPending         = $text -match 'ArchonFactoryCanary: NextMatchPending nextMap=splitroot_valley countdownSeconds=3\.0'
    TravelRequested          = $text -match 'ArchonFactoryCanary: TravelRequested reason=match_complete'
    NextMatchLoading         = $text -match 'ArchonFactoryCanary: NextMatchLoading map=splitroot_valley'
    RestartedSameMatchMap    = $text -match 'ArchonFactoryCanary: TravelingTo map=splitroot_valley url=/Engine/Maps/Entry\?ArchonSplitrootValley\?ArchonMatchLoop\?game=/Script/ArchonFactoryCanary\.ArchonMatchGameMode\?ArchonMapId=splitroot_valley\?ArchonMatchProofQuit\?ArchonBotMatch\?listen'
    SecondMatchLoopActive    = $matchLoopCount -ge 2
    BotFlagSurvivedTravel    = $botSpawnCount -ge 2
    SecondWorldQuitScheduled = $text -match 'ArchonFactoryCanary: MatchProofQuitScheduled map=Entry delaySeconds=2.0'
    QuitCommandHonored       = $text -match 'UGameEngine::HandleExitCommand'
    LogPath                  = $logPath
}

$result | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath $jsonPath -Encoding UTF8
$result | ConvertTo-Json -Depth 4

if (
    $exitCode -ne 0 -or
    -not $result.FirstMatchLoopActive -or
    -not $result.BuildFingerprintLogged -or
    -not $result.MatchConfigLogged -or
    -not $result.FirstMatchEnded -or
    -not $result.NextMatchPending -or
    -not $result.TravelRequested -or
    -not $result.NextMatchLoading -or
    -not $result.RestartedSameMatchMap -or
    -not $result.SecondMatchLoopActive -or
    -not $result.BotFlagSurvivedTravel -or
    -not $result.SecondWorldQuitScheduled -or
    -not $result.QuitCommandHonored
) {
    exit 1
}
