param(
    [string]$ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
)

$ErrorActionPreference = 'Stop'

$loopScript = Join-Path $ProjectRoot 'Proof\playtest-loop.ps1'
$pendingScript = Join-Path $ProjectRoot 'Proof\host-refresh-pending-state.ps1'
$proofDir = Join-Path $ProjectRoot 'Saved\Proof'
$harnessRoot = Join-Path $proofDir 'PlaytestLoopAdoptionProof'
$jsonPath = Join-Path $proofDir 'playtest-loop-adoption-proof.json'
$stdoutPath = Join-Path $proofDir 'playtest-loop-adoption-proof.stdout.log'
$stderrPath = Join-Path $proofDir 'playtest-loop-adoption-proof.stderr.log'

New-Item -ItemType Directory -Force -Path $proofDir | Out-Null

$resolvedProofDir = (Resolve-Path $proofDir).Path
if (Test-Path -LiteralPath $harnessRoot) {
    $oldStatePath = Join-Path $harnessRoot 'Saved\Proof\playtest-drive-session.json'
    if (Test-Path -LiteralPath $oldStatePath) {
        try {
            $oldState = Get-Content -LiteralPath $oldStatePath -Raw | ConvertFrom-Json
            Stop-Process -Id $oldState.Pid -Force -ErrorAction SilentlyContinue
        } catch {}
    }
    $resolvedHarness = (Resolve-Path $harnessRoot).Path
    if (-not $resolvedHarness.StartsWith($resolvedProofDir, [StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing to remove harness outside proof dir: $resolvedHarness"
    }
    Remove-Item -LiteralPath $resolvedHarness -Recurse -Force -Confirm:$false
}

$fakeProofDir = Join-Path $harnessRoot 'Saved\Proof'
$fakeLogDir = Join-Path $harnessRoot 'Saved\Logs'
$fakeDriveDir = Join-Path $harnessRoot 'Proof'
$fakeDllDir = Join-Path $harnessRoot 'Binaries\Win64'
New-Item -ItemType Directory -Force -Path $fakeProofDir, $fakeLogDir, $fakeDriveDir, $fakeDllDir | Out-Null

Copy-Item -LiteralPath $pendingScript -Destination (Join-Path $fakeDriveDir 'host-refresh-pending-state.ps1') -Force

$fakeDllPath = Join-Path $fakeDllDir 'UnrealEditor-ArchonFactoryCanary.dll'
Set-Content -LiteralPath $fakeDllPath -Value 'fake module dll for loop pending-refresh proof' -Encoding UTF8
(Get-Item -LiteralPath $fakeDllPath).LastWriteTimeUtc = (Get-Date).ToUniversalTime().AddMinutes(-10)

$fakeDrivePath = Join-Path $fakeDriveDir 'playtest-drive.ps1'
@'
param(
    [Parameter(Mandatory=$true)]
    [ValidateSet('launch','stop')]
    [string]$Action,
    [switch]$Force,
    [string]$MapUrl = '',
    [string]$ExtraArgs = '',
    [string]$ProjectRoot = '',
    [switch]$AllowLiveStop
)

$ErrorActionPreference = 'Stop'
if (-not $ProjectRoot) {
    $ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
}
$proofDir = Join-Path $ProjectRoot 'Saved\Proof'
$statePath = Join-Path $proofDir 'playtest-drive-session.json'
$logPath = Join-Path $proofDir 'playtest-drive.log'
$dllPath = Join-Path $ProjectRoot 'Binaries\Win64\UnrealEditor-ArchonFactoryCanary.dll'
New-Item -ItemType Directory -Force -Path $proofDir | Out-Null

if ($Action -eq 'launch') {
    $proc = Start-Process -FilePath 'powershell' -ArgumentList @('-NoProfile','-Command','Start-Sleep -Seconds 600') -WindowStyle Hidden -PassThru
    $dllUtc = 'missing'
    if (Test-Path -LiteralPath $dllPath) {
        $dllUtc = (Get-Item -LiteralPath $dllPath).LastWriteTimeUtc.ToString('o')
    }
    $processStartUtc = (Get-Date).ToUniversalTime().ToString('o')
    "ArchonFactoryCanary: BuildFingerprint moduleDllUtc=$dllUtc processStartUtc=$processStartUtc modulePath=$dllPath" | Add-Content -LiteralPath $logPath -Encoding UTF8
    'ArchonFactoryCanary: MatchConfig warmupSeconds=15.0 matchTimeLimitSeconds=900.0 scoreboardSeconds=15.0 maxCycleSeconds=930.0' | Add-Content -LiteralPath $logPath -Encoding UTF8
    'ArchonFactoryCanary: NextMatchPending nextMap=splitroot_valley countdownSeconds=3.0' | Add-Content -LiteralPath $logPath -Encoding UTF8
    [pscustomobject]@{
        Pid = $proc.Id
        LogPath = $logPath
        FakeMapUrl = $MapUrl
        FakeExtraArgs = $ExtraArgs
    } | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath $statePath -Encoding UTF8
    Write-Output "Pid=$($proc.Id)"
    Write-Output "Log=$logPath"
    exit 0
}

if ($Action -eq 'stop') {
    if (-not $AllowLiveStop) {
        Write-Output 'StopRefused=true'
        exit 2
    }
    if (Test-Path -LiteralPath $statePath) {
        $state = Get-Content -LiteralPath $statePath -Raw | ConvertFrom-Json
        Stop-Process -Id $state.Pid -Force -ErrorAction SilentlyContinue
        Remove-Item -LiteralPath $statePath -Force -ErrorAction SilentlyContinue
        Write-Output "Stopped=$($state.Pid)"
    } else {
        Write-Output 'AlreadyStopped=true'
    }
    exit 0
}
'@ | Set-Content -LiteralPath $fakeDrivePath -Encoding UTF8

Remove-Item -LiteralPath $stdoutPath, $stderrPath, $jsonPath -Force -ErrorAction SilentlyContinue

$loopArgs = @(
    '-ExecutionPolicy', 'Bypass',
    '-File', $loopScript,
    '-ProjectRoot', $harnessRoot,
    '-PollSeconds', '1',
    '-BotsPerTeam', '3',
    '-MaxIterations', '6',
    '-ForceLaunch',
    '-HarnessIgnoreExternalCurrentBuild'
)

$loopProc = Start-Process `
    -FilePath 'powershell' `
    -ArgumentList $loopArgs `
    -RedirectStandardOutput $stdoutPath `
    -RedirectStandardError $stderrPath `
    -WindowStyle Hidden `
    -PassThru

$statePath = Join-Path $fakeProofDir 'playtest-drive-session.json'
$loopEventLogPath = Join-Path $fakeProofDir 'playtest-loop.log'
$pendingPath = Join-Path $fakeProofDir 'host-refresh-pending.json'
$pendingHistoryPath = Join-Path $fakeProofDir 'host-refresh-pending-history.jsonl'
$firstAdoptedUtc = $null

$deadline = (Get-Date).AddSeconds(20)
while ((Get-Date) -lt $deadline) {
    if (Test-Path -LiteralPath $statePath) {
        $candidate = Get-Content -LiteralPath $statePath -Raw | ConvertFrom-Json
        if ($candidate.LoopAdoptedDllUtc) {
            $firstAdoptedUtc = [datetime]::Parse($candidate.LoopAdoptedDllUtc).ToUniversalTime()
            break
        }
    }
    Start-Sleep -Milliseconds 250
}

if (-not $firstAdoptedUtc) {
    Stop-Process -Id $loopProc.Id -Force -ErrorAction SilentlyContinue
    throw 'Loop adoption proof did not observe first session evidence.'
}

$newDllUtc = (Get-Date).ToUniversalTime().AddMinutes(5)
(Get-Item -LiteralPath $fakeDllPath).LastWriteTimeUtc = $newDllUtc

if (-not $loopProc.WaitForExit(45000)) {
    Stop-Process -Id $loopProc.Id -Force -ErrorAction SilentlyContinue
    throw 'Loop adoption proof timed out waiting for finite loop process.'
}
$loopProc.Refresh()

$finalSession = $null
if (Test-Path -LiteralPath $statePath) {
    $finalSession = Get-Content -LiteralPath $statePath -Raw | ConvertFrom-Json
}

$eventText = if (Test-Path -LiteralPath $loopEventLogPath) { Get-Content -LiteralPath $loopEventLogPath -Raw } else { '' }
$pendingState = if (Test-Path -LiteralPath $pendingPath) { Get-Content -LiteralPath $pendingPath -Raw | ConvertFrom-Json } else { $null }
$pendingHistoryText = if (Test-Path -LiteralPath $pendingHistoryPath) { Get-Content -LiteralPath $pendingHistoryPath -Raw } else { '' }

$launchCount = ([regex]::Matches($eventText, 'LoopEvent: bot match session launched')).Count
$pendingDetectedCount = ([regex]::Matches($eventText, 'LoopEvent: newer build detected - live session kept running')).Count
$oldRecycleCount = ([regex]::Matches($eventText, 'LoopEvent: newer build detected - session recycled')).Count

if ($finalSession -and $finalSession.Pid) {
    Stop-Process -Id $finalSession.Pid -Force -ErrorAction SilentlyContinue
}

$result = [pscustomobject]@{
    ProjectRoot = $ProjectRoot
    HarnessRoot = $harnessRoot
    LoopProcessExited = $loopProc.HasExited
    LoopExitCode = if ($null -eq $loopProc.ExitCode) { 0 } else { $loopProc.ExitCode }
    FirstAdoptedDllUtc = $firstAdoptedUtc.ToString('o')
    NewDllUtc = $newDllUtc.ToString('o')
    LaunchCount = $launchCount
    PendingDetectedCount = $pendingDetectedCount
    OldRecycleCount = $oldRecycleCount
    SessionStayedOnOriginalDll = [bool]($finalSession -and $finalSession.LoopAdoptedDllUtc -eq $firstAdoptedUtc.ToString('o'))
    PendingStateWritten = [bool]($pendingState -and $pendingState.Pending)
    PendingStateReason = if ($pendingState) { $pendingState.Reason } else { '' }
    PendingStateSupervisorOwnsHost = if ($pendingState) { $pendingState.SupervisorOwnsHost } else { $null }
    PendingHistoryRecorded = $pendingHistoryText -match '"Pending":true'
    StdoutPath = $stdoutPath
    StderrPath = $stderrPath
    LoopEventLogPath = $loopEventLogPath
    PendingStatePath = $pendingPath
    PendingHistoryPath = $pendingHistoryPath
}

$result | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $jsonPath -Encoding UTF8
$result | ConvertTo-Json -Depth 6

if (
    -not $result.LoopProcessExited -or
    $result.LoopExitCode -ne 0 -or
    $result.LaunchCount -ne 1 -or
    $result.PendingDetectedCount -lt 1 -or
    $result.OldRecycleCount -ne 0 -or
    -not $result.SessionStayedOnOriginalDll -or
    -not $result.PendingStateWritten -or
    $result.PendingStateReason -ne 'newer_cxx_build_detected_live_session_kept_running' -or
    $result.PendingStateSupervisorOwnsHost -ne $false -or
    -not $result.PendingHistoryRecorded
) {
    exit 1
}
