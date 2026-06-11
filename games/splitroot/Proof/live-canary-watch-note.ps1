param(
    [string]$ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..\..')).Path,
    [string]$SummaryPath = '',
    [string]$SnapshotPath = '',
    [string]$PendingPath = '',
    [string]$CoordinationStatusPath = '',
    [string]$VectorDiagnosisPath = '',
    [string]$HotspotManifestPath = '',
    [string]$SupervisorPidPath = '',
    [string]$SupervisorLogPath = '',
    [string]$OutputPath = '',
    [switch]$Archive
)

$ErrorActionPreference = 'Stop'

if (-not $SummaryPath) {
    $SummaryPath = Join-Path $ProjectRoot 'Saved\Proof\last-live-log-window-summary.json'
}
if (-not $SnapshotPath) {
    $SnapshotPath = Join-Path $ProjectRoot 'Saved\Proof\last-live-bot-match-log-snapshot.json'
}
if (-not $PendingPath) {
    $PendingPath = Join-Path $ProjectRoot 'Saved\Proof\host-refresh-pending.json'
}
if (-not $CoordinationStatusPath) {
    $CoordinationStatusPath = Join-Path $ProjectRoot 'Saved\Proof\coordination-status-summary.json'
}
if (-not $VectorDiagnosisPath) {
    $VectorDiagnosisPath = Join-Path $ProjectRoot 'Saved\Proof\last-vector-bot-replay-diagnosis.json'
}
if (-not $HotspotManifestPath) {
    $HotspotManifestPath = Join-Path $ProjectRoot 'Saved\Proof\live-route-hotspot-manifest.json'
}
if (-not $SupervisorPidPath) {
    $SupervisorPidPath = Join-Path $ProjectRoot 'Saved\Proof\host-supervisor-loop.pid.json'
}
if (-not $SupervisorLogPath) {
    $SupervisorLogPath = Join-Path $ProjectRoot 'Saved\Proof\host-supervisor-loop.log'
}
if (-not $OutputPath) {
    $OutputPath = Join-Path $ProjectRoot 'Saved\Proof\hex-live-canary-watch-latest.json'
}

$proofDir = Split-Path -Parent $OutputPath
New-Item -ItemType Directory -Force -Path $proofDir | Out-Null

function Read-JsonOrNull([string]$Path) {
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        return $null
    }
    return Get-Content -Raw -LiteralPath $Path | ConvertFrom-Json
}

function Get-SourceState([string]$Path) {
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        return [pscustomobject]@{
            Path = $Path
            Exists = $false
            LastWriteTimeUtc = $null
        }
    }

    $item = Get-Item -LiteralPath $Path
    [pscustomobject]@{
        Path = $Path
        Exists = $true
        LastWriteTimeUtc = $item.LastWriteTimeUtc.ToString('o')
    }
}

function Get-TopZoneStrings($Zones, [int]$Limit = 3) {
    @($Zones | Select-Object -First $Limit | ForEach-Object {
        '{0} ({1})' -f $_.Key, $_.Count
    })
}

function Get-TextTail([string]$Path, [int]$Count = 8) {
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        return @()
    }
    @(Get-Content -LiteralPath $Path -Tail $Count | ForEach-Object { [string]$_ })
}

$summary = Read-JsonOrNull $SummaryPath
$snapshot = Read-JsonOrNull $SnapshotPath
$pending = Read-JsonOrNull $PendingPath
$coordinationStatus = Read-JsonOrNull $CoordinationStatusPath
$vectorDiagnosis = Read-JsonOrNull $VectorDiagnosisPath
$hotspotManifest = Read-JsonOrNull $HotspotManifestPath
$supervisorState = Read-JsonOrNull $SupervisorPidPath
$supervisorLogTail = Get-TextTail $SupervisorLogPath 8
$supervisorProcess = $null
if ($supervisorState -and $supervisorState.Pid) {
    $supervisorPid = [int]$supervisorState.Pid
    $supervisorProcess = Get-Process -Id $supervisorPid -ErrorAction SilentlyContinue
}
$inboxPath = Join-Path $ProjectRoot 'Coordination\inbox-hex'
$inboxFiles = if (Test-Path -LiteralPath $inboxPath -PathType Container) {
    @(Get-ChildItem -LiteralPath $inboxPath -File)
} else {
    @()
}

$runningUnrealProcesses = @(Get-Process UnrealEditor -ErrorAction SilentlyContinue |
    Select-Object Id, ProcessName, MainWindowTitle, Path, StartTime)

$boundedRead = @()
if ($runningUnrealProcesses.Count -gt 0) {
    $boundedRead += 'Visible current-build UnrealEditor.exe process is still running.'
} else {
    $boundedRead += 'No UnrealEditor.exe process was visible to this read-only proof.'
}
if ($snapshot -and $snapshot.MatchLoopActive) {
    $boundedRead += 'Live bot match loop is active in the latest snapshot.'
}
if ($snapshot -and -not $snapshot.TakeFiringPositionCovered) {
    $boundedRead += 'Latest snapshot still has no take_firing_position coverage.'
}
if ($summary -and $summary.BotStuckRecoveryCount -gt 0) {
    $zones = Get-TopZoneStrings $summary.TopBotStuckZones 3
    if ($zones.Count -gt 0) {
        $boundedRead += ('Current tail stuck hotspots: ' + ($zones -join '; ') + '.')
    } else {
        $boundedRead += 'Current tail has bot stuck recovery events.'
    }
}
if ($vectorDiagnosis) {
    if ($vectorDiagnosis.BotUnstickQueryCount -eq 0 -and $vectorDiagnosis.BotStuckRecoveryCount -gt 0) {
        $boundedRead += 'Vector diagnosis has stuck recovery but no BotUnstickQuery coverage.'
    }
    if ($vectorDiagnosis.BotFiringPositionCount -eq 0 -and $vectorDiagnosis.BotStuckRecoveryCount -gt 0) {
        $boundedRead += 'Vector diagnosis has stuck recovery but no BotFiringPosition coverage.'
    }
    if ($vectorDiagnosis.Recommendations) {
        $firstRecommendation = @($vectorDiagnosis.Recommendations | Sort-Object Priority | Select-Object -First 1)
        if ($firstRecommendation.Count -gt 0) {
            $boundedRead += ('Top Vector recommendation: ' + $firstRecommendation[0].Code + '.')
        }
    }
}
if ($hotspotManifest) {
    if ($hotspotManifest.AdoptionState) {
        $boundedRead += ('Route hotspot manifest adoption state: ' + $hotspotManifest.AdoptionState + '.')
    }
    if ($hotspotManifest.StagedBotStrategyRevision -and $null -ne $hotspotManifest.LiveBotStrategyTuningLoadedCount) {
        $boundedRead += (
            'Staged bot strategy ' + $hotspotManifest.StagedBotStrategyRevision +
            ' has live loaded count ' + $hotspotManifest.LiveBotStrategyTuningLoadedCount + '.')
    }
    if ($hotspotManifest.LatestVectorDiagnosisTopStuckZone) {
        $boundedRead += ('Latest Vector diagnosis top stuck zone: ' + $hotspotManifest.LatestVectorDiagnosisTopStuckZone + '.')
    }
}
if ($supervisorState) {
    if ($supervisorProcess) {
        $boundedRead += ('Host supervisor loop PID ' + $supervisorState.Pid + ' is running.')
    } else {
        $boundedRead += ('Host supervisor loop state names PID ' + $supervisorState.Pid + ', but that process was not visible.')
    }
    $observingLine = @($supervisorLogTail | Where-Object { $_ -match 'observing existing live current-build pid=' } | Select-Object -Last 1)
    if ($observingLine.Count -gt 0) {
        $boundedRead += 'Host supervisor latest observed-current-build log confirms it will not attach or stop the visible session.'
    }
}
if ($pending -and $pending.Pending) {
    $boundedRead += 'Host refresh pending state is true; supervisor-owned refresh policy applies.'
} else {
    $boundedRead += 'Host refresh pending state is not active.'
}
if ($inboxFiles.Count -eq 0) {
    $boundedRead += 'Coordination/inbox-hex is empty.'
} else {
    $boundedRead += ('Coordination/inbox-hex has ' + $inboxFiles.Count + ' task file(s).')
}
if ($coordinationStatus) {
    $boundedRead += (
        'Coordination status counts: blocked_requires_handoff=' +
        $coordinationStatus.BlockedRequiresHandoffCount +
        ', staged_not_live=' + $coordinationStatus.StagedNotLiveCount +
        ', proof_only=' + $coordinationStatus.ProofOnlyCount +
        ', completed_and_live=' + $coordinationStatus.CompletedAndLiveCount + '.')
    if ($coordinationStatus.PSObject.Properties['InferredBlockedRequiresHandoffCount']) {
        $boundedRead += (
            'Coordination inferred counts: blocked_requires_handoff=' +
            $coordinationStatus.InferredBlockedRequiresHandoffCount +
            ', staged_not_live=' + $coordinationStatus.InferredStagedNotLiveCount +
            ', proof_only=' + $coordinationStatus.InferredProofOnlyCount +
            ', legacy_unknown=' + $coordinationStatus.InferredLegacyUnknownCount + '.')
    }
}

$result = [pscustomobject]@{
    Agent = 'Hex'
    ObservedUtc = (Get-Date).ToUniversalTime().ToString('o')
    ProjectRoot = $ProjectRoot
    NonDisruptiveOnly = $true
    SourceArtifacts = @(
        Get-SourceState $SummaryPath
        Get-SourceState $SnapshotPath
        Get-SourceState $PendingPath
        Get-SourceState $CoordinationStatusPath
        Get-SourceState $VectorDiagnosisPath
        Get-SourceState $HotspotManifestPath
        Get-SourceState $SupervisorPidPath
        Get-SourceState $SupervisorLogPath
    )
    RunningUnrealProcesses = $runningUnrealProcesses
    InboxHexCount = $inboxFiles.Count
    InboxHexFiles = @($inboxFiles | Select-Object Name, Length, LastWriteTimeUtc)
    Summary = if ($summary) {
        [pscustomobject]@{
            SnapshotUtc = $summary.SnapshotUtc
            TailLinesRead = $summary.TailLinesRead
            MatchEndCount = $summary.MatchEndCount
            NextMatchPendingCount = $summary.NextMatchPendingCount
            WeaponFiredCount = $summary.WeaponFiredCount
            TowerFiredCount = $summary.TowerFiredCount
            BotRespawnedCount = $summary.BotRespawnedCount
            BotStuckRecoveryCount = $summary.BotStuckRecoveryCount
            TopBotStuckZones = @($summary.TopBotStuckZones | Select-Object -First 5)
            FeatureCoverageKeys = @($summary.FeatureCoverageKeys)
            Read = @($summary.Read)
        }
    } else { $null }
    Snapshot = if ($snapshot) {
        [pscustomobject]@{
            SnapshotUtc = $snapshot.SnapshotUtc
            EvidenceReady = $snapshot.EvidenceReady
            MatchLoopActive = $snapshot.MatchLoopActive
            BotMatchSpawned = $snapshot.BotMatchSpawned
            WeaponFiredCount = $snapshot.WeaponFiredCount
            TowerFiredCount = $snapshot.TowerFiredCount
            BotRespawnedCount = $snapshot.BotRespawnedCount
            BotStuckRecoveryCount = $snapshot.BotStuckRecoveryCount
            TakeFiringPositionCovered = $snapshot.TakeFiringPositionCovered
            FeatureCoverageKeys = @($snapshot.FeatureCoverageKeys)
            LatestBuildFingerprint = $snapshot.LatestBuildFingerprint
        }
    } else { $null }
    HostRefresh = if ($pending) {
        [pscustomobject]@{
            Pending = $pending.Pending
            SupervisorOwnsHost = $pending.SupervisorOwnsHost
            BoundaryPolicy = $pending.BoundaryPolicy
            CurrentProcessId = $pending.CurrentProcessId
        }
    } else { $null }
    VectorDiagnosis = if ($vectorDiagnosis) {
        [pscustomobject]@{
            SnapshotUtc = $vectorDiagnosis.SnapshotUtc
            TailLinesRead = $vectorDiagnosis.TailLinesRead
            HasCombatWindow = $vectorDiagnosis.HasCombatWindow
            BotStuckRecoveryCount = $vectorDiagnosis.BotStuckRecoveryCount
            MaxObservedStuckAttempt = $vectorDiagnosis.MaxObservedStuckAttempt
            TopStuckZones = @($vectorDiagnosis.TopStuckZones | Select-Object -First 5)
            BotUnstickQueryCount = $vectorDiagnosis.BotUnstickQueryCount
            BotFiringPositionCount = $vectorDiagnosis.BotFiringPositionCount
            WeaponFiredCount = $vectorDiagnosis.WeaponFiredCount
            NativePerceptionCount = $vectorDiagnosis.NativePerceptionCount
            FeatureCoverageKeys = @($vectorDiagnosis.FeatureCoverageKeys)
            Recommendations = @($vectorDiagnosis.Recommendations | Sort-Object Priority | Select-Object -First 5)
        }
    } else { $null }
    HotspotManifest = if ($hotspotManifest) {
        [pscustomobject]@{
            ObservedUtc = $hotspotManifest.ObservedUtc
            AdoptionState = $hotspotManifest.AdoptionState
            PendingNativeRefresh = $hotspotManifest.PendingNativeRefresh
            SupervisorOwnsHost = $hotspotManifest.SupervisorOwnsHost
            StagedBotStrategyRevision = $hotspotManifest.StagedBotStrategyRevision
            LiveBotStrategyTuningLoadedCount = $hotspotManifest.LiveBotStrategyTuningLoadedCount
            StagedBreacherPursuitMemoryWindowSeconds = $hotspotManifest.StagedBreacherPursuitMemoryWindowSeconds
            StagedBreacherThreatWindowSeconds = $hotspotManifest.StagedBreacherThreatWindowSeconds
            TrendAggregateVectorTopStuckZone = $hotspotManifest.TrendAggregateVectorTopStuckZone
            LatestVectorDiagnosisTopStuckZone = $hotspotManifest.LatestVectorDiagnosisTopStuckZone
            CurrentRunLatestVectorTopZoneSampleCount = $hotspotManifest.CurrentRunLatestVectorTopZoneSampleCount
            HandoffZones = @($hotspotManifest.HandoffZones | Select-Object -First 5)
            NextAction = $hotspotManifest.NextAction
        }
    } else { $null }
    SupervisorLoop = if ($supervisorState) {
        [pscustomobject]@{
            Pid = $supervisorState.Pid
            ProcessAlive = [bool]$supervisorProcess
            StartedUtc = $supervisorState.StartedUtc
            AllowBoundaryRefresh = $supervisorState.AllowBoundaryRefresh
            BuildAtBoundary = $supervisorState.BuildAtBoundary
            BuildScriptPath = $supervisorState.BuildScriptPath
            LatestLogTail = @($supervisorLogTail)
        }
    } else { $null }
    CoordinationStatus = if ($coordinationStatus) {
        [pscustomobject]@{
            ObservedUtc = $coordinationStatus.ObservedUtc
            ActiveInboxCount = $coordinationStatus.ActiveInboxCount
            CompletedCount = $coordinationStatus.CompletedCount
            CompletedAndLiveCount = $coordinationStatus.CompletedAndLiveCount
            BlockedRequiresHandoffCount = $coordinationStatus.BlockedRequiresHandoffCount
            StagedNotLiveCount = $coordinationStatus.StagedNotLiveCount
            ProofOnlyCount = $coordinationStatus.ProofOnlyCount
            SupersededCount = $coordinationStatus.SupersededCount
            LegacyUnknownCount = $coordinationStatus.LegacyUnknownCount
            UnrecognizedStatusCount = $coordinationStatus.UnrecognizedStatusCount
            InferredCompletedCount = $coordinationStatus.InferredCompletedCount
            InferredBlockedRequiresHandoffCount = $coordinationStatus.InferredBlockedRequiresHandoffCount
            InferredStagedNotLiveCount = $coordinationStatus.InferredStagedNotLiveCount
            InferredProofOnlyCount = $coordinationStatus.InferredProofOnlyCount
            InferredLegacyUnknownCount = $coordinationStatus.InferredLegacyUnknownCount
        }
    } else { $null }
    BoundedRead = $boundedRead
    NextNonDisruptiveMove = 'Continue read-only live telemetry analysis; defer build/import/restart work until an explicit live-session handoff.'
}

$json = $result | ConvertTo-Json -Depth 10
$json | Set-Content -LiteralPath $OutputPath -Encoding UTF8

if ($Archive) {
    $stamp = (Get-Date).ToUniversalTime().ToString('yyyyMMddTHHmmssZ')
    $archivePath = Join-Path $proofDir ("hex-live-canary-watch-note-$stamp.json")
    $json | Set-Content -LiteralPath $archivePath -Encoding UTF8
}

$json
