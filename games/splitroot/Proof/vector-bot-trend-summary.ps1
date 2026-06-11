param(
    [string]$ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..\..')).Path,
    [string]$HistoryPath = '',
    [string]$OutputPath = '',
    [int]$RecentRows = 20,
    [string]$Revision = ''
)

$ErrorActionPreference = 'Stop'

if (-not $HistoryPath) {
    $HistoryPath = Join-Path $ProjectRoot 'Saved\Proof\vector-bot-replay-diagnosis-history.jsonl'
}
if (-not $OutputPath) {
    $OutputPath = Join-Path $ProjectRoot 'Saved\Proof\last-vector-bot-trend-summary.json'
}

function Get-Prop([object]$Object, [string]$Name, [object]$DefaultValue = $null) {
    if (-not $Object) { return $DefaultValue }
    $property = $Object.PSObject.Properties[$Name]
    if (-not $property) { return $DefaultValue }
    if ($null -eq $property.Value) { return $DefaultValue }
    return $property.Value
}

function Add-Count([hashtable]$Table, [string]$Key, [int]$Amount = 1) {
    if ([string]::IsNullOrWhiteSpace($Key)) { $Key = 'unknown' }
    if (-not $Table.ContainsKey($Key)) { $Table[$Key] = 0 }
    $Table[$Key] += $Amount
}

function Add-TopRows([hashtable]$WeightedTable, [hashtable]$SeenTable, [object[]]$Rows) {
    $seenThisWindow = @{}
    foreach ($row in @($Rows)) {
        $key = [string](Get-Prop $row 'Key' '')
        if ([string]::IsNullOrWhiteSpace($key)) { continue }
        $count = [int](Get-Prop $row 'Count' 0)
        Add-Count $WeightedTable $key $count
        if (-not $seenThisWindow.ContainsKey($key)) {
            $seenThisWindow[$key] = $true
            Add-Count $SeenTable $key 1
        }
    }
}

function Get-TopWeightedRows([hashtable]$WeightedTable, [hashtable]$SeenTable, [int]$Limit = 10) {
    $items = foreach ($key in $WeightedTable.Keys) {
        [pscustomobject]@{
            Key = $key
            WeightedCount = [int]$WeightedTable[$key]
            WindowsSeen = if ($SeenTable.ContainsKey($key)) { [int]$SeenTable[$key] } else { 0 }
        }
    }
    @($items | Sort-Object -Property @{ Expression = 'WindowsSeen'; Descending = $true }, @{ Expression = 'WeightedCount'; Descending = $true }, @{ Expression = 'Key'; Ascending = $true } | Select-Object -First $Limit)
}

$rowsAll = @()
$readErrors = @()
if (Test-Path -LiteralPath $HistoryPath -PathType Leaf) {
    $lineNumber = 0
    foreach ($line in Get-Content -LiteralPath $HistoryPath) {
        $lineNumber++
        if ([string]::IsNullOrWhiteSpace($line)) { continue }
        try {
            $rowsAll += ($line | ConvertFrom-Json)
        } catch {
            $readErrors += [pscustomobject]@{
                Line = $lineNumber
                Error = $_.Exception.Message
            }
        }
    }
}

$filteredRows = @($rowsAll)
if (-not [string]::IsNullOrWhiteSpace($Revision)) {
    $filteredRows = @($filteredRows | Where-Object { [string](Get-Prop $_ 'BotStrategyTuningRevision' '') -eq $Revision })
}
if ($RecentRows -gt 0 -and $filteredRows.Count -gt $RecentRows) {
    $filteredRows = @($filteredRows | Select-Object -Last $RecentRows)
}

$revisionStats = @{}
$zoneWeighted = @{}
$zoneSeen = @{}
$roleWeighted = @{}
$roleSeen = @{}
$unstickSelectionWeighted = @{}
$unstickSelectionSeen = @{}
$phaseTargetLaneWeighted = @{}
$phaseTargetLaneSeen = @{}
$objectiveLaneWeighted = @{}
$objectiveLaneSeen = @{}
$firingPositionLaneWeighted = @{}
$firingPositionLaneSeen = @{}
$objectiveLaneShiftWeighted = @{}
$objectiveLaneShiftSeen = @{}

foreach ($row in $filteredRows) {
    $rowRevision = [string](Get-Prop $row 'BotStrategyTuningRevision' 'unknown')
    if (-not $revisionStats.ContainsKey($rowRevision)) {
        $revisionStats[$rowRevision] = [ordered]@{
            Revision = $rowRevision
            Windows = 0
            AdoptedWindows = 0
            ReadyWindows = 0
            ObserveMoreWindows = 0
            MatchEndCount = 0
            WeaponFiredCount = 0
            TowerFiredCount = 0
            BotStuckRecoveryCount = 0
            MaxObservedStuckAttempt = 0
            BotUnstickQueryCount = 0
            BotUnstickQueryClearCount = 0
            BotUnstickQueryFallbackCount = 0
            BotFiringPositionCount = 0
            BotRouteWaypointAbandonedCount = 0
            BotObjectiveLaneShiftCount = 0
            ObjectiveStuckContextLaneZeroCount = 0
            ObjectiveStuckContextShiftedCount = 0
            ObjectiveStuckContextAtMaxLaneShiftCount = 0
            LatestSnapshotUtc = $null
            LatestLoadedRevision = $null
        }
    }

    $stat = $revisionStats[$rowRevision]
    $stat.Windows++
    if ((Get-Prop $row 'BotStrategyTuningRevisionAdopted' $false) -eq $true) { $stat.AdoptedWindows++ }
    $gate = Get-Prop (Get-Prop $row 'VectorTuningGate' $null) 'Status' ''
    if ($gate -eq 'ready_for_tuning_review') { $stat.ReadyWindows++ }
    if ($gate -eq 'observe_more') { $stat.ObserveMoreWindows++ }
    $stat.MatchEndCount += [int](Get-Prop $row 'MatchEndCount' 0)
    $stat.WeaponFiredCount += [int](Get-Prop $row 'WeaponFiredCount' 0)
    $stat.TowerFiredCount += [int](Get-Prop $row 'TowerFiredCount' 0)
    $stat.BotStuckRecoveryCount += [int](Get-Prop $row 'BotStuckRecoveryCount' 0)
    $stat.MaxObservedStuckAttempt = [math]::Max([int]$stat.MaxObservedStuckAttempt, [int](Get-Prop $row 'MaxObservedStuckAttempt' 0))
    $stat.BotUnstickQueryCount += [int](Get-Prop $row 'BotUnstickQueryCount' 0)
    $stat.BotUnstickQueryClearCount += [int](Get-Prop $row 'BotUnstickQueryClearCount' 0)
    $stat.BotUnstickQueryFallbackCount += [int](Get-Prop $row 'BotUnstickQueryFallbackCount' 0)
    $stat.BotFiringPositionCount += [int](Get-Prop $row 'BotFiringPositionCount' 0)
    $stat.BotRouteWaypointAbandonedCount += [int](Get-Prop $row 'BotRouteWaypointAbandonedCount' 0)
    $stat.BotObjectiveLaneShiftCount += [int](Get-Prop $row 'BotObjectiveLaneShiftCount' 0)
    $stat.ObjectiveStuckContextLaneZeroCount += [int](Get-Prop $row 'ObjectiveStuckContextLaneZeroCount' 0)
    $stat.ObjectiveStuckContextShiftedCount += [int](Get-Prop $row 'ObjectiveStuckContextShiftedCount' 0)
    $stat.ObjectiveStuckContextAtMaxLaneShiftCount += [int](Get-Prop $row 'ObjectiveStuckContextAtMaxLaneShiftCount' 0)
    $stat.LatestSnapshotUtc = Get-Prop $row 'SnapshotUtc' $stat.LatestSnapshotUtc
    $stat.LatestLoadedRevision = Get-Prop $row 'LatestBotStrategyTuningLoadedRevision' $stat.LatestLoadedRevision

    Add-TopRows $zoneWeighted $zoneSeen @(Get-Prop $row 'TopStuckZones' @())
    Add-TopRows $roleWeighted $roleSeen @(Get-Prop $row 'TopStuckByRole' @())
    Add-TopRows $unstickSelectionWeighted $unstickSelectionSeen @(Get-Prop $row 'TopUnstickQueryBySelection' @())
    Add-TopRows $phaseTargetLaneWeighted $phaseTargetLaneSeen @(Get-Prop $row 'TopStuckRecoveryContextByPhaseTargetLaneShift' @())
    Add-TopRows $objectiveLaneWeighted $objectiveLaneSeen @(Get-Prop $row 'TopObjectiveStuckContextByLaneShift' @())
    Add-TopRows $firingPositionLaneWeighted $firingPositionLaneSeen @(Get-Prop $row 'TopBotFiringPositionByTargetLaneShift' @())
    Add-TopRows $objectiveLaneShiftWeighted $objectiveLaneShiftSeen @(Get-Prop $row 'TopObjectiveLaneShiftByTargetLane' @())
}

$revisionSummary = foreach ($stat in $revisionStats.Values) {
    $clearRate = if ([int]$stat.BotUnstickQueryCount -gt 0) {
        [math]::Round([int]$stat.BotUnstickQueryClearCount / [double][int]$stat.BotUnstickQueryCount, 3)
    } else {
        $null
    }
    [pscustomobject]@{
        Revision = $stat.Revision
        Windows = $stat.Windows
        AdoptedWindows = $stat.AdoptedWindows
        ReadyWindows = $stat.ReadyWindows
        ObserveMoreWindows = $stat.ObserveMoreWindows
        MatchEndCount = $stat.MatchEndCount
        WeaponFiredCount = $stat.WeaponFiredCount
        TowerFiredCount = $stat.TowerFiredCount
        BotStuckRecoveryCount = $stat.BotStuckRecoveryCount
        MaxObservedStuckAttempt = $stat.MaxObservedStuckAttempt
        BotUnstickQueryCount = $stat.BotUnstickQueryCount
        BotUnstickQueryClearCount = $stat.BotUnstickQueryClearCount
        BotUnstickQueryFallbackCount = $stat.BotUnstickQueryFallbackCount
        BotUnstickQueryClearRate = $clearRate
        BotFiringPositionCount = $stat.BotFiringPositionCount
        BotRouteWaypointAbandonedCount = $stat.BotRouteWaypointAbandonedCount
        BotObjectiveLaneShiftCount = $stat.BotObjectiveLaneShiftCount
        ObjectiveStuckContextLaneZeroCount = $stat.ObjectiveStuckContextLaneZeroCount
        ObjectiveStuckContextShiftedCount = $stat.ObjectiveStuckContextShiftedCount
        ObjectiveStuckContextAtMaxLaneShiftCount = $stat.ObjectiveStuckContextAtMaxLaneShiftCount
        LatestSnapshotUtc = $stat.LatestSnapshotUtc
        LatestLoadedRevision = $stat.LatestLoadedRevision
    }
}

$latestRow = if ($rowsAll.Count -gt 0) { $rowsAll[-1] } else { $null }
$topZones = Get-TopWeightedRows $zoneWeighted $zoneSeen 12
$latestRevision = [string](Get-Prop $latestRow 'BotStrategyTuningRevision' '')
$latestAdopted = Get-Prop $latestRow 'BotStrategyTuningRevisionAdopted' $null
$latestGate = Get-Prop (Get-Prop $latestRow 'VectorTuningGate' $null) 'Status' $null
$latestRevisionWindows = @($filteredRows | Where-Object { [string](Get-Prop $_ 'BotStrategyTuningRevision' '') -eq $latestRevision })

$recommendedLens = 'continue_sampling'
if ($false -eq $latestAdopted) {
    $recommendedLens = 'wait_for_revision_adoption'
} elseif ($latestRevisionWindows.Count -lt 2) {
    $recommendedLens = 'collect_second_window_for_latest_revision'
} elseif ($topZones.Count -gt 0 -and $topZones[0].WindowsSeen -ge 2) {
    $recommendedLens = 'track_or_route_stable_hotspot'
} elseif ($latestGate -eq 'observe_more') {
    $recommendedLens = 'observe_more_current_match'
}

$result = [pscustomobject]@{
    ProjectRoot = $ProjectRoot
    HistoryPath = $HistoryPath
    OutputPath = $OutputPath
    SnapshotUtc = (Get-Date).ToUniversalTime().ToString('o')
    HistoryExists = Test-Path -LiteralPath $HistoryPath -PathType Leaf
    ReadErrorCount = $readErrors.Count
    ReadErrors = @($readErrors | Select-Object -First 5)
    TotalHistoryRows = $rowsAll.Count
    RecentRowsRequested = $RecentRows
    RevisionFilter = $Revision
    RowsAnalyzed = $filteredRows.Count
    LatestRevision = $latestRevision
    LatestLoadedRevision = Get-Prop $latestRow 'LatestBotStrategyTuningLoadedRevision' $null
    LatestRevisionAdopted = $latestAdopted
    LatestGateStatus = $latestGate
    RecommendedLens = $recommendedLens
    RevisionSummary = @($revisionSummary | Sort-Object -Property @{ Expression = 'LatestSnapshotUtc'; Descending = $true })
    TopStableStuckZones = @($topZones)
    TopStuckByRoleAggregate = @(Get-TopWeightedRows $roleWeighted $roleSeen 8)
    TopUnstickSelectionAggregate = @(Get-TopWeightedRows $unstickSelectionWeighted $unstickSelectionSeen 5)
    TopStuckRecoveryContextByPhaseTargetLaneShiftAggregate = @(Get-TopWeightedRows $phaseTargetLaneWeighted $phaseTargetLaneSeen 12)
    TopObjectiveStuckContextByLaneShiftAggregate = @(Get-TopWeightedRows $objectiveLaneWeighted $objectiveLaneSeen 8)
    TopBotFiringPositionByTargetLaneShiftAggregate = @(Get-TopWeightedRows $firingPositionLaneWeighted $firingPositionLaneSeen 12)
    TopObjectiveLaneShiftByTargetLaneAggregate = @(Get-TopWeightedRows $objectiveLaneShiftWeighted $objectiveLaneShiftSeen 8)
}

$outputDir = Split-Path -Parent $OutputPath
if ($outputDir) {
    New-Item -ItemType Directory -Force -Path $outputDir | Out-Null
}
$result | ConvertTo-Json -Depth 10 | Set-Content -LiteralPath $OutputPath -Encoding UTF8
$result | ConvertTo-Json -Depth 10
