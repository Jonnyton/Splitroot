param(
    [string]$ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..\..')).Path,
    [string]$EngineRoot = 'C:\Program Files\Epic Games\UE_5.7'
)

$ErrorActionPreference = 'Stop'

$projectFile = Join-Path $ProjectRoot 'ArchonFactoryCanary.uproject'
$editorCmd = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$proofDir = Join-Path $ProjectRoot 'Saved\Proof'
$logPath = Join-Path $proofDir 'last-map-rotation-smoke.log'
$jsonPath = Join-Path $proofDir 'last-map-rotation-smoke.json'

New-Item -ItemType Directory -Force -Path $proofDir | Out-Null

# Rotation proof: the whole match arc runs on the valley, then instead of
# quitting at the travel boundary (-ArchonMatchProofTravel), the match
# actor performs a REAL ServerTravel to the next rotation entry
# (canary_range). All Archon flags ride the map URL, NOT the command
# line — command-line flags persist across travel and would respawn the
# valley on the next map. The travel URL carries ArchonMatchProofQuit so
# the second world ends the process after booting.
$mapUrl = '/Engine/Maps/Entry?ArchonSplitrootValley?ArchonMatchLoop?ArchonMatchProof?ArchonMatchProofTravel?ArchonMatchProofQuitAfterTravel?ArchonMapId=splitroot_valley'
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

$result = [pscustomobject]@{
    ProjectRoot = $ProjectRoot
    EngineRoot = $EngineRoot
    MapUrl = $mapUrl
    ExitCode = $exitCode

    ValleySpawnedViaUrlOptions = $text -match 'ArchonFactoryCanary: SplitrootValleySpawned placements=\d+ blocks=\d+ stones=\d+ bases=\d+ cores=2'
    MatchLoopActive            = $text -match 'ArchonFactoryCanary: MatchLoopActive sites=3 proof=true'
    MatchEndedCoreDestroyed    = $text -match 'ArchonFactoryCanary: MatchEnd winner=TeamA reason=core_destroyed'
    TravelRequested            = $text -match 'ArchonFactoryCanary: TravelRequested reason=match_complete'
    TravelingToCanaryRange     = $text -match 'ArchonFactoryCanary: TravelingTo map=canary_range url=/Game/FirstPerson/Lvl_FirstPerson\?ArchonMapId=canary_range\?ArchonMatchProofQuit'
    SecondWorldBooted          = $text -match 'ArchonFactoryCanary: MatchProofQuitScheduled map=Lvl_FirstPerson'
    SecondWorldMapTableSpawned = ([regex]::Matches($text, 'ArchonFactoryCanary: RuntimeMapTableSpawned')).Count -ge 2
    QuitCommandHonored         = $text -match 'UGameEngine::HandleExitCommand'
    LogPath                    = $logPath
}

$result | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath $jsonPath -Encoding UTF8
$result | ConvertTo-Json -Depth 4

if (
    $exitCode -ne 0 -or
    -not $result.ValleySpawnedViaUrlOptions -or
    -not $result.MatchLoopActive -or
    -not $result.MatchEndedCoreDestroyed -or
    -not $result.TravelRequested -or
    -not $result.TravelingToCanaryRange -or
    -not $result.SecondWorldBooted -or
    -not $result.SecondWorldMapTableSpawned -or
    -not $result.QuitCommandHonored
) {
    exit 1
}
