param(
    [string]$ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..\..')).Path,
    [string]$LogPath = '',
    [string]$SnapshotPath = '',
    [string]$PendingPath = '',
    [string]$HotspotManifestPath = '',
    [string]$SupervisorPidPath = '',
    [string]$SupervisorLogPath = '',
    [string]$OutputPath = '',
    [int]$TailLines = 5000
)

$ErrorActionPreference = 'Stop'

if (-not $LogPath) {
    $LogPath = Join-Path $ProjectRoot 'Saved\Logs\ArchonFactoryCanary.log'
}
if (-not $SnapshotPath) {
    $SnapshotPath = Join-Path $ProjectRoot 'Saved\Proof\last-live-bot-match-log-snapshot.json'
}
if (-not $PendingPath) {
    $PendingPath = Join-Path $ProjectRoot 'Saved\Proof\host-refresh-pending.json'
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
    $OutputPath = Join-Path $ProjectRoot 'Saved\Proof\live-adoption-ledger.json'
}

$proofDir = Split-Path -Parent $OutputPath
New-Item -ItemType Directory -Force -Path $proofDir | Out-Null

function Read-JsonOrNull([string]$Path) {
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        return $null
    }

    try {
        return Get-Content -LiteralPath $Path -Raw | ConvertFrom-Json
    } catch {
        return $null
    }
}

function Get-PropertyOrNull($Object, [string]$Name) {
    if ($null -eq $Object) {
        return $null
    }

    $property = $Object.PSObject.Properties[$Name]
    if ($null -eq $property) {
        return $null
    }

    return $property.Value
}

function Get-RelativePath([string]$Path) {
    $root = (Resolve-Path -LiteralPath $ProjectRoot).Path.TrimEnd('\')
    $full = if (Test-Path -LiteralPath $Path) {
        (Resolve-Path -LiteralPath $Path).Path
    } else {
        $Path
    }

    if ($full.StartsWith($root, [System.StringComparison]::OrdinalIgnoreCase)) {
        return $full.Substring($root.Length).TrimStart('\')
    }

    return $full
}

function Get-FileState([string]$Path) {
    $fullPath = Join-Path $ProjectRoot $Path
    if (-not (Test-Path -LiteralPath $fullPath -PathType Leaf)) {
        return [pscustomobject]@{
            Path = $Path
            Exists = $false
            LastWriteTimeUtc = $null
        }
    }

    $item = Get-Item -LiteralPath $fullPath
    return [pscustomobject]@{
        Path = $Path
        Exists = $true
        LastWriteTimeUtc = $item.LastWriteTimeUtc.ToString('o')
    }
}

function Test-ExcludedInputPath([string]$Path, [string[]]$ExcludePathFragments) {
    if (-not $Path -or -not $ExcludePathFragments) {
        return $false
    }

    $normalized = $Path.Replace('/', '\')
    foreach ($fragment in $ExcludePathFragments) {
        if (-not $fragment) {
            continue
        }
        if ($normalized.IndexOf($fragment.Replace('/', '\'), [System.StringComparison]::OrdinalIgnoreCase) -ge 0) {
            return $true
        }
    }

    return $false
}

function Test-IncludedFileName([string]$Name, [string[]]$Include) {
    if (-not $Include -or $Include.Count -eq 0) {
        return $true
    }

    foreach ($pattern in $Include) {
        if ($Name -like $pattern) {
            return $true
        }
    }

    return $false
}

function Get-NewestFile([string[]]$Roots, [string[]]$Include, [string[]]$ExcludePathFragments = @()) {
    $files = @()
    foreach ($root in $Roots) {
        $fullRoot = Join-Path $ProjectRoot $root
        if (Test-Path -LiteralPath $fullRoot -PathType Container) {
            $files += @(Get-ChildItem -LiteralPath $fullRoot -Recurse -File |
                Where-Object {
                    (Test-IncludedFileName -Name $_.Name -Include $Include) -and
                    -not (Test-ExcludedInputPath -Path $_.FullName -ExcludePathFragments $ExcludePathFragments)
                })
        }
    }

    if ($files.Count -eq 0) {
        return $null
    }

    return $files | Sort-Object LastWriteTimeUtc -Descending | Select-Object -First 1
}

function Get-NewerFiles([string[]]$Roots, [string[]]$Include, [datetime]$AfterUtc, [string[]]$ExcludePathFragments = @()) {
    $files = @()
    foreach ($root in $Roots) {
        $fullRoot = Join-Path $ProjectRoot $root
        if (Test-Path -LiteralPath $fullRoot -PathType Container) {
            $files += @(Get-ChildItem -LiteralPath $fullRoot -Recurse -File |
                Where-Object {
                    (Test-IncludedFileName -Name $_.Name -Include $Include) -and
                    $_.LastWriteTimeUtc -gt $AfterUtc -and
                    -not (Test-ExcludedInputPath -Path $_.FullName -ExcludePathFragments $ExcludePathFragments)
                })
        }
    }

    return @($files | Sort-Object LastWriteTimeUtc -Descending)
}

function Get-TailText([string]$Path, [int]$LineCount) {
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        return ''
    }

    return (Get-Content -LiteralPath $Path -Tail $LineCount) -join "`n"
}

function Get-TextTail([string]$Path, [int]$LineCount) {
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        return @()
    }

    return @(Get-Content -LiteralPath $Path -Tail $LineCount | ForEach-Object { [string]$_ })
}

function Count-Regex([string]$Text, [string]$Pattern) {
    if (-not $Text -or -not $Pattern) {
        return 0
    }

    return [regex]::Matches($Text, $Pattern).Count
}

function Read-TaskFiles {
    $taskFiles = @()
    foreach ($folder in @('Coordination\inbox-hex', 'Coordination\done')) {
        $fullFolder = Join-Path $ProjectRoot $folder
        if (-not (Test-Path -LiteralPath $fullFolder -PathType Container)) {
            continue
        }

        foreach ($file in Get-ChildItem -LiteralPath $fullFolder -File) {
            $content = Get-Content -LiteralPath $file.FullName -Raw
            $taskFiles += [pscustomobject]@{
                Name = $file.Name
                Folder = $folder
                Path = Get-RelativePath $file.FullName
                HasResult = $content -match '(?m)^## Result\b'
                Blocked = $content -match '(?i)\bblocked\b|blocked_requires_handoff|required live-session handoff'
                StagedNotLive = $content -match '(?i)staged|not live|not loaded|not adopted|no live-adoption claim|has not loaded'
                ProofOnly = $content -match '(?i)proof-only|proof only|read-only evidence'
                Completed = $content -match '(?i)completed by|Status:\s*completed'
            }
        }
    }

    return @($taskFiles)
}

function Test-AnyNameMatch($Task, [string]$Pattern) {
    if (-not $Pattern) {
        return $false
    }

    return $Task.Name -match $Pattern
}

$snapshot = Read-JsonOrNull $SnapshotPath
$pending = Read-JsonOrNull $PendingPath
$hotspotManifest = Read-JsonOrNull $HotspotManifestPath
$supervisorState = Read-JsonOrNull $SupervisorPidPath
$supervisorLogTail = Get-TextTail $SupervisorLogPath 8
$supervisorProcess = $null
$supervisorCommandLine = $null
if ($supervisorState -and $supervisorState.Pid) {
    $supervisorProcessId = [int]$supervisorState.Pid
    $supervisorProcess = Get-Process -Id $supervisorProcessId -ErrorAction SilentlyContinue
    $supervisorCim = Get-CimInstance Win32_Process -Filter "ProcessId = $supervisorProcessId" -ErrorAction SilentlyContinue
    if ($supervisorCim) {
        $supervisorCommandLine = $supervisorCim.CommandLine
    }
}
$tailText = Get-TailText $LogPath $TailLines
$taskFiles = Read-TaskFiles

$dllPath = Join-Path $ProjectRoot 'Binaries\Win64\UnrealEditor-ArchonFactoryCanary.dll'
$dllItem = if (Test-Path -LiteralPath $dllPath -PathType Leaf) { Get-Item -LiteralPath $dllPath } else { $null }
$dllUtc = if ($dllItem) { $dllItem.LastWriteTimeUtc } else { [datetime]::MinValue }
$newestSource = Get-NewestFile @('Source\ArchonFactoryCanary') @('*.h', '*.cpp', '*.cs')
$contractExcludeFragments = @('\FactoryContracts\thumbs\generated\')
$newestContract = Get-NewestFile @('games\splitroot\FactoryContracts') @('*.json') $contractExcludeFragments
$newerSourceFiles = if ($dllItem) {
    Get-NewerFiles @('Source\ArchonFactoryCanary') @('*.h', '*.cpp', '*.cs') $dllUtc
} else {
    @()
}
$newerContractFiles = if ($dllItem) {
    Get-NewerFiles @('games\splitroot\FactoryContracts') @('*.json') $dllUtc $contractExcludeFragments
} else {
    @()
}

$runningUnrealProcesses = @(Get-Process UnrealEditor -ErrorAction SilentlyContinue |
    Select-Object Id, ProcessName, MainWindowTitle, Path, StartTime)

$mechanicDefinitions = @(
    @{
        Key = 'bot_strategy_tuning_loader'
        Owner = 'Vector/Hex'
        SourceHints = @('ArchonBotStrategyTuningLibrary', 'ArchonBotAIController', 'ArchonBotBrainComponent', 'ArchonCanaryWorldSubsystem')
        ContractPath = 'games\splitroot\FactoryContracts\bot_strategy_tuning.json'
        LivePattern = 'BotStrategyTuningLoaded'
        SnapshotCountProperty = 'BotStrategyTuningLoadedCount'
        TaskPattern = '023-adopt-vector|024-vector-bot-strategy'
        ProofPath = 'Saved\Proof\last-bot-match-smoke.json'
    },
    @{
        Key = 'bot_unstick_query'
        Owner = 'Vector'
        SourceHints = @('ArchonBotSteeringPolicyLibrary', 'ArchonBotBrainComponent')
        ContractPath = ''
        LivePattern = 'BotUnstickQuery'
        SnapshotCountProperty = 'BotUnstickQueryCount'
        TaskPattern = '021-bot-unstick'
        ProofPath = 'Saved\Proof\last-bot-match-smoke.json'
    },
    @{
        Key = 'bot_firing_positions'
        Owner = 'Gauge/Vector'
        SourceHints = @('ArchonBotSteeringPolicyLibrary', 'ArchonBotBrainComponent')
        ContractPath = ''
        LivePattern = 'BotFiringPosition|feature=take_firing_position'
        SnapshotCountProperty = 'BotFiringPositionCount'
        TaskPattern = '021-bot-objective-firing-position'
        ProofPath = 'Saved\Proof\last-bot-match-smoke.json'
    },
    @{
        Key = 'rally_point_reinforcements'
        Owner = 'Gauge/Grid'
        SourceHints = @('ArchonTeamRtsStateComponent', 'ArchonTeamRtsPolicyLibrary', 'ArchonMapTableWidget', 'ArchonPlayerInputBridgeComponent', 'ArchonCanaryWorldSubsystem')
        ContractPath = ''
        LivePattern = 'RuntimeRallyPointSet|TeamRallyPointSet|ReinforcementRallyApplied'
        SnapshotCountProperty = ''
        TaskPattern = '022-rally-point'
        ProofPath = 'Saved\Proof\last-rally-reinforcement-smoke.json'
    },
    @{
        Key = 'tech_shop_loop'
        Owner = 'Grid/Gauge'
        SourceHints = @('ArchonItemShopPolicyLibrary', 'ArchonCanaryWorldSubsystem', 'ArchonMatchTechState', 'ArchonTechBuilding')
        ContractPath = ''
        LivePattern = 'TechBuilt|ShopRowUnlocked|TechBuildingDestroyed|ShopRowLockedOrInflatedAfterTechLoss'
        SnapshotCountProperty = ''
        TaskPattern = '013-build-reference-mechanics-gap'
        ProofPath = 'Saved\Proof\last-tech-shop-smoke.json'
    },
    @{
        Key = 'map_registry_rotation'
        Owner = 'Quarry/Hex'
        SourceHints = @('ArchonMapRegistry', 'ArchonCanaryWorldSubsystem')
        ContractPath = 'games\splitroot\FactoryContracts\map_registry.json'
        LivePattern = 'MapRegistryLoaded|NextMatchLoading|TravelingTo map='
        SnapshotCountProperty = ''
        TaskPattern = '014-live-replay|015-next-match|016-host-supervisor'
        ProofPath = 'Saved\Proof\last-live-match-restart-smoke.json'
    },
    @{
        Key = 'unit_asset_candidate_catalog'
        Owner = 'Ledger'
        SourceHints = @()
        ContractPath = 'games\splitroot\FactoryContracts\unit_asset_candidates.json'
        LivePattern = 'UnitAssetCatalogLoaded|ContentCatalogLoaded'
        SnapshotCountProperty = ''
        TaskPattern = '018-add-infinity|unit_asset'
        ProofPath = ''
    }
)

$mechanics = @()
foreach ($definition in $mechanicDefinitions) {
    $countFromTail = Count-Regex $tailText $definition.LivePattern
    $snapshotCount = 0
    if ($definition.SnapshotCountProperty) {
        $value = Get-PropertyOrNull $snapshot $definition.SnapshotCountProperty
        if ($null -ne $value) {
            $snapshotCount = [int]$value
        }
    }

    $matchingSource = @()
    foreach ($file in $newerSourceFiles) {
        foreach ($hint in $definition.SourceHints) {
            if ($file.Name -like "*$hint*" -or $file.FullName -like "*$hint*") {
                $matchingSource += $file
                break
            }
        }
    }

    $contractState = if ($definition.ContractPath) {
        Get-FileState $definition.ContractPath
    } else {
        $null
    }

    $contractNewerThanDll = $false
    if ($contractState -and $contractState.Exists -and $dllItem) {
        $contractNewerThanDll = ([datetime]$contractState.LastWriteTimeUtc) -gt $dllUtc
    }

    $relatedTasks = @($taskFiles | Where-Object { Test-AnyNameMatch $_ $definition.TaskPattern })
    $hasActiveTask = @($relatedTasks | Where-Object { $_.Folder -eq 'Coordination\inbox-hex' }).Count -gt 0
    $hasBlockedTask = @($relatedTasks | Where-Object { $_.Blocked }).Count -gt 0
    $hasProofOnlyTask = @($relatedTasks | Where-Object { $_.ProofOnly }).Count -gt 0
    $proofState = if ($definition.ProofPath) { Get-FileState $definition.ProofPath } else { $null }
    $livePresent = ($countFromTail -gt 0) -or ($snapshotCount -gt 0)

    $adoptionState = 'not_started'
    if ($livePresent) {
        $adoptionState = 'live_adopted'
    } elseif ($hasBlockedTask) {
        $adoptionState = 'blocked_requires_handoff'
    } elseif ($contractState -and $contractState.Exists -and $contractNewerThanDll) {
        $adoptionState = 'contract_staged_not_loaded'
    } elseif ($matchingSource.Count -gt 0) {
        $adoptionState = 'coded_not_built'
    } elseif ($proofState -and $proofState.Exists) {
        $adoptionState = 'built_not_live'
    } elseif ($hasActiveTask) {
        $adoptionState = 'queued'
    }

    $mechanics += [pscustomobject]@{
        Key = $definition.Key
        Owner = $definition.Owner
        AdoptionState = $adoptionState
        LiveMarkerPattern = $definition.LivePattern
        LiveMarkerCountInTail = $countFromTail
        SnapshotCount = $snapshotCount
        ActiveTask = $hasActiveTask
        BlockedTask = $hasBlockedTask
        ProofOnlyTask = $hasProofOnlyTask
        RelatedTasks = @($relatedTasks | Select-Object Name, Folder, Blocked, StagedNotLive, ProofOnly)
        NewerSourceFiles = @($matchingSource | Select-Object -First 8 @{Name='Path';Expression={Get-RelativePath $_.FullName}}, @{Name='LastWriteTimeUtc';Expression={$_.LastWriteTimeUtc.ToString('o')}})
        Contract = $contractState
        ContractNewerThanDll = $contractNewerThanDll
        Proof = $proofState
    }
}

$coordinationSummary = [pscustomobject]@{
    InboxHexCount = @($taskFiles | Where-Object { $_.Folder -eq 'Coordination\inbox-hex' }).Count
    DoneCount = @($taskFiles | Where-Object { $_.Folder -eq 'Coordination\done' }).Count
    BlockedCount = @($taskFiles | Where-Object { $_.Blocked }).Count
    StagedNotLiveCount = @($taskFiles | Where-Object { $_.StagedNotLive }).Count
    ProofOnlyCount = @($taskFiles | Where-Object { $_.ProofOnly }).Count
}
$coordinationStatusPath = Join-Path $ProjectRoot 'Saved\Proof\coordination-status-summary.json'
$coordinationStatus = Read-JsonOrNull $coordinationStatusPath
$currentCoordination = if ($coordinationStatus) {
    [pscustomobject]@{
        Source = 'Saved\Proof\coordination-status-summary.json'
        InboxHexCount = $coordinationStatus.ActiveInboxCount
        DoneCount = $coordinationStatus.DoneCount
        CompletedCount = $coordinationStatus.CompletedCount
        CompletedAndLiveCount = $coordinationStatus.CompletedAndLiveCount
        BlockedCount = $coordinationStatus.BlockedRequiresHandoffCount
        StagedNotLiveCount = $coordinationStatus.StagedNotLiveCount
        ProofOnlyCount = $coordinationStatus.ProofOnlyCount
        SupersededCount = $coordinationStatus.SupersededCount
    }
} else {
    [pscustomobject]@{
        Source = 'inline_legacy_text_scan'
        InboxHexCount = $coordinationSummary.InboxHexCount
        DoneCount = $coordinationSummary.DoneCount
        CompletedCount = $null
        CompletedAndLiveCount = $null
        BlockedCount = $coordinationSummary.BlockedCount
        StagedNotLiveCount = $coordinationSummary.StagedNotLiveCount
        ProofOnlyCount = $coordinationSummary.ProofOnlyCount
        SupersededCount = $null
    }
}

$queuedReplayManifestMarkers = @(
    'ReplaySchema version=...',
    'MatchBuildManifest matchId=... buildId=... moduleDllUtc=... processStartUtc=... sourceState=... sourceNewestUtc=...',
    'MechanicCatalog mechanic=... sourceStatus=... liveStatus=... owner=...',
    'ContentCatalogLoaded mapRegistryHash=... unitCatalogHash=... itemCatalogHash=... botTuningRevision=... botTuningLoaded=...',
    'HostRefreshState pending=... supervisorOwnsHost=... reason=...'
)

$read = @()
if ($newerSourceFiles.Count -gt 0) {
    $read += ('Source files newer than the loaded module DLL: ' + $newerSourceFiles.Count)
}
if ($newerContractFiles.Count -gt 0) {
    $read += ('Per-game contract files newer than the loaded module DLL: ' + $newerContractFiles.Count)
}
if ($currentCoordination.BlockedCount -gt 0 -or $currentCoordination.StagedNotLiveCount -gt 0) {
    $read += (
        'Current coordination blocked/staged counts: blocked=' +
        $currentCoordination.BlockedCount +
        ', staged=' + $currentCoordination.StagedNotLiveCount)
}
if (@($mechanics | Where-Object { $_.AdoptionState -ne 'live_adopted' }).Count -gt 0) {
    $read += 'At least one tracked mechanic is not proven live in the current match stream.'
}
if ($hotspotManifest -and $hotspotManifest.AdoptionState) {
    $read += ('Route hotspot manifest adoption state: ' + $hotspotManifest.AdoptionState)
}
if ($hotspotManifest -and $null -ne $hotspotManifest.LiveBotStrategyTuningLoadedCount) {
    $read += ('Live bot strategy tuning loaded count: ' + $hotspotManifest.LiveBotStrategyTuningLoadedCount)
}
if ($supervisorState) {
    $read += ('Host supervisor loop pid ' + $supervisorState.Pid + ' alive=' + [bool]$supervisorProcess + '; ledger treats it as observer state only.')
}

$result = [pscustomobject]@{
    Schema = 'tinyassets.splitroot.live_adoption_ledger.v1'
    Agent = 'Hex'
    ObservedUtc = (Get-Date).ToUniversalTime().ToString('o')
    ProjectRoot = $ProjectRoot
    NonDisruptiveOnly = $true
    TailLinesRead = $TailLines
    RunningUnrealProcesses = $runningUnrealProcesses
    LiveSnapshot = if ($snapshot) {
        [pscustomobject]@{
            SnapshotUtc = $snapshot.SnapshotUtc
            EvidenceReady = $snapshot.EvidenceReady
            MatchLoopActive = $snapshot.MatchLoopActive
            BotMatchSpawned = $snapshot.BotMatchSpawned
            LatestBuildFingerprint = $snapshot.LatestBuildFingerprint
        }
    } else { $null }
    Build = [pscustomobject]@{
        ModuleDllPath = if ($dllItem) { Get-RelativePath $dllItem.FullName } else { 'Binaries\Win64\UnrealEditor-ArchonFactoryCanary.dll' }
        ModuleDllExists = $null -ne $dllItem
        ModuleDllUtc = if ($dllItem) { $dllItem.LastWriteTimeUtc.ToString('o') } else { $null }
        NewestSource = if ($newestSource) {
            [pscustomobject]@{
                Path = Get-RelativePath $newestSource.FullName
                LastWriteTimeUtc = $newestSource.LastWriteTimeUtc.ToString('o')
            }
        } else { $null }
        NewestContract = if ($newestContract) {
            [pscustomobject]@{
                Path = Get-RelativePath $newestContract.FullName
                LastWriteTimeUtc = $newestContract.LastWriteTimeUtc.ToString('o')
            }
        } else { $null }
        SourceFilesNewerThanDllCount = $newerSourceFiles.Count
        ContractFilesNewerThanDllCount = $newerContractFiles.Count
        PendingSourceFiles = @($newerSourceFiles |
            Select-Object -First 12 @{Name='Path';Expression={Get-RelativePath $_.FullName}}, @{Name='LastWriteTimeUtc';Expression={$_.LastWriteTimeUtc.ToString('o')}})
        PendingContractFiles = @($newerContractFiles |
            Select-Object -First 12 @{Name='Path';Expression={Get-RelativePath $_.FullName}}, @{Name='LastWriteTimeUtc';Expression={$_.LastWriteTimeUtc.ToString('o')}})
        PendingSourceFilesTruncated = $newerSourceFiles.Count -gt 12
        PendingContractFilesTruncated = $newerContractFiles.Count -gt 12
        SourceState = if ($newerSourceFiles.Count -gt 0) { 'newer_source_pending' } else { 'loaded_or_unknown' }
    }
    HostRefresh = if ($pending) {
        [pscustomobject]@{
            Pending = $pending.Pending
            Reason = $pending.Reason
            SupervisorOwnsHost = $pending.SupervisorOwnsHost
            SupervisorMode = $pending.SupervisorMode
            BoundaryPolicy = $pending.BoundaryPolicy
            CurrentProcessId = $pending.CurrentProcessId
            CurrentModuleDllUtc = $pending.CurrentModuleDllUtc
            PendingModuleDllUtc = $pending.PendingModuleDllUtc
        }
    } else { $null }
    HotspotManifest = if ($hotspotManifest) {
        [pscustomobject]@{
            Path = Get-RelativePath $HotspotManifestPath
            ObservedUtc = $hotspotManifest.ObservedUtc
            AdoptionState = $hotspotManifest.AdoptionState
            PendingNativeRefresh = $hotspotManifest.PendingNativeRefresh
            SupervisorOwnsHost = $hotspotManifest.SupervisorOwnsHost
            StagedBotStrategyRevision = $hotspotManifest.StagedBotStrategyRevision
            LiveBotStrategyTuningLoadedCount = $hotspotManifest.LiveBotStrategyTuningLoadedCount
            LatestVectorDiagnosisTopStuckZone = $hotspotManifest.LatestVectorDiagnosisTopStuckZone
            NextAction = $hotspotManifest.NextAction
        }
    } else { $null }
    SupervisorLoop = if ($supervisorState) {
        [pscustomobject]@{
            Path = Get-RelativePath $SupervisorPidPath
            Pid = $supervisorState.Pid
            ProcessAlive = [bool]$supervisorProcess
            StartedUtc = $supervisorState.StartedUtc
            ProjectRoot = $supervisorState.ProjectRoot
            AllowBoundaryRefresh = $supervisorState.AllowBoundaryRefresh
            BuildAtBoundary = $supervisorState.BuildAtBoundary
            BuildScriptPath = if ($supervisorState.BuildScriptPath) { Get-RelativePath $supervisorState.BuildScriptPath } else { $null }
            CommandLine = $supervisorCommandLine
            LatestLogTail = @($supervisorLogTail)
        }
    } else { $null }
    Coordination = $currentCoordination
    CoordinationLegacyTextScan = $coordinationSummary
    CoordinationStatus = $coordinationStatus
    Mechanics = $mechanics
    QueuedReplayManifestMarkers = [pscustomobject]@{
        Status = 'queued_for_next_safe_cxx_boundary'
        Reason = 'Adding runtime log markers requires a future C++ build/adoption boundary; this pass is read-only against the visible canary.'
        Markers = $queuedReplayManifestMarkers
    }
    Read = $read
}

$json = $result | ConvertTo-Json -Depth 12
$json | Set-Content -LiteralPath $OutputPath -Encoding UTF8
$json
