param(
    [string]$ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..\..')).Path,
    [string]$EngineRoot = 'C:\Program Files\Epic Games\UE_5.7',
    [int]$TimeoutSeconds = 90
)

$ErrorActionPreference = 'Stop'

$projectFile = Join-Path $ProjectRoot 'ArchonFactoryCanary.uproject'
$editorCmd = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$proofDir = Join-Path $ProjectRoot 'Saved\Proof'
$logPath = Join-Path $proofDir 'last-host-loop-forever-smoke.log'
$jsonPath = Join-Path $proofDir 'last-host-loop-forever-smoke.json'

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

# Proof restart without ArchonMatchProofQuitAfterTravel. The script kills
# the process only after the second match boots, proving the host loop did
# not self-quit at the match boundary.
$mapUrl = '/Engine/Maps/Entry?ArchonSplitrootValley?ArchonMatchLoop?ArchonMatchProof?ArchonMatchProofRestart?ArchonBotMatch?game=/Script/ArchonFactoryCanary.ArchonMatchGameMode?ArchonMapId=splitroot_valley?listen'
$argsList = @(
    $projectFile,
    $mapUrl,
    '-game',
    '-NullRHI',
    '-NoSound',
    '-NoSplash',
    '-unattended',
    '-nop4',
    "-abslog=$logPath"
)

$process = Start-Process -FilePath $editorCmd -ArgumentList $argsList -PassThru -WindowStyle Hidden
$deadline = (Get-Date).AddSeconds($TimeoutSeconds)
$text = ''
$completed = $false

while ((Get-Date) -lt $deadline) {
    if (Test-Path -LiteralPath $logPath) {
        $text = Read-SmokeLogText
        $matchLoopCount = ([regex]::Matches($text, 'ArchonFactoryCanary: MatchLoopActive sites=3')).Count
        if ($matchLoopCount -ge 2) {
            $completed = $true
            break
        }
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

if (Test-Path -LiteralPath $logPath) {
    $text = Read-SmokeLogText
}

$matchLoopCount = ([regex]::Matches($text, 'ArchonFactoryCanary: MatchLoopActive sites=3')).Count
$botSpawnCount = ([regex]::Matches($text, 'ArchonFactoryCanary: BotMatchSpawned')).Count
$fingerprintMatches = [regex]::Matches($text, 'ArchonFactoryCanary: BuildFingerprint moduleDllUtc=\S+ processStartUtc=(\S+) modulePath=')
$processStartUtcValues = @($fingerprintMatches | ForEach-Object { $_.Groups[1].Value })
$uniqueProcessStartUtcValues = @($processStartUtcValues | Select-Object -Unique)
$reusedSameProcess = $fingerprintMatches.Count -ge 2 -and $uniqueProcessStartUtcValues.Count -eq 1
$processBoundary = if ($reusedSameProcess) { 'in_process_server_travel' } elseif ($fingerprintMatches.Count -ge 2) { 'fresh_process_relaunch' } else { 'unknown' }

$result = [pscustomobject]@{
    ProjectRoot = $ProjectRoot
    EngineRoot = $EngineRoot
    MapUrl = $mapUrl
    ProcessExitCode = $process.ExitCode
    KilledByProofHarness = $completed
    ProcessBoundary = $processBoundary
    BuildFingerprintCount = $fingerprintMatches.Count
    ProcessStartUtcValues = $processStartUtcValues
    UniqueProcessStartUtcValues = $uniqueProcessStartUtcValues
    ReusedSameProcessAcrossTravel = $reusedSameProcess

    FirstMatchLoopActive      = $text -match 'ArchonFactoryCanary: MatchLoopActive sites=3 proof=true'
    FirstMatchEnded           = $text -match 'ArchonFactoryCanary: MatchEnd winner='
    NextMatchPending          = $text -match 'ArchonFactoryCanary: NextMatchPending nextMap=splitroot_valley countdownSeconds=3\.0'
    TravelRequested           = $text -match 'ArchonFactoryCanary: TravelRequested reason=match_complete'
    NextMatchLoading          = $text -match 'ArchonFactoryCanary: NextMatchLoading map=splitroot_valley'
    SecondMatchLoopActive     = $matchLoopCount -ge 2
    BotFlagSurvivedTravel     = $botSpawnCount -ge 2
    NoProofQuitCarried        = $text -notmatch 'ArchonFactoryCanary: MatchProofQuitScheduled'
    NoQuitCommandBeforeHarness = $text -notmatch 'UGameEngine::HandleExitCommand'
    LogPath                   = $logPath
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
    -not $result.NoProofQuitCarried -or
    -not $result.NoQuitCommandBeforeHarness
) {
    exit 1
}
