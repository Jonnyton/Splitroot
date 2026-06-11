param(
    [string]$ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
)

$ErrorActionPreference = 'Stop'

$proofDir = Join-Path $ProjectRoot 'Saved\Proof'
$harnessRoot = Join-Path $proofDir 'HostSupervisorHotReloadProof'
$jsonPath = Join-Path $proofDir 'last-host-supervisor-hot-reload-proof.json'
$stdoutPath = Join-Path $proofDir 'host-supervisor-hot-reload-proof.stdout.log'
$stderrPath = Join-Path $proofDir 'host-supervisor-hot-reload-proof.stderr.log'
$resolvedProofDir = (Resolve-Path $proofDir).Path

New-Item -ItemType Directory -Force -Path $proofDir | Out-Null

function Get-CurrentBuildProcesses {
    try {
        return @(Get-CimInstance Win32_Process |
            Where-Object {
                $_.Name -eq 'UnrealEditor.exe' -and
                $_.CommandLine -match 'ArchonFactoryCanary\.uproject'
            } |
            Sort-Object ProcessId |
            Select-Object ProcessId, Name, CommandLine)
    } catch {
        return @()
    }
}

function Stop-HarnessProcessFromState([string]$StatePath) {
    if (-not (Test-Path -LiteralPath $StatePath -PathType Leaf)) { return }
    try {
        $state = Get-Content -LiteralPath $StatePath -Raw | ConvertFrom-Json
        if ($state.Pid -and (Get-Process -Id $state.Pid -ErrorAction SilentlyContinue)) {
            Stop-Process -Id $state.Pid -Force -ErrorAction SilentlyContinue
        }
    } catch {}
}

if (Test-Path -LiteralPath $harnessRoot) {
    $oldStatePath = Join-Path $harnessRoot 'Saved\Proof\playtest-drive-session.json'
    Stop-HarnessProcessFromState $oldStatePath
    $resolvedHarness = (Resolve-Path $harnessRoot).Path
    if (-not $resolvedHarness.StartsWith($resolvedProofDir, [StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing to remove harness outside proof dir: $resolvedHarness"
    }
    Remove-Item -LiteralPath $resolvedHarness -Recurse -Force -Confirm:$false
}

$fakeProofDir = Join-Path $harnessRoot 'Saved\Proof'
$fakeDriveDir = Join-Path $harnessRoot 'Proof'
$fakeDllDir = Join-Path $harnessRoot 'Binaries\Win64'
$fakeSourceDir = Join-Path $harnessRoot 'Source\ArchonFactoryCanary\Private'
New-Item -ItemType Directory -Force -Path $fakeProofDir, $fakeDriveDir, $fakeDllDir, $fakeSourceDir | Out-Null

Copy-Item -LiteralPath (Join-Path $ProjectRoot 'Proof\host-refresh-pending-state.ps1') -Destination (Join-Path $fakeDriveDir 'host-refresh-pending-state.ps1') -Force

$oldDllUtc = (Get-Date).ToUniversalTime().AddMinutes(-30)
$sourceUtc = (Get-Date).ToUniversalTime().AddMinutes(-5)
$fakeDllPath = Join-Path $fakeDllDir 'UnrealEditor-ArchonFactoryCanary.dll'
$fakeSourcePath = Join-Path $fakeSourceDir 'ArchonHotReloadProof.cpp'
Set-Content -LiteralPath $fakeDllPath -Value 'fake old module dll for host supervisor hot reload proof' -Encoding UTF8
(Get-Item -LiteralPath $fakeDllPath).LastWriteTimeUtc = $oldDllUtc
Set-Content -LiteralPath $fakeSourcePath -Value '// fake source newer than module' -Encoding UTF8
(Get-Item -LiteralPath $fakeSourcePath).LastWriteTimeUtc = $sourceUtc

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
    "ArchonFactoryCanary: BuildFingerprint moduleDllUtc=$dllUtc processStartUtc=$processStartUtc modulePath=$dllPath version=vproof effectiveUtc=$processStartUtc" | Add-Content -LiteralPath $logPath -Encoding UTF8
    "ArchonFactoryCanary: BuildVersionEffective version=vproof effectiveUtc=$processStartUtc moduleDllUtc=$dllUtc" | Add-Content -LiteralPath $logPath -Encoding UTF8
    'ArchonFactoryCanary: MatchLoopActive sites=3 proof=true' | Add-Content -LiteralPath $logPath -Encoding UTF8
    'ArchonFactoryCanary: MatchEnd winner=0 reason=proof_boundary' | Add-Content -LiteralPath $logPath -Encoding UTF8
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

$fakeBuildPath = Join-Path $fakeDriveDir 'fake-build-at-boundary.ps1'
@'
param(
    [string]$ProjectRoot = ''
)

$ErrorActionPreference = 'Stop'
if (-not $ProjectRoot) {
    $ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
}
$proofDir = Join-Path $ProjectRoot 'Saved\Proof'
$dllPath = Join-Path $ProjectRoot 'Binaries\Win64\UnrealEditor-ArchonFactoryCanary.dll'
New-Item -ItemType Directory -Force -Path $proofDir | Out-Null
$newUtc = (Get-Date).ToUniversalTime().AddMinutes(2)
Set-Content -LiteralPath $dllPath -Value "fake rebuilt module dll at $($newUtc.ToString('o'))" -Encoding UTF8
(Get-Item -LiteralPath $dllPath).LastWriteTimeUtc = $newUtc
[pscustomobject]@{
    ProjectRoot = $ProjectRoot
    BuildExitCode = 0
    ModuleDllUtc = $newUtc.ToString('o')
} | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $proofDir 'fake-boundary-build.json') -Encoding UTF8
exit 0
'@ | Set-Content -LiteralPath $fakeBuildPath -Encoding UTF8

Remove-Item -LiteralPath $stdoutPath, $stderrPath, $jsonPath -Force -ErrorAction SilentlyContinue

$beforeLive = Get-CurrentBuildProcesses
$fakeLaunch = powershell -ExecutionPolicy Bypass -File $fakeDrivePath `
    -Action launch `
    -ProjectRoot $harnessRoot `
    -MapUrl '/Engine/Maps/Entry?ArchonSplitrootValley?ArchonMatchLoop?listen' `
    -ExtraArgs '-ArchonBotMatch -ArchonBotCountPerTeam=3' `
    -Force 2>&1 | Out-String
if ($LASTEXITCODE -ne 0) {
    throw "Fake host launch failed: $fakeLaunch"
}

$seedPlaytestStatePath = Join-Path $fakeProofDir 'playtest-drive-session.json'
$seedPlaytestState = Get-Content -LiteralPath $seedPlaytestStatePath -Raw | ConvertFrom-Json
$seedSupervisorStatePath = Join-Path $fakeProofDir 'host-supervisor-session.json'
[pscustomobject]@{
    SupervisorOwnsHost = $true
    Pid = [int]$seedPlaytestState.Pid
    StartedUtc = (Get-Date).ToUniversalTime().ToString('o')
    MapUrl = '/Engine/Maps/Entry?ArchonSplitrootValley?ArchonMatchLoop?listen'
    HostEndpoint = 'listen://localhost'
    PlaytestStatePath = $seedPlaytestStatePath
    PlaytestLogPath = Join-Path $fakeProofDir 'playtest-drive.log'
    AllowBoundaryRefresh = $true
    BuildAtBoundary = $true
    BuildScriptPath = $fakeBuildPath
} | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $seedSupervisorStatePath -Encoding UTF8

$supervisorArgs = @(
    '-ExecutionPolicy', 'Bypass',
    '-File', (Join-Path $ProjectRoot 'Proof\host-supervisor-loop.ps1'),
    '-ProjectRoot', $harnessRoot,
    '-PollSeconds', '1',
    '-MaxIterations', '5',
    '-BotsPerTeam', '3',
    '-AllowBoundaryRefresh',
    '-BuildAtBoundary',
    '-BuildScriptPath', $fakeBuildPath,
    '-ForceLaunch',
    '-HarnessIgnoreExternalCurrentBuild'
)

$supervisorProc = Start-Process `
    -FilePath 'powershell' `
    -ArgumentList $supervisorArgs `
    -RedirectStandardOutput $stdoutPath `
    -RedirectStandardError $stderrPath `
    -WindowStyle Hidden `
    -PassThru

if (-not $supervisorProc.WaitForExit(30000)) {
    Stop-Process -Id $supervisorProc.Id -Force -ErrorAction SilentlyContinue
    throw 'Host supervisor hot-reload proof timed out.'
}
$supervisorProc.Refresh()

$afterLive = Get-CurrentBuildProcesses
$refreshProofPath = Join-Path $fakeProofDir 'last-host-supervisor-refresh.json'
$pendingPath = Join-Path $fakeProofDir 'host-refresh-pending.json'
$pendingHistoryPath = Join-Path $fakeProofDir 'host-refresh-pending-history.jsonl'
$supervisorStatePath = Join-Path $fakeProofDir 'host-supervisor-session.json'
$refreshProof = if (Test-Path -LiteralPath $refreshProofPath) { Get-Content -LiteralPath $refreshProofPath -Raw | ConvertFrom-Json } else { $null }
$pendingState = if (Test-Path -LiteralPath $pendingPath) { Get-Content -LiteralPath $pendingPath -Raw | ConvertFrom-Json } else { $null }
$pendingHistoryText = if (Test-Path -LiteralPath $pendingHistoryPath) { Get-Content -LiteralPath $pendingHistoryPath -Raw } else { '' }
$supervisorState = if (Test-Path -LiteralPath $supervisorStatePath) { Get-Content -LiteralPath $supervisorStatePath -Raw | ConvertFrom-Json } else { $null }

if ($supervisorState -and $supervisorState.Pid) {
    Stop-Process -Id $supervisorState.Pid -Force -ErrorAction SilentlyContinue
}

$beforeLivePids = @($beforeLive | ForEach-Object { [int]$_.ProcessId })
$afterLivePids = @($afterLive | ForEach-Object { [int]$_.ProcessId })
$externalUnchanged = @($beforeLivePids | Where-Object { $afterLivePids -contains $_ }).Count -eq $beforeLivePids.Count
$historySawDetectedPending = $pendingHistoryText -match 'source_newer_than_module_boundary_build_pending' -and $pendingHistoryText -match 'host_refreshing_build_reconnecting'

$beforeBuildUtc = if ($refreshProof) { [datetime]::Parse($refreshProof.ModuleDllUtcBeforeBuild) } else { $null }
$afterBuildUtc = if ($refreshProof) { [datetime]::Parse($refreshProof.ModuleDllUtcAfterBuild) } else { $null }

$result = [pscustomobject]@{
    ProjectRoot = $ProjectRoot
    HarnessRoot = $harnessRoot
    SupervisorExitCode = if ($null -eq $supervisorProc.ExitCode) { 0 } else { $supervisorProc.ExitCode }
    ProcessBoundary = if ($refreshProof) { $refreshProof.ProcessBoundary } else { 'missing' }
    BoundaryPidChanged = [bool]($refreshProof -and $refreshProof.BeforePid -and $refreshProof.AfterPid -and $refreshProof.BeforePid -ne $refreshProof.AfterPid)
    PendingStateEmitted = [bool]$historySawDetectedPending
    FinalPendingCleared = [bool]($pendingState -and -not $pendingState.Pending)
    BuildAttempted = [bool]($refreshProof -and $refreshProof.BuildAttempted)
    BuildSucceeded = [bool]($refreshProof -and $refreshProof.BuildSucceeded)
    ModuleTimestampAdvanced = [bool]($beforeBuildUtc -and $afterBuildUtc -and $afterBuildUtc -gt $beforeBuildUtc)
    ClientReconnectPlaceholder = [bool]($refreshProof -and $refreshProof.ClientReconnectPlaceholder)
    SupervisorOwnedOnly = [bool]($refreshProof -and -not $refreshProof.TouchedExternalLiveProcess)
    ExternalCurrentBuildPidsBefore = $beforeLivePids
    ExternalCurrentBuildPidsAfter = $afterLivePids
    ExternalCurrentBuildStillAlive = $externalUnchanged
    RefreshProofPath = $refreshProofPath
    PendingStatePath = $pendingPath
    PendingHistoryPath = $pendingHistoryPath
    SupervisorStatePath = $supervisorStatePath
    StdoutPath = $stdoutPath
    StderrPath = $stderrPath
}

$result | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $jsonPath -Encoding UTF8
$result | ConvertTo-Json -Depth 8

if (
    $result.SupervisorExitCode -ne 0 -or
    $result.ProcessBoundary -ne 'fresh_process_relaunch' -or
    -not $result.BoundaryPidChanged -or
    -not $result.PendingStateEmitted -or
    -not $result.FinalPendingCleared -or
    -not $result.BuildAttempted -or
    -not $result.BuildSucceeded -or
    -not $result.ModuleTimestampAdvanced -or
    -not $result.ClientReconnectPlaceholder -or
    -not $result.SupervisorOwnedOnly -or
    -not $result.ExternalCurrentBuildStillAlive
) {
    exit 1
}
