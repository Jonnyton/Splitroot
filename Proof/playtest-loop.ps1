<#
.SYNOPSIS
Perpetual background playtest (Jonathan 2026-06-10): keeps an all-AI
bot match running match after match while work continues. The live session
is shared telemetry/playtest infrastructure and must not be stopped by this
loop for build adoption.

Intended to run under a persistent monitor. Emits one line per ACTION
(launch / pending build / match rotation observed); silence means the match
is just playing.

Cooperation rules:
- `Saved/Proof/build-lock.flag` present => keep the live session running
  and log that the build must wait for an explicit host-safe refresh.
- Respects the human-idle gate via playtest-drive launch (never pops
  the boot window while Jonathan is active; retries next poll).
- Jonathan can jump in/out of the running session at any time — the
  loop does not stop for build-locks or newer DLLs; it records pending
  refresh evidence and waits for an explicit host-safe refresh.
#>
param(
    [string]$ProjectRoot = '',
    [int]$PollSeconds = 30,
    [int]$BotsPerTeam = 8,
    [int]$MaxIterations = 0,
    [switch]$ForceLaunch,
    [switch]$HarnessIgnoreExternalCurrentBuild
)
$ErrorActionPreference = 'Continue'
if (-not $ProjectRoot) {
    $ProjectRoot = (Resolve-Path (Join-Path (Split-Path -Parent $MyInvocation.MyCommand.Path) '..')).Path
}
$drive = Join-Path $ProjectRoot 'Proof\playtest-drive.ps1'
$proofDir = Join-Path $ProjectRoot 'Saved\Proof'
$statePath = Join-Path $proofDir 'playtest-drive-session.json'
$lockPath = Join-Path $proofDir 'build-lock.flag'
$dllPath = Join-Path $ProjectRoot 'Binaries\Win64\UnrealEditor-ArchonFactoryCanary.dll'
$logPath = Join-Path $proofDir 'playtest-drive.log'
$liveLogPath = Join-Path $ProjectRoot 'Saved\Logs\ArchonFactoryCanary.log'
$loopEventLogPath = Join-Path $proofDir 'playtest-loop.log'
$pendingRefreshScript = Join-Path $ProjectRoot 'Proof\host-refresh-pending-state.ps1'
$matchUrl = '/Engine/Maps/Entry?ArchonSplitrootValley?ArchonMatchLoop?game=/Script/ArchonFactoryCanary.ArchonMatchGameMode?ArchonMapId=splitroot_valley'

New-Item -ItemType Directory -Force -Path $proofDir | Out-Null

$sessionDllTime = $null
$seenTravels = 0
$iteration = 0
$buildLockKeepAliveLogged = $false
$reportedPendingDllUtc = $null

function Write-LoopEvent([string]$Message) {
    $line = "LoopEvent: $Message"
    Write-Output $line
    $stamp = (Get-Date).ToUniversalTime().ToString('o')
    "$stamp $line" | Add-Content -LiteralPath $loopEventLogPath -Encoding UTF8
}

function Test-MaxIterationsReached {
    return ($MaxIterations -gt 0 -and $iteration -ge $MaxIterations)
}

function Update-LoopSessionEvidence($AdoptedDllTime) {
    if (-not (Test-Path -LiteralPath $statePath)) { return }
    try {
        $state = Get-Content -LiteralPath $statePath -Raw | ConvertFrom-Json
        $adoptedDllUtc = $null
        if ($null -ne $AdoptedDllTime) {
            $adoptedDllTimeValue = ([datetime]$AdoptedDllTime).ToUniversalTime()
            if ($adoptedDllTimeValue.Ticks -gt 0) {
                $adoptedDllUtc = $adoptedDllTimeValue.ToString('o')
            }
        }
        $fingerprint = if ($adoptedDllUtc) { "moduleDllUtc=$adoptedDllUtc" } else { 'moduleDllUtc=missing' }
        $state | Add-Member -NotePropertyName LoopLaunchedUtc -NotePropertyValue ((Get-Date).ToUniversalTime().ToString('o')) -Force
        $state | Add-Member -NotePropertyName LoopAdoptedDllUtc -NotePropertyValue $adoptedDllUtc -Force
        $state | Add-Member -NotePropertyName LoopBuildFingerprint -NotePropertyValue $fingerprint -Force
        $state | Add-Member -NotePropertyName LoopMapUrl -NotePropertyValue $matchUrl -Force
        $state | Add-Member -NotePropertyName LoopBotsPerTeam -NotePropertyValue $BotsPerTeam -Force
        $state | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $statePath -Encoding UTF8
    } catch {
        Write-LoopEvent "session evidence update failed: $($_.Exception.Message)"
    }
}

function Get-TextOrEmpty([string]$Path) {
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) { return '' }
    $text = Get-Content -LiteralPath $Path -Raw
    if ($null -eq $text) { return '' }
    return $text
}

function Get-LiveSessionDllTime {
    $text = Get-TextOrEmpty $liveLogPath
    $matches = [regex]::Matches($text, 'ArchonFactoryCanary: BuildFingerprint moduleDllUtc=(\S+)')
    if ($matches.Count -eq 0) { return $null }
    try {
        return [datetime]::Parse($matches[$matches.Count - 1].Groups[1].Value).ToUniversalTime()
    } catch {
        return $null
    }
}

function Write-HostRefreshPending(
    [string]$Reason,
    [int]$ProcessId,
    [datetime]$CurrentDllTime,
    [datetime]$PendingDllTime
) {
    if (-not (Test-Path -LiteralPath $pendingRefreshScript -PathType Leaf)) {
        Write-LoopEvent "pending refresh script missing path=$pendingRefreshScript"
        return
    }
    $currentUtc = if ($CurrentDllTime -and $CurrentDllTime.Ticks -gt 0) { $CurrentDllTime.ToUniversalTime().ToString('o') } else { '' }
    $pendingUtc = if ($PendingDllTime -and $PendingDllTime.Ticks -gt 0) { $PendingDllTime.ToUniversalTime().ToString('o') } else { '' }
    $args = @(
        '-ExecutionPolicy', 'Bypass',
        '-File', $pendingRefreshScript,
        '-Action', 'write',
        '-ProjectRoot', $ProjectRoot,
        '-Reason', $Reason,
        '-CurrentProcessId', ([string]$ProcessId),
        '-CurrentModuleDllUtc', $currentUtc,
        '-PendingModuleDllUtc', $pendingUtc,
        '-SupervisorOwnsHost', 'False',
        '-SupervisorMode', 'live_loop_observing_existing_session',
        '-ClientState', 'host_refresh_pending_reconnect_placeholder',
        '-ObservedLogPath', $liveLogPath
    )
    powershell @args | Out-Null
}

function Get-SessionPid {
    if (Test-Path -LiteralPath $statePath) {
        try {
            $state = Get-Content -LiteralPath $statePath -Raw | ConvertFrom-Json
            if (Get-Process -Id $state.Pid -ErrorAction SilentlyContinue) { return $state.Pid }
        } catch {}
    }
    if ($HarnessIgnoreExternalCurrentBuild) {
        return $null
    }
    try {
        $existing = Get-CimInstance Win32_Process |
            Where-Object {
                $_.Name -eq 'UnrealEditor.exe' -and
                $_.CommandLine -match 'ArchonFactoryCanary\.uproject'
            } |
            Sort-Object ProcessId |
            Select-Object -First 1
        if ($existing) { return [int]$existing.ProcessId }
    } catch {}
    return $null
}

while ($true) {
    $iteration++
    $sessionPid = Get-SessionPid

    if (Test-Path -LiteralPath $lockPath) {
        if ($sessionPid) {
            if (-not $buildLockKeepAliveLogged) {
                Write-LoopEvent "build-lock present - live session kept running; build must wait for explicit host-safe refresh"
                $currentDll = if ($sessionDllTime) { $sessionDllTime } else { Get-LiveSessionDllTime }
                $pendingDll = (Get-Item -LiteralPath $dllPath -ErrorAction SilentlyContinue).LastWriteTimeUtc
                Write-HostRefreshPending 'build_lock_present_live_session_kept_running' $sessionPid $currentDll $pendingDll
                $buildLockKeepAliveLogged = $true
            }
        }
        if (Test-MaxIterationsReached) { break }
        Start-Sleep -Seconds $PollSeconds
        continue
    }
    $buildLockKeepAliveLogged = $false

    if ($sessionPid) {
        if (-not $sessionDllTime) {
            $sessionDllTime = Get-LiveSessionDllTime
        }
        # Newer DLLs are metadata, not permission to kill the live game.
        # Keep the session running and record that a host-safe refresh is pending.
        $dllTime = (Get-Item -LiteralPath $dllPath -ErrorAction SilentlyContinue).LastWriteTimeUtc
        if ($sessionDllTime -and $dllTime -and $dllTime -gt $sessionDllTime) {
            $dllStamp = $dllTime.ToString('o')
            if ($reportedPendingDllUtc -ne $dllStamp) {
                Write-LoopEvent "newer build detected - live session kept running oldBuild=$($sessionDllTime.ToString('o')) newBuild=$dllStamp pendingHostSafeRefresh=true"
                Write-HostRefreshPending 'newer_cxx_build_detected_live_session_kept_running' $sessionPid $sessionDllTime $dllTime
                $reportedPendingDllUtc = $dllStamp
            }
        }

        # One line per match rotation so the event stream tells match
        # cadence without spam.
        if (Test-Path -LiteralPath $logPath) {
            $travels = ([regex]::Matches((Get-Content -LiteralPath $logPath -Raw), 'ArchonFactoryCanary: TravelingTo map=')).Count
            if ($travels -gt $seenTravels) {
                Write-LoopEvent "match rotated (total rotations this session: $travels)"
                $seenTravels = $travels
            }
        }
        if (Test-MaxIterationsReached) { break }
        Start-Sleep -Seconds $PollSeconds
        continue
    }

    # Archive the previous session's marker log BEFORE launch wipes it —
    # match history (winners, durations, kill feeds) is the morning's
    # replay metadata.
    if (Test-Path -LiteralPath $logPath) {
        $archiveDir = Join-Path $ProjectRoot 'Saved\Proof\MatchLogs'
        New-Item -ItemType Directory -Force -Path $archiveDir | Out-Null
        $stamp = (Get-Item -LiteralPath $logPath).LastWriteTime.ToString('yyyyMMdd-HHmmss')
        Copy-Item -LiteralPath $logPath -Destination (Join-Path $archiveDir "session-$stamp.log") -Force
    }

    # No session: launch (the drive's idle gate throws while Jonathan is
    # active; swallow and retry next poll).
    $launchArgs = @(
        '-ExecutionPolicy', 'Bypass',
        '-File', $drive,
        '-Action', 'launch',
        '-MapUrl', $matchUrl,
        '-ExtraArgs', "-ArchonBotMatch -ArchonBotCountPerTeam=$BotsPerTeam"
    )
    if ($ForceLaunch) { $launchArgs += '-Force' }
    $out = powershell @launchArgs 2>&1 | Out-String
    if ($LASTEXITCODE -eq 0) {
        # Close the launch/lock race without stopping the live session.
        if (Test-Path -LiteralPath $lockPath) {
            Write-LoopEvent "launch raced a build-lock - live session kept running; build must wait for explicit host-safe refresh"
        } else {
            $sessionDllTime = (Get-Item -LiteralPath $dllPath -ErrorAction SilentlyContinue).LastWriteTimeUtc
            Update-LoopSessionEvidence $sessionDllTime
            $seenTravels = 0
            $adopted = if ($sessionDllTime) { $sessionDllTime.ToString('o') } else { 'missing' }
            Write-LoopEvent "bot match session launched (adopting build $adopted)"
        }
    }
    if (Test-MaxIterationsReached) { break }
    Start-Sleep -Seconds $PollSeconds
}
