param(
    [string]$ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..\..')).Path,
    [string]$EngineRoot = 'C:\Program Files\Epic Games\UE_5.7'
)

$ErrorActionPreference = 'Stop'

$projectFile = Join-Path $ProjectRoot 'ArchonFactoryCanary.uproject'
$editorCmd = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$proofDir = Join-Path $ProjectRoot 'Saved\Proof'
$logPath = Join-Path $proofDir 'last-rally-reinforcement-smoke.log'
$jsonPath = Join-Path $proofDir 'last-rally-reinforcement-smoke.json'

New-Item -ItemType Directory -Force -Path $proofDir | Out-Null

$mapUrl = '/Engine/Maps/Entry?ArchonSplitrootValley?ArchonMatchLoop?ArchonMatchProof?game=/Script/ArchonFactoryCanary.ArchonMatchGameMode?ArchonMapId=splitroot_valley'
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
    '-FullStdOutLogOutput',
    '-ArchonRunRuntimeRallyProof'
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

    MatchLoopActive          = $text -match 'ArchonFactoryCanary: MatchLoopActive sites=3'
    ProofScheduled           = $text -match 'ArchonFactoryCanary: RuntimeRallyProofScheduled delay=7\.0'
    SupplyTicked             = $text -match 'ArchonFactoryCanary: SupplyTick team=0 sites=\d+ gained=\d+ total=\d+'
    RuntimeRallySubmitted    = $text -match 'ArchonFactoryCanary: RuntimeRallyPointSet submitted=true target=runtime_rally_proof'
    TeamRallyStored          = $text -match 'ArchonFactoryCanary: TeamRallyPointSet team=0 sequence=\d+ target=runtime_rally_proof'
    WidgetRallySubmitted     = $text -match 'ArchonFactoryCanary: MapTableRallyPoint submitted=true target=runtime_rally_proof'
    ReinforcementPurchased   = $text -match 'ArchonFactoryCanary: ReinforcementPurchased team=0 cost=5 size=\d+ remainingSupply=\d+'
    ReinforcementRallyApplied = $text -match 'ArchonFactoryCanary: ReinforcementRallyApplied team=0 location='
    ReinforcementFielded     = $text -match 'ArchonFactoryCanary: ReinforcementFielded team=0 count=\d+ fieldedTotal=\d+'
    ProofCompleted           = $text -match 'ArchonFactoryCanary: RuntimeRallyProof completed=true .*rallySet=true.*rallyStored=true.*purchased=true.*fieldedAfter='
    QuitCommandHonored       = $text -match 'UGameEngine::HandleExitCommand'
    LogPath                  = $logPath
}

$result | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath $jsonPath -Encoding UTF8
$result | ConvertTo-Json -Depth 4

if (
    $exitCode -ne 0 -or
    -not $result.MatchLoopActive -or
    -not $result.ProofScheduled -or
    -not $result.SupplyTicked -or
    -not $result.RuntimeRallySubmitted -or
    -not $result.TeamRallyStored -or
    -not $result.WidgetRallySubmitted -or
    -not $result.ReinforcementPurchased -or
    -not $result.ReinforcementRallyApplied -or
    -not $result.ReinforcementFielded -or
    -not $result.ProofCompleted -or
    -not $result.QuitCommandHonored
) {
    exit 1
}
