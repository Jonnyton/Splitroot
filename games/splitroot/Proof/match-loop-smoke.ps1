param(
    [string]$ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..\..')).Path,
    [string]$EngineRoot = 'C:\Program Files\Epic Games\UE_5.7'
)

$ErrorActionPreference = 'Stop'

$projectFile = Join-Path $ProjectRoot 'ArchonFactoryCanary.uproject'
$editorCmd = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$proofDir = Join-Path $ProjectRoot 'Saved\Proof'
$logPath = Join-Path $proofDir 'last-match-loop-smoke.log'
$jsonPath = Join-Path $proofDir 'last-match-loop-smoke.json'

New-Item -ItemType Directory -Force -Path $proofDir | Out-Null

# Match-loop proof: valley spawns with base cores + resource sites, the
# match state actor walks Warmup -> Live -> MatchEnd -> Traveling in real
# frames (-ArchonMatchProof shortens the clock and scripts the arc:
# player placed on the central site, site flips, supply ticks, enemy core
# destroyed, scoreboard runs out, travel requested, C++ issues quit).
# No -ExecCmds=quit here: the proof owns process exit.
$argsList = @(
    $projectFile,
    '/Engine/Maps/Entry',
    '-game',
    '-NullRHI',
    '-NoSound',
    '-NoSplash',
    '-unattended',
    '-nop4',
    '-stdout',
    '-FullStdOutLogOutput',
    '-ArchonSplitrootValley',
    '-ArchonMatchLoop',
    '-ArchonMatchProof'
)

$output = & $editorCmd @argsList 2>&1
$exitCode = $LASTEXITCODE
$text = ($output | ForEach-Object { $_.ToString() }) -join [Environment]::NewLine
$text | Set-Content -LiteralPath $logPath -Encoding UTF8

$result = [pscustomobject]@{
    ProjectRoot = $ProjectRoot
    EngineRoot = $EngineRoot
    Map = '/Engine/Maps/Entry'
    ExitCode = $exitCode

    ValleySpawnedWithCores  = $text -match 'ArchonFactoryCanary: SplitrootValleySpawned placements=\d+ blocks=\d+ stones=\d+ bases=\d+ cores=2'
    MatchLoopActive         = $text -match 'ArchonFactoryCanary: MatchLoopActive sites=3 proof=true'
    MatchWentLive           = $text -match 'ArchonFactoryCanary: MatchPhase phase=Live'
    ProofPlayerPlaced       = $text -match 'ArchonFactoryCanary: MatchProofPlayerPlaced site=resource_site_central'
    CentralSiteCaptured     = $text -match 'ArchonFactoryCanary: SiteCaptured site=resource_site_central team=0'
    SupplyTickedWithSite    = $text -match 'ArchonFactoryCanary: SupplyTick team=0 sites=[1-9] gained=\d+ total=\d+'
    ProofCoreKillIssued     = $text -match 'ArchonFactoryCanary: MatchProofCoreKillIssued target=lenswright_base_core'
    MatchEndedCoreDestroyed = $text -match 'ArchonFactoryCanary: MatchEnd winner=TeamA reason=core_destroyed pointsA=[1-9]\d* pointsB=\d+'
    ScoreboardPhaseReached  = $text -match 'ArchonFactoryCanary: MatchPhase phase=MatchEnd'
    TravelPhaseReached      = $text -match 'ArchonFactoryCanary: MatchPhase phase=Traveling'
    TravelRequested         = $text -match 'ArchonFactoryCanary: TravelRequested reason=match_complete'
    QuitCommandHonored      = $text -match 'UGameEngine::HandleExitCommand'
    LogPath                 = $logPath
}

$result | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath $jsonPath -Encoding UTF8
$result | ConvertTo-Json -Depth 4

if (
    $exitCode -ne 0 -or
    -not $result.ValleySpawnedWithCores -or
    -not $result.MatchLoopActive -or
    -not $result.MatchWentLive -or
    -not $result.ProofPlayerPlaced -or
    -not $result.CentralSiteCaptured -or
    -not $result.SupplyTickedWithSite -or
    -not $result.ProofCoreKillIssued -or
    -not $result.MatchEndedCoreDestroyed -or
    -not $result.ScoreboardPhaseReached -or
    -not $result.TravelPhaseReached -or
    -not $result.TravelRequested -or
    -not $result.QuitCommandHonored
) {
    exit 1
}
