param(
    [string]$ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..\..')).Path,
    [string]$EngineRoot = 'C:\Program Files\Epic Games\UE_5.7',
    [int]$TimeoutSeconds = 110
)

$ErrorActionPreference = 'Stop'

$projectFile = Join-Path $ProjectRoot 'ArchonFactoryCanary.uproject'
$editor = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor.exe'
$proofDir = Join-Path $ProjectRoot 'Saved\Proof'
$logPath = Join-Path $proofDir 'last-rendered-host-loop-forever-smoke.log'
$jsonPath = Join-Path $proofDir 'last-rendered-host-loop-forever-smoke.json'

New-Item -ItemType Directory -Force -Path $proofDir | Out-Null
if (Test-Path -LiteralPath $logPath) {
    Remove-Item -LiteralPath $logPath -Force
}

function Read-SmokeLogText {
    if (-not (Test-Path -LiteralPath $logPath)) {
        return ''
    }
    $content = Get-Content -LiteralPath $logPath -Raw
    if ($null -eq $content) {
        return ''
    }
    return $content
}

# Rendered proof for Jonathan's visible forever-host problem. This uses the
# real editor game executable and RHI path, not UnrealEditor-Cmd/-NullRHI.
# ArchonMatchProofRestart shortens the first match and then must travel into
# a second match without a self-quit. The harness kills only after observing
# the second match boot.
$mapUrl = '/Engine/Maps/Entry?ArchonSplitrootValley?ArchonMatchLoop?ArchonMatchProof?ArchonMatchProofRestart?ArchonBotMatch?game=/Script/ArchonFactoryCanary.ArchonMatchGameMode?ArchonMapId=splitroot_valley?listen'
$argsList = @(
    $projectFile,
    $mapUrl,
    '-game',
    '-windowed',
    '-ResX=640',
    '-ResY=360',
    '-NoSound',
    '-NoSplash',
    '-nop4',
    "-abslog=$logPath"
)

$process = Start-Process -FilePath $editor -ArgumentList $argsList -PassThru -WindowStyle Minimized
$deadline = (Get-Date).AddSeconds($TimeoutSeconds)
$text = ''
$completed = $false

while ((Get-Date) -lt $deadline) {
    $text = Read-SmokeLogText
    $matchLoopCount = ([regex]::Matches($text, 'ArchonFactoryCanary: MatchLoopActive sites=3')).Count
    if ($matchLoopCount -ge 2) {
        $completed = $true
        break
    }

    if ($process.HasExited) {
        break
    }
    Start-Sleep -Milliseconds 500
}

if (-not $process.HasExited) {
    Stop-Process -Id $process.Id -Force
    $process.WaitForExit()
}

$text = Read-SmokeLogText
$matchLoopCount = ([regex]::Matches($text, 'ArchonFactoryCanary: MatchLoopActive sites=3')).Count
$botSpawnCount = ([regex]::Matches($text, 'ArchonFactoryCanary: BotMatchSpawned')).Count
$fingerprintMatches = [regex]::Matches($text, 'ArchonFactoryCanary: BuildFingerprint moduleDllUtc=\S+ processStartUtc=(\S+) modulePath=')
$processStartUtcValues = @($fingerprintMatches | ForEach-Object { $_.Groups[1].Value })
$uniqueProcessStartUtcValues = @($processStartUtcValues | Select-Object -Unique)

$result = [pscustomobject]@{
    ProjectRoot = $ProjectRoot
    EngineRoot = $EngineRoot
    MapUrl = $mapUrl
    ProcessExitCode = $process.ExitCode
    KilledByProofHarness = $completed

    RenderedRhiPath = $true
    FirstMatchLoopActive = $text -match 'ArchonFactoryCanary: MatchLoopActive sites=3 proof=true'
    FirstMatchEnded = $text -match 'ArchonFactoryCanary: MatchEnd winner='
    NextMatchPending = $text -match 'ArchonFactoryCanary: NextMatchPending nextMap=splitroot_valley countdownSeconds=3\.0'
    TravelRequested = $text -match 'ArchonFactoryCanary: TravelRequested reason=match_complete'
    NextMatchLoading = $text -match 'ArchonFactoryCanary: NextMatchLoading map=splitroot_valley'
    SecondMatchLoopActive = $matchLoopCount -ge 2
    BotFlagSurvivedTravel = $botSpawnCount -ge 2
    BuildFingerprintCount = $fingerprintMatches.Count
    ProcessStartUtcValues = $processStartUtcValues
    UniqueProcessStartUtcValues = $uniqueProcessStartUtcValues
    ReusedSameProcessAcrossTravel = $fingerprintMatches.Count -ge 2 -and $uniqueProcessStartUtcValues.Count -eq 1

    RecorderSkippedByDefault = $text -match 'ArchonFactoryCanary: PlaytestRecorderSkipped reason=opt_in_flag_missing'
    NoRecorderStarted = $text -notmatch 'ArchonFactoryCanary: PlaytestRecorderStarted'
    NoRecorderShot = $text -notmatch 'ArchonFactoryCanary: PlaytestRecorderShot'
    NoFatalError = $text -notmatch 'Fatal error!'
    NoQuitCommandBeforeHarness = $text -notmatch 'UGameEngine::HandleExitCommand'
    LogPath = $logPath
}

$result | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath $jsonPath -Encoding UTF8
$result | ConvertTo-Json -Depth 4

if (
    -not $completed -or
    -not $result.FirstMatchLoopActive -or
    -not $result.FirstMatchEnded -or
    -not $result.NextMatchPending -or
    -not $result.TravelRequested -or
    -not $result.NextMatchLoading -or
    -not $result.SecondMatchLoopActive -or
    -not $result.BotFlagSurvivedTravel -or
    -not $result.ReusedSameProcessAcrossTravel -or
    -not $result.RecorderSkippedByDefault -or
    -not $result.NoRecorderStarted -or
    -not $result.NoRecorderShot -or
    -not $result.NoFatalError -or
    -not $result.NoQuitCommandBeforeHarness
) {
    exit 1
}
