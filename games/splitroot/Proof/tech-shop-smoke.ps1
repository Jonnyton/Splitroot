param(
    [string]$ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..\..')).Path,
    [string]$EngineRoot = 'C:\Program Files\Epic Games\UE_5.7'
)

$ErrorActionPreference = 'Stop'

$projectFile = Join-Path $ProjectRoot 'ArchonFactoryCanary.uproject'
$editorCmd = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$proofDir = Join-Path $ProjectRoot 'Saved\Proof'
$logPath = Join-Path $proofDir 'last-tech-shop-smoke.log'
$jsonPath = Join-Path $proofDir 'last-tech-shop-smoke.json'

New-Item -ItemType Directory -Force -Path $proofDir | Out-Null

$mapUrl = '/Engine/Maps/Entry?ArchonSplitrootValley?ArchonMatchLoop?game=/Script/ArchonFactoryCanary.ArchonMatchGameMode?ArchonMapId=splitroot_valley'
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
    '-ArchonRunRuntimeTechShopProof'
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

    MatchLoopActive      = $text -match 'ArchonFactoryCanary: MatchLoopActive sites=3'
    ProofScheduled       = $text -match 'ArchonFactoryCanary: RuntimeTechShopProofScheduled delay=27\.0'
    SupplyTicked         = $text -match 'ArchonFactoryCanary: SupplyTick team=0 sites=\d+ gained=\d+ total=\d+'
    ShopRowLockedBefore  = $text -match 'ArchonFactoryCanary: ShopRowLocked team=0 item=pressure_bolt_crossbow requiredTech=armory reason=missing_tech'
    BuildOrderAccepted   = $text -match 'ArchonFactoryCanary: RuntimeMapTableOrder submitted=true order=2'
    TechBuilt            = $text -match 'ArchonFactoryCanary: TechBuilt team=0 tech=armory cost=5 remainingSupply=\d+'
    TechBuildingPlaced   = $text -match 'ArchonFactoryCanary: TechBuildingPlaced team=0 tech=armory health=\d+ location='
    ShopRowUnlocked      = $text -match 'ArchonFactoryCanary: ShopRowUnlocked team=0 item=pressure_bolt_crossbow requiredTech=armory'
    TechBuildingDestroyed = $text -match 'ArchonFactoryCanary: TechBuildingDestroyed team=0 tech=armory'
    ShopRowLockedAfterTechLoss = $text -match 'ArchonFactoryCanary: ShopRowLockedOrInflatedAfterTechLoss team=0 item=pressure_bolt_crossbow requiredTech=armory reason=missing_tech'
    TableOrderAccepted   = $text -match 'ArchonFactoryCanary: TechBuildOrderedAtTable team=0 tech=armory orderAccepted=true purchaseAccepted=true'
    ProofCompleted       = $text -match 'ArchonFactoryCanary: RuntimeTechShopProof completed=true .*destroyed=true.*armoryLost=true'
    QuitCommandHonored   = $text -match 'UGameEngine::HandleExitCommand'
    LogPath              = $logPath
}

$result | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath $jsonPath -Encoding UTF8
$result | ConvertTo-Json -Depth 4

if (
    $exitCode -ne 0 -or
    -not $result.MatchLoopActive -or
    -not $result.ProofScheduled -or
    -not $result.SupplyTicked -or
    -not $result.ShopRowLockedBefore -or
    -not $result.BuildOrderAccepted -or
    -not $result.TechBuilt -or
    -not $result.TechBuildingPlaced -or
    -not $result.ShopRowUnlocked -or
    -not $result.TechBuildingDestroyed -or
    -not $result.ShopRowLockedAfterTechLoss -or
    -not $result.TableOrderAccepted -or
    -not $result.ProofCompleted -or
    -not $result.QuitCommandHonored
) {
    exit 1
}
