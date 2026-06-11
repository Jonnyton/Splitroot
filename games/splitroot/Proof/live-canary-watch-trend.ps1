param(
    [string]$ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..\..')).Path,
    [int]$MaxNotes = 12,
    [string]$OutputPath = ''
)

$ErrorActionPreference = 'Stop'

if (-not $OutputPath) {
    $OutputPath = Join-Path $ProjectRoot 'Saved\Proof\hex-live-canary-watch-trend-latest.json'
}

$proofDir = Split-Path -Parent $OutputPath
New-Item -ItemType Directory -Force -Path $proofDir | Out-Null

function Read-JsonOrNull([string]$Path) {
    try {
        return Get-Content -Raw -LiteralPath $Path | ConvertFrom-Json
    } catch {
        return $null
    }
}

function Get-ObservedUtc($Note) {
    if ($Note.ObservedUtc) { return [datetime]::Parse([string]$Note.ObservedUtc).ToUniversalTime() }
    if ($Note.observedUtc) { return [datetime]::Parse([string]$Note.observedUtc).ToUniversalTime() }
    return [datetime]::MinValue
}

function Add-Count([hashtable]$Table, [string]$Key, [int]$Count = 1) {
    if ([string]::IsNullOrWhiteSpace($Key)) { return }
    if (-not $Table.ContainsKey($Key)) { $Table[$Key] = 0 }
    $Table[$Key] += $Count
}

function Get-TopCounts([hashtable]$Table, [int]$Limit = 10) {
    $items = foreach ($key in $Table.Keys) {
        [pscustomobject]@{ Key = $key; Count = [int]$Table[$key] }
    }
    @($items | Sort-Object -Property @{ Expression = 'Count'; Descending = $true }, @{ Expression = 'Key'; Ascending = $true } | Select-Object -First $Limit)
}

function Get-NoteProcessIds($Note) {
    $ids = @()
    if ($Note.RunningUnrealProcesses) {
        $ids += @($Note.RunningUnrealProcesses | ForEach-Object { $_.Id })
    }
    if ($Note.liveUnrealProcessId) {
        $ids += $Note.liveUnrealProcessId
    }
    @($ids | Where-Object { $null -ne $_ } | ForEach-Object { [int]$_ } | Sort-Object -Unique)
}

function Get-NoteTopZones($Note) {
    if ($Note.Summary -and $Note.Summary.TopBotStuckZones) {
        return @($Note.Summary.TopBotStuckZones | ForEach-Object {
            [pscustomobject]@{ Key = [string]$_.Key; Count = [int]$_.Count }
        })
    }
    if ($Note.tailWindow -and $Note.tailWindow.topStuckZones) {
        return @($Note.tailWindow.topStuckZones | ForEach-Object {
            [pscustomobject]@{ Key = [string]$_.zone; Count = [int]$_.count }
        })
    }
    return @()
}

function Get-NoteVectorTopZones($Note) {
    if ($Note.VectorDiagnosis -and $Note.VectorDiagnosis.TopStuckZones) {
        return @($Note.VectorDiagnosis.TopStuckZones | ForEach-Object {
            [pscustomobject]@{ Key = [string]$_.Key; Count = [int]$_.Count }
        })
    }
    return @()
}

function Get-NoteVectorRecommendationCodes($Note) {
    if ($Note.VectorDiagnosis -and $Note.VectorDiagnosis.Recommendations) {
        return @($Note.VectorDiagnosis.Recommendations | Sort-Object Priority | ForEach-Object { [string]$_.Code } |
            Where-Object { -not [string]::IsNullOrWhiteSpace($_) })
    }
    return @()
}

function Get-NoteFeatureKeys($Note) {
    $keys = @()
    if ($Note.Summary -and $Note.Summary.FeatureCoverageKeys) {
        $keys += @($Note.Summary.FeatureCoverageKeys)
    }
    if ($Note.Snapshot -and $Note.Snapshot.FeatureCoverageKeys) {
        $keys += @($Note.Snapshot.FeatureCoverageKeys)
    }
    if ($Note.tailWindow -and $Note.tailWindow.featureCoverageKeys) {
        $keys += @($Note.tailWindow.featureCoverageKeys)
    }
    if ($Note.fullSnapshot -and $Note.fullSnapshot.featureCoverageKeys) {
        $keys += @($Note.fullSnapshot.featureCoverageKeys)
    }
    @($keys | Where-Object { -not [string]::IsNullOrWhiteSpace([string]$_) } | Sort-Object -Unique)
}

function Join-ProcessIds([object[]]$Ids) {
    @($Ids | Where-Object { $null -ne $_ } | ForEach-Object { [string]$_ } | Sort-Object -Unique) -join ','
}

$noteFiles = @(Get-ChildItem -LiteralPath $proofDir -File -Filter 'hex-live-canary-watch-note-*.json' |
    Sort-Object LastWriteTimeUtc -Descending |
    Select-Object -First $MaxNotes)

$notes = @()
foreach ($file in $noteFiles) {
    $note = Read-JsonOrNull $file.FullName
    if ($note) {
        $notes += [pscustomobject]@{
            File = $file.FullName
            Note = $note
            ObservedUtc = Get-ObservedUtc $note
        }
    }
}

$notes = @($notes | Sort-Object ObservedUtc)
$zoneTotals = @{}
$zoneSamples = @{}
$vectorZoneTotals = @{}
$vectorZoneSamples = @{}
$featureKeys = @{}
$processIds = @{}
$reads = @()
$sampleSummaries = @()

foreach ($entry in $notes) {
    $note = $entry.Note
    $ids = Get-NoteProcessIds $note
    foreach ($id in $ids) { Add-Count $processIds ([string]$id) 1 }

    $zones = Get-NoteTopZones $note
    foreach ($zone in $zones) {
        Add-Count $zoneTotals $zone.Key $zone.Count
        Add-Count $zoneSamples $zone.Key 1
    }

    $vectorZones = Get-NoteVectorTopZones $note
    foreach ($zone in $vectorZones) {
        Add-Count $vectorZoneTotals $zone.Key $zone.Count
        Add-Count $vectorZoneSamples $zone.Key 1
    }

    foreach ($key in (Get-NoteFeatureKeys $note)) {
        Add-Count $featureKeys ([string]$key) 1
    }

    if ($note.BoundedRead) { $reads += @($note.BoundedRead) }
    if ($note.boundedRead) { $reads += @($note.boundedRead) }

    $sampleSummaries += [pscustomobject]@{
        ObservedUtc = $entry.ObservedUtc.ToString('o')
        FileName = Split-Path -Leaf $entry.File
        ProcessIds = $ids
        TopStuckZones = $zones
        VectorTopStuckZones = $vectorZones
        VectorBotStuckRecoveryCount = if ($note.VectorDiagnosis) { $note.VectorDiagnosis.BotStuckRecoveryCount } else { $null }
        VectorBotUnstickQueryCount = if ($note.VectorDiagnosis) { $note.VectorDiagnosis.BotUnstickQueryCount } else { $null }
        VectorBotFiringPositionCount = if ($note.VectorDiagnosis) { $note.VectorDiagnosis.BotFiringPositionCount } else { $null }
        VectorRecommendationCodes = Get-NoteVectorRecommendationCodes $note
        FeatureCoverageKeys = Get-NoteFeatureKeys $note
        MatchLoopActive = if ($note.Snapshot) { $note.Snapshot.MatchLoopActive } elseif ($note.fullSnapshot) { $note.fullSnapshot.matchLoopActive } else { $null }
        EvidenceReady = if ($note.Snapshot) { $note.Snapshot.EvidenceReady } elseif ($note.fullSnapshot) { $note.fullSnapshot.evidenceReady } else { $null }
        TakeFiringPositionCovered = if ($note.Snapshot) { $note.Snapshot.TakeFiringPositionCovered } elseif ($note.fullSnapshot) { $note.fullSnapshot.takeFiringPositionCovered } else { $null }
    }
}

$latest = if ($sampleSummaries.Count -gt 0) { $sampleSummaries[-1] } else { $null }
$pidKeys = @($processIds.Keys | Sort-Object)
$latestProcessIds = if ($latest) { @($latest.ProcessIds | Sort-Object -Unique) } else { @() }
$latestProcessKey = Join-ProcessIds $latestProcessIds
$currentRunSamples = @()
if ($latestProcessKey) {
    for ($i = $sampleSummaries.Count - 1; $i -ge 0; $i--) {
        $sampleProcessKey = Join-ProcessIds @($sampleSummaries[$i].ProcessIds)
        if ($sampleProcessKey -ne $latestProcessKey) {
            break
        }
        $currentRunSamples = @($sampleSummaries[$i]) + $currentRunSamples
    }
}
$latestTopZone = if ($latest -and $latest.TopStuckZones -and $latest.TopStuckZones.Count -gt 0) { $latest.TopStuckZones[0].Key } else { $null }
$latestTopZoneSampleCount = if ($latestTopZone -and $zoneSamples.ContainsKey($latestTopZone)) { [int]$zoneSamples[$latestTopZone] } else { 0 }
$currentRunZoneSamples = @($currentRunSamples | Where-Object {
    $_.TopStuckZones -and $_.TopStuckZones.Count -gt 0 -and $_.TopStuckZones[0].Key -eq $latestTopZone
})
$currentRunLatestTopZoneSampleCount = $currentRunZoneSamples.Count
$latestVectorTopZone = if ($latest -and $latest.VectorTopStuckZones -and $latest.VectorTopStuckZones.Count -gt 0) { $latest.VectorTopStuckZones[0].Key } else { $null }
$latestVectorTopZoneSampleCount = if ($latestVectorTopZone -and $vectorZoneSamples.ContainsKey($latestVectorTopZone)) { [int]$vectorZoneSamples[$latestVectorTopZone] } else { 0 }
$currentRunVectorZoneSamples = @($currentRunSamples | Where-Object {
    $_.VectorTopStuckZones -and $_.VectorTopStuckZones.Count -gt 0 -and $_.VectorTopStuckZones[0].Key -eq $latestVectorTopZone
})
$currentRunLatestVectorTopZoneSampleCount = $currentRunVectorZoneSamples.Count

$trendRead = @()
if ($pidKeys.Count -eq 1) {
    $trendRead += "Visible current-build PID stayed stable across sampled notes: $($pidKeys[0])."
} elseif ($pidKeys.Count -gt 1) {
    $trendRead += "Multiple Unreal process IDs appear across sampled notes: $($pidKeys -join ', ')."
} else {
    $trendRead += 'No Unreal process ID was present in sampled notes.'
}
if ($latestProcessKey) {
    $trendRead += "Current run PID set $latestProcessKey appears in the latest $($currentRunSamples.Count) consecutive sampled note(s)."
}
if ($latestTopZone) {
    $trendRead += "Latest top stuck zone is $latestTopZone and it appears in $latestTopZoneSampleCount sampled note(s)."
    if ($currentRunSamples.Count -gt 0) {
        $trendRead += "Within the current PID run, latest top stuck zone $latestTopZone appears as the top zone in $currentRunLatestTopZoneSampleCount of $($currentRunSamples.Count) sampled note(s)."
    }
}
if ($latestVectorTopZone) {
    $trendRead += "Latest Vector diagnosis top stuck zone is $latestVectorTopZone and it appears in $latestVectorTopZoneSampleCount sampled note(s)."
    if ($currentRunSamples.Count -gt 0) {
        $trendRead += "Within the current PID run, Vector top stuck zone $latestVectorTopZone appears as the top zone in $currentRunLatestVectorTopZoneSampleCount of $($currentRunSamples.Count) sampled note(s)."
    }
}
if ($latest -and $null -ne $latest.VectorBotStuckRecoveryCount -and $latest.VectorBotStuckRecoveryCount -gt 0) {
    if ($latest.VectorBotUnstickQueryCount -eq 0) {
        $trendRead += 'Latest Vector diagnosis still has stuck recovery but no BotUnstickQuery coverage.'
    }
    if ($latest.VectorBotFiringPositionCount -eq 0) {
        $trendRead += 'Latest Vector diagnosis still has stuck recovery but no BotFiringPosition coverage.'
    }
}
if (-not ($featureKeys.ContainsKey('take_firing_position'))) {
    $trendRead += 'No sampled watch note includes take_firing_position coverage.'
}

$observedUtc = (Get-Date).ToUniversalTime().ToString('o')
$result = [pscustomobject]@{
    Schema = 'tinyassets.splitroot.live_canary_watch_trend.v1'
    Agent = 'Hex'
    ObservedUtc = $observedUtc
    GeneratedAtUtc = $observedUtc
    ProjectRoot = $ProjectRoot
    NonDisruptiveOnly = $true
    OutputPath = $OutputPath
    SourcePattern = 'Saved\Proof\hex-live-canary-watch-note-*.json'
    NotesFound = $noteFiles.Count
    NotesParsed = $notes.Count
    EarliestObservedUtc = if ($notes.Count -gt 0) { $notes[0].ObservedUtc.ToString('o') } else { $null }
    LatestObservedUtc = if ($notes.Count -gt 0) { $notes[-1].ObservedUtc.ToString('o') } else { $null }
    ProcessIds = $pidKeys
    ProcessIdStable = $pidKeys.Count -eq 1
    LatestProcessIds = $latestProcessIds
    CurrentRunProcessIdStable = $latestProcessKey -and $currentRunSamples.Count -gt 1
    CurrentRunSampleCount = $currentRunSamples.Count
    CurrentRunEarliestObservedUtc = if ($currentRunSamples.Count -gt 0) { $currentRunSamples[0].ObservedUtc } else { $null }
    AggregateTopStuckZones = Get-TopCounts $zoneTotals 10
    StuckZoneSampleCounts = Get-TopCounts $zoneSamples 10
    AggregateVectorTopStuckZones = Get-TopCounts $vectorZoneTotals 10
    VectorStuckZoneSampleCounts = Get-TopCounts $vectorZoneSamples 10
    LatestTopStuckZone = $latestTopZone
    LatestTopStuckZoneSampleCount = $latestTopZoneSampleCount
    CurrentRunLatestTopZoneSampleCount = $currentRunLatestTopZoneSampleCount
    LatestVectorTopStuckZone = $latestVectorTopZone
    LatestVectorTopStuckZoneSampleCount = $latestVectorTopZoneSampleCount
    CurrentRunLatestVectorTopZoneSampleCount = $currentRunLatestVectorTopZoneSampleCount
    FeatureCoverageKeysSeen = @($featureKeys.Keys | Sort-Object)
    Samples = $sampleSummaries
    DistinctReads = @($reads | Sort-Object -Unique)
    TrendRead = $trendRead
}

$result | ConvertTo-Json -Depth 10 | Set-Content -LiteralPath $OutputPath -Encoding UTF8
$result | ConvertTo-Json -Depth 10
