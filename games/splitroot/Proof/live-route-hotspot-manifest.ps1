param(
    [string]$ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..\..')).Path,
    [string]$TrendPath = '',
    [string]$VectorTrendPath = '',
    [string]$WatchPath = '',
    [string]$VectorDiagnosisPath = '',
    [string]$HostRefreshPath = '',
    [string]$BotStrategyTuningPath = '',
    [string]$OutputPath = ''
)

$ErrorActionPreference = 'Stop'

if (-not $TrendPath) {
    $TrendPath = Join-Path $ProjectRoot 'Saved\Proof\hex-live-canary-watch-trend-latest.json'
}
if (-not $WatchPath) {
    $WatchPath = Join-Path $ProjectRoot 'Saved\Proof\hex-live-canary-watch-latest.json'
}
if (-not $VectorDiagnosisPath) {
    $VectorDiagnosisPath = Join-Path $ProjectRoot 'Saved\Proof\last-vector-bot-replay-diagnosis.json'
}
if (-not $HostRefreshPath) {
    $HostRefreshPath = Join-Path $ProjectRoot 'Saved\Proof\host-refresh-pending.json'
}
if (-not $BotStrategyTuningPath) {
    $BotStrategyTuningPath = Join-Path $ProjectRoot 'games\splitroot\FactoryContracts\bot_strategy_tuning.json'
}
if (-not $OutputPath) {
    $OutputPath = Join-Path $ProjectRoot 'Saved\Proof\live-route-hotspot-manifest.json'
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

function Convert-ZoneKey([string]$Key) {
    if ($Key -match '^x=(-?\d+);y=(-?\d+)$') {
        return [pscustomobject]@{
            Key = $Key
            X = [int]$Matches[1]
            Y = [int]$Matches[2]
        }
    }
    [pscustomobject]@{
        Key = $Key
        X = $null
        Y = $null
    }
}

function Get-Prop($Object, [string]$Name, $DefaultValue = $null) {
    if (-not $Object) { return $DefaultValue }
    $property = $Object.PSObject.Properties[$Name]
    if (-not $property) { return $DefaultValue }
    if ($null -eq $property.Value) { return $DefaultValue }
    return $property.Value
}

function Get-ZoneSampleCount($SampleCounts, [string]$Key) {
    if (-not $SampleCounts -or [string]::IsNullOrWhiteSpace($Key)) {
        return 0
    }
    $match = @($SampleCounts | Where-Object { $_.Key -eq $Key } | Select-Object -First 1)
    if ($match.Count -eq 0) {
        return 0
    }
    return [int]$match[0].Count
}

$trend = Read-JsonOrNull $TrendPath
$watch = Read-JsonOrNull $WatchPath
$vector = Read-JsonOrNull $VectorDiagnosisPath
$hostRefresh = Read-JsonOrNull $HostRefreshPath
$botStrategyTuning = Read-JsonOrNull $BotStrategyTuningPath

if (-not $VectorTrendPath) {
    $revision = if ($botStrategyTuning -and $botStrategyTuning.PSObject.Properties['revision']) {
        [string]$botStrategyTuning.revision
    } else {
        ''
    }
    if ($revision -match '-v(\d+)$') {
        $candidatePath = Join-Path $ProjectRoot ("Saved\Proof\last-vector-bot-trend-summary-v{0}.json" -f $Matches[1])
        if (Test-Path -LiteralPath $candidatePath -PathType Leaf) {
            $VectorTrendPath = $candidatePath
        }
    }
    if (-not $VectorTrendPath) {
        $VectorTrendPath = Join-Path $ProjectRoot 'Saved\Proof\last-vector-bot-trend-summary.json'
    }
}

$vectorTrend = Read-JsonOrNull $VectorTrendPath

$latestPidSet = if ($trend -and $trend.LatestProcessIds) { @($trend.LatestProcessIds) } else { @() }
$currentRunSampleCount = if ($trend) { [int]$trend.CurrentRunSampleCount } else { 0 }
$topVectorZones = if ($vectorTrend -and $vectorTrend.TopStableStuckZones) {
    @($vectorTrend.TopStableStuckZones | Select-Object -First 8)
} elseif ($trend -and $trend.AggregateVectorTopStuckZones) {
    @($trend.AggregateVectorTopStuckZones | Select-Object -First 8)
} elseif ($vector -and $vector.TopStuckZones) {
    @($vector.TopStuckZones | Select-Object -First 8)
} else {
    @()
}
$trendAggregateVectorTopZone = if ($vectorTrend -and $vectorTrend.TopStableStuckZones -and $vectorTrend.TopStableStuckZones.Count -gt 0) {
    $vectorTrend.TopStableStuckZones[0].Key
} elseif ($trend -and $trend.LatestVectorTopStuckZone) {
    $trend.LatestVectorTopStuckZone
} elseif ($topVectorZones.Count -gt 0) {
    $topVectorZones[0].Key
} else {
    $null
}
$latestVectorDiagnosisTopZone = if ($vector -and $vector.TopStuckZones -and $vector.TopStuckZones.Count -gt 0) {
    $vector.TopStuckZones[0].Key
} elseif ($watch -and $watch.VectorDiagnosis -and $watch.VectorDiagnosis.TopStuckZones -and $watch.VectorDiagnosis.TopStuckZones.Count -gt 0) {
    $watch.VectorDiagnosis.TopStuckZones[0].Key
} else {
    $null
}

$missingUnstickQuery = $false
$missingFiringPosition = $false
$vectorStuckCount = 0
if ($vector) {
    $vectorStuckCount = [int]$vector.BotStuckRecoveryCount
    $missingUnstickQuery = $vectorStuckCount -gt 0 -and [int]$vector.BotUnstickQueryCount -eq 0
    $missingFiringPosition = $vectorStuckCount -gt 0 -and [int]$vector.BotFiringPositionCount -eq 0
} elseif ($watch -and $watch.VectorDiagnosis) {
    $vectorStuckCount = [int]$watch.VectorDiagnosis.BotStuckRecoveryCount
    $missingUnstickQuery = $vectorStuckCount -gt 0 -and [int]$watch.VectorDiagnosis.BotUnstickQueryCount -eq 0
    $missingFiringPosition = $vectorStuckCount -gt 0 -and [int]$watch.VectorDiagnosis.BotFiringPositionCount -eq 0
}

$vectorGate = if ($vector) {
    Get-Prop $vector 'VectorTuningGate' $null
} elseif ($watch -and $watch.VectorDiagnosis) {
    Get-Prop $watch.VectorDiagnosis 'VectorTuningGate' $null
} else {
    $null
}
$vectorGateStatus = [string](Get-Prop $vectorGate 'Status' '')
$vectorGateReasons = @()
if ($vectorGate -and $vectorGate.PSObject.Properties['Reasons']) {
    $vectorGateReasons = @($vectorGate.Reasons)
}
$isEarlyWindow = $vectorGateStatus -eq 'observe_more' -and ($vectorGateReasons -contains 'early_match_window')
$needsVectorCoverage = $missingUnstickQuery -or $missingFiringPosition

$handoffZones = @()
foreach ($zone in $topVectorZones) {
    $parsed = Convert-ZoneKey ([string]$zone.Key)
    $sampleCount = if ($zone.PSObject.Properties['WindowsSeen']) {
        [int]$zone.WindowsSeen
    } elseif ($trend) {
        Get-ZoneSampleCount $trend.VectorStuckZoneSampleCounts $zone.Key
    } else {
        0
    }
    $currentRunTopCount = if ($trend -and $trend.LatestVectorTopStuckZone -eq $zone.Key) {
        [int]$trend.CurrentRunLatestVectorTopZoneSampleCount
    } else {
        0
    }
    $aggregateCount = if ($zone.PSObject.Properties['WeightedCount']) {
        [int]$zone.WeightedCount
    } else {
        [int](Get-Prop $zone 'Count' 0)
    }

    $handoffZones += [pscustomobject]@{
        Key = $parsed.Key
        X = $parsed.X
        Y = $parsed.Y
        AggregateCount = $aggregateCount
        SampleCount = $sampleCount
        CurrentRunTopSampleCount = $currentRunTopCount
        PersistentInCurrentRun = $currentRunTopCount -ge 3
        PrimaryLane = if ($needsVectorCoverage) { 'Vector' } else { 'Quarry' }
        SecondaryLane = if ($needsVectorCoverage) { 'Quarry' } else { 'Vector' }
        BoundedReason = if ($isEarlyWindow -and $needsVectorCoverage) {
            'Current revision is adopted, but this is an early match window; keep sampling before route/terrain interpretation.'
        } elseif ($needsVectorCoverage) {
            'Bot feature adoption is missing, so route interpretation should wait for Vector C++ adoption proof.'
        } else {
            'Bot feature coverage is present; persistent spatial clustering is ready for route/terrain audit.'
        }
    }
}

$nextAction = if ($missingUnstickQuery -or $missingFiringPosition) {
    'Vector should adopt/prove BotUnstickQuery and BotFiringPosition before Quarry treats these as terrain-only blockers.'
} elseif ($handoffZones.Count -gt 0) {
    'Quarry should inspect persistent route geometry around the top zones; Vector should keep sampling after any terrain fix.'
} else {
    'Keep sampling; no hotspot zones were present in the source artifacts.'
}
$pendingNativeRefresh = $hostRefresh -and $hostRefresh.Pending
$supervisorOwnsHost = $hostRefresh -and $hostRefresh.SupervisorOwnsHost

$result = [pscustomobject]@{
    Agent = 'Hex'
    ObservedUtc = (Get-Date).ToUniversalTime().ToString('o')
    ProjectRoot = $ProjectRoot
    NonDisruptiveOnly = $true
    Purpose = 'Condense live canary stuck-zone evidence into a small Vector/Quarry handoff manifest.'
    SourceArtifacts = @(
        Get-SourceState $TrendPath
        Get-SourceState $VectorTrendPath
        Get-SourceState $WatchPath
        Get-SourceState $VectorDiagnosisPath
        Get-SourceState $HostRefreshPath
        Get-SourceState $BotStrategyTuningPath
    )
    LatestProcessIds = $latestPidSet
    CurrentRunSampleCount = $currentRunSampleCount
    HostRefresh = if ($hostRefresh) {
        [pscustomobject]@{
            Pending = [bool]$hostRefresh.Pending
            Reason = $hostRefresh.Reason
            BoundaryPolicy = $hostRefresh.BoundaryPolicy
            SupervisorOwnsHost = [bool]$hostRefresh.SupervisorOwnsHost
            CurrentProcessId = $hostRefresh.CurrentProcessId
            PendingModuleDllUtc = $hostRefresh.PendingModuleDllUtc
            NextMap = $hostRefresh.NextMap
        }
    } else { $null }
    PendingNativeRefresh = [bool]$pendingNativeRefresh
    SupervisorOwnsHost = [bool]$supervisorOwnsHost
    AdoptionState = if ($pendingNativeRefresh -and -not $supervisorOwnsHost) {
        'blocked_requires_handoff'
    } elseif ($pendingNativeRefresh) {
        'pending_supervisor_refresh'
    } else {
        'no_pending_refresh'
    }
    StagedBotStrategyRevision = if ($botStrategyTuning) { $botStrategyTuning.revision } else { $null }
    StagedBreacherPursuitMemoryWindowSeconds = if ($botStrategyTuning -and $botStrategyTuning.roles) {
        $botStrategyTuning.roles.breacher_pursuit_memory_window_seconds
    } else { $null }
    StagedBreacherThreatWindowSeconds = if ($botStrategyTuning -and $botStrategyTuning.roles) {
        $botStrategyTuning.roles.breacher_threat_window_seconds
    } else { $null }
    LiveBotStrategyTuningLoadedCount = if ($vector -and $vector.PSObject.Properties['BotStrategyTuningLoadedCount']) {
        $vector.BotStrategyTuningLoadedCount
    } else { $null }
    LiveBotStrategyTuningRevision = if ($vector -and $vector.PSObject.Properties['BotStrategyTuningRevision']) {
        $vector.BotStrategyTuningRevision
    } else { $null }
    VectorTuningGate = if ($vectorGate) {
        [pscustomobject]@{
            Status = $vectorGateStatus
            Reasons = $vectorGateReasons
        }
    } else { $null }
    VectorStuckRecoveryCount = $vectorStuckCount
    MissingBotUnstickQueryCoverage = $missingUnstickQuery
    MissingBotFiringPositionCoverage = $missingFiringPosition
    TrendAggregateVectorTopStuckZone = $trendAggregateVectorTopZone
    VectorTrend = if ($vectorTrend) {
        [pscustomobject]@{
            Path = $VectorTrendPath
            SnapshotUtc = $vectorTrend.SnapshotUtc
            RowsAnalyzed = $vectorTrend.RowsAnalyzed
            LatestRevision = $vectorTrend.LatestRevision
            LatestLoadedRevision = $vectorTrend.LatestLoadedRevision
            LatestRevisionAdopted = $vectorTrend.LatestRevisionAdopted
            LatestGateStatus = $vectorTrend.LatestGateStatus
            RecommendedLens = $vectorTrend.RecommendedLens
        }
    } else { $null }
    LatestVectorDiagnosisTopStuckZone = $latestVectorDiagnosisTopZone
    LatestVectorDiagnosisTopStuckZones = if ($vector -and $vector.TopStuckZones) { @($vector.TopStuckZones | Select-Object -First 5) } else { @() }
    LatestVectorTopStuckZone = $trendAggregateVectorTopZone
    CurrentRunLatestVectorTopZoneSampleCount = if ($trend) { $trend.CurrentRunLatestVectorTopZoneSampleCount } else { 0 }
    HandoffZones = $handoffZones
    NextAction = if ($pendingNativeRefresh -and -not $supervisorOwnsHost) {
        'Wait for explicit live-session handoff or supervisor-owned match-boundary refresh before interpreting staged native bot-strategy behavior.'
    } elseif ($isEarlyWindow -and $needsVectorCoverage) {
        'Keep sampling this adopted revision until feature coverage is ready before sending terrain causality to Quarry.'
    } else {
        $nextAction
    }
    ClaimBoundary = 'Proof-only artifact from existing log summaries; it does not prove terrain causality or live adoption of staged C++ slices.'
}

$result | ConvertTo-Json -Depth 10 | Set-Content -LiteralPath $OutputPath -Encoding UTF8
$result | ConvertTo-Json -Depth 10
