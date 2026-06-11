<#
.SYNOPSIS
Canonical build entry while the perpetual playtest loop is running:
creates the build-lock flag (the loop stands its session down), WAITS
for every UnrealEditor process to actually exit (the DLL link loses
races otherwise — learned 2026-06-10), runs build-and-test-policy,
then releases the lock so the loop relaunches on the new build.
#>
param(
    [string]$ProjectRoot = '',
    [int]$WaitSeconds = 120
)
$ErrorActionPreference = 'Continue'
if (-not $ProjectRoot) {
    $ProjectRoot = (Resolve-Path (Join-Path (Split-Path -Parent $MyInvocation.MyCommand.Path) '..')).Path
}
$lockPath = Join-Path $ProjectRoot 'Saved\Proof\build-lock.flag'

$running = Get-Process UnrealEditor* -ErrorAction SilentlyContinue
if ($running) {
    Write-Output 'BuildLocked: live Unreal editor running; no build-lock created and no editor stopped'
    Write-Output 'BuildLocked: the live SPLITROOT canary is shared telemetry/playtest infrastructure'
    Write-Output 'BuildLocked: ask Jonathan before any build that requires the editor/game to close'
    Write-Output ($running | Select-Object Id,ProcessName,MainWindowTitle | Format-Table -AutoSize | Out-String)
    exit 2
}

New-Item -ItemType File -Path $lockPath -Force | Out-Null
try {
    $deadline = (Get-Date).AddSeconds($WaitSeconds)
    while ((Get-Date) -lt $deadline -and (Get-Process UnrealEditor* -ErrorAction SilentlyContinue)) {
        Start-Sleep -Seconds 5
    }
    $still = Get-Process UnrealEditor* -ErrorAction SilentlyContinue
    if ($still) {
        Write-Output 'BuildLocked: live Unreal editor still running; refusing to stop it'
        Write-Output ($still | Select-Object Id,ProcessName,MainWindowTitle | Format-Table -AutoSize | Out-String)
        exit 2
    }

    powershell -ExecutionPolicy Bypass -File (Join-Path $ProjectRoot 'Proof\build-and-test-policy.ps1')
    exit $LASTEXITCODE
}
finally {
    Remove-Item -LiteralPath $lockPath -Force -ErrorAction SilentlyContinue
}
