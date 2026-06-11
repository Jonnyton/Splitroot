<#
.SYNOPSIS
Imports downloaded Fab source packs into Unreal-ready /Game/Fab assets.

.DESCRIPTION
The Unreal commandlet currently saves imported assets and then can return
exit 3 on a Slate assertion. This runner keeps one import_asset_tasks call
per commandlet process, records each log, then runs a final inventory pass
that must exit cleanly.
#>
param(
    [string]$ProjectRoot = '',
    [string]$EngineRoot = 'C:\Program Files\Epic Games\UE_5.7',
    [int]$WaitSeconds = 120,
    [switch]$InventoryOnly,
    [int]$MaxImports = 0,
    [switch]$RetryRecorded,
    [string]$TaskKey = ''
)

$ErrorActionPreference = 'Stop'

if (-not $ProjectRoot) {
    $ProjectRoot = (Resolve-Path (Join-Path (Split-Path -Parent $MyInvocation.MyCommand.Path) '..')).Path
}

$project = Join-Path $ProjectRoot 'ArchonFactoryCanary.uproject'
$script = Join-Path $ProjectRoot 'Proof\import_fab_library_assets.py'
$editorCmd = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$resultPath = Join-Path $ProjectRoot 'Saved\Proof\last-fab-library-import.json'
$logRoot = Join-Path $ProjectRoot 'Saved\Proof\FabImportLibrary'
$summaryPath = Join-Path $logRoot 'import-wave-summary.json'
$lockPath = Join-Path $ProjectRoot 'Saved\Proof\build-lock.flag'

New-Item -ItemType Directory -Path $logRoot -Force | Out-Null

$runningEditors = Get-Process UnrealEditor* -ErrorAction SilentlyContinue
if ($runningEditors) {
    Write-Output 'FabImport: live Unreal editor running; no build-lock created and no editor stopped'
    Write-Output 'FabImport: the live SPLITROOT canary is shared telemetry/playtest infrastructure'
    Write-Output 'FabImport: ask Jonathan before any import that requires the editor/game to close'
    Write-Output ($runningEditors | Select-Object Id,ProcessName,MainWindowTitle | Format-Table -AutoSize | Out-String)
    exit 2
}

New-Item -ItemType File -Path $lockPath -Force | Out-Null

function Wait-ForEditors {
    $deadline = (Get-Date).AddSeconds($WaitSeconds)
    while ((Get-Date) -lt $deadline -and (Get-Process UnrealEditor* -ErrorAction SilentlyContinue)) {
        Start-Sleep -Seconds 5
    }
    $still = Get-Process UnrealEditor* -ErrorAction SilentlyContinue
    if ($still) {
        Write-Output 'FabImport: live Unreal editor still running; refusing to stop it'
        Write-Output ($still | Select-Object Id,ProcessName,MainWindowTitle | Format-Table -AutoSize | Out-String)
        exit 2
    }
}

function Invoke-FabCommandlet {
    param(
        [string]$LogPath,
        [string]$SelectedKey = '',
        [switch]$InventoryPass
    )

    $oldSelected = $env:FAB_LIBRARY_SELECTED_KEY
    $oldInventory = $env:FAB_LIBRARY_INVENTORY_ONLY
    try {
        if ($SelectedKey) {
            $env:FAB_LIBRARY_SELECTED_KEY = $SelectedKey
        } else {
            Remove-Item Env:FAB_LIBRARY_SELECTED_KEY -ErrorAction SilentlyContinue
        }
        if ($InventoryPass) {
            $env:FAB_LIBRARY_INVENTORY_ONLY = '1'
        } else {
            Remove-Item Env:FAB_LIBRARY_INVENTORY_ONLY -ErrorAction SilentlyContinue
        }

        & $editorCmd $project -run=pythonscript "-script=$script" -unattended -nop4 -nosplash -stdout -FullStdOutLogOutput *> $LogPath
        return $LASTEXITCODE
    }
    finally {
        if ($null -ne $oldSelected) {
            $env:FAB_LIBRARY_SELECTED_KEY = $oldSelected
        } else {
            Remove-Item Env:FAB_LIBRARY_SELECTED_KEY -ErrorAction SilentlyContinue
        }
        if ($null -ne $oldInventory) {
            $env:FAB_LIBRARY_INVENTORY_ONLY = $oldInventory
        } else {
            Remove-Item Env:FAB_LIBRARY_INVENTORY_ONLY -ErrorAction SilentlyContinue
        }
    }
}

function Read-InterestingLines {
    param([string]$LogPath)
    if (-not (Test-Path -LiteralPath $LogPath)) {
        return @()
    }
    $patterns = 'Saving Package|Imported |Error:|Warning:|Assertion failed|LogPython'
    return @(Select-String -Path $LogPath -Pattern $patterns | Select-Object -First 20 | ForEach-Object { $_.Line })
}

try {
    Wait-ForEditors

    $planLog = Join-Path $logRoot 'inventory-before.log'
    $planExit = Invoke-FabCommandlet -LogPath $planLog -InventoryPass
    if ($planExit -ne 0) {
        throw "Initial inventory commandlet failed with exit $planExit. See $planLog"
    }

    $result = Get-Content -LiteralPath $resultPath -Raw | ConvertFrom-Json
    $queue = @($result.import_queue)
    if ($TaskKey) {
        $queue = @($queue | Where-Object { $_.task_key -eq $TaskKey })
        if ($queue.Count -eq 0) {
            throw "TaskKey not found in import queue: $TaskKey"
        }
    }
    $recorded = @()
    if ((Test-Path -LiteralPath $summaryPath) -and -not $RetryRecorded -and -not $TaskKey -and -not $InventoryOnly) {
        $recorded = @(Get-Content -LiteralPath $summaryPath -Raw | ConvertFrom-Json)
        $recordedKeys = @{}
        foreach ($entry in $recorded) {
            if ($entry.TaskKey) {
                $recordedKeys[$entry.TaskKey] = $true
            }
        }
        $queue = @($queue | Where-Object { -not $recordedKeys.ContainsKey($_.task_key) })
    }
    if ($MaxImports -gt 0 -and -not $TaskKey) {
        $queue = @($queue | Select-Object -First $MaxImports)
    }

    $summary = @($recorded)
    $hadImportFailure = $false
    if (-not $InventoryOnly) {
        foreach ($item in $queue) {
            $safeKey = ($item.task_key -replace '[^A-Za-z0-9_.-]+', '_')
            $logPath = Join-Path $logRoot ("import-$safeKey.log")
            Write-Output ("FabImport: {0}" -f $item.task_key)
            $exitCode = Invoke-FabCommandlet -LogPath $logPath -SelectedKey $item.task_key
            $interesting = Read-InterestingLines -LogPath $logPath
            $accepted = $exitCode -eq 0 -or ($exitCode -eq 3 -and ($interesting -match 'Saving Package|Imported '))
            $summary += [pscustomobject]@{
                TaskKey = $item.task_key
                Source = $item.source
                Destination = $item.destination
                Kind = $item.kind
                ExitCode = $exitCode
                Accepted = [bool]$accepted
                LogPath = $logPath
                Interesting = $interesting
            }
            if (-not $accepted) {
                $hadImportFailure = $true
                $summary | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $summaryPath -Encoding UTF8
                Write-Warning "Import failed for $($item.task_key) with exit $exitCode. See $logPath"
            }
        }
    }

    $finalLog = Join-Path $logRoot 'inventory-after.log'
    $finalExit = Invoke-FabCommandlet -LogPath $finalLog -InventoryPass
    $summary | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath $summaryPath -Encoding UTF8
    if ($finalExit -ne 0) {
        throw "Final inventory commandlet failed with exit $finalExit. See $finalLog"
    }

    Write-Output ("FabImport: imported {0} selected sources; summary {1}" -f $summary.Count, $summaryPath)
    if ($hadImportFailure) {
        exit 1
    }
    exit 0
}
finally {
    Remove-Item -LiteralPath $lockPath -Force -ErrorAction SilentlyContinue
}
