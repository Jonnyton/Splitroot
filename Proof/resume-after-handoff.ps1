<#
.SYNOPSIS
One-shot resume runner for the 2026-06-11 Cowork session (Rook).
The Cowork harness has no host shell, so this script is launched on
the host (double-click the .cmd or Win+R) and does the sequence the
handoff requires:

1. clear the stale stray Proof\build-lock.flag (old locked-window job
   died holding it; the canonical lock lives at Saved\Proof\).
2. build via build-locked.ps1 (loop handshake + editor drain).
3. GATE on last-policy-build-and-test.json (BuildExitCode==0 AND
   AutomationFailedCount==0) - never chain smokes on a failed build.
4. run unreal-map-smoke.ps1, then grep the newest session log for the
   MapTable load failure; write the verdict to
   Saved\Proof\last-table-fix-verify.json.
5. start or resume the perpetual playtest monitor without touching an existing
   live game instance.

Everything is also transcribed to Saved\Proof\resume-after-handoff.log
so a sandboxed session can read the outcome.
#>
param(
    [string]$ProjectRoot = ''
)
$ErrorActionPreference = 'Continue'
if (-not $ProjectRoot) {
    $ProjectRoot = (Resolve-Path (Join-Path (Split-Path -Parent $MyInvocation.MyCommand.Path) '..')).Path
}
$logPath = Join-Path $ProjectRoot 'Saved\Proof\resume-after-handoff.log'
Start-Transcript -Path $logPath -Force | Out-Null

try {
    # 1. stale stray lock from the dead locked-window job
    $strayLock = Join-Path $ProjectRoot 'Proof\build-lock.flag'
    if (Test-Path -LiteralPath $strayLock) {
        Remove-Item -LiteralPath $strayLock -Force
        Write-Output 'Resume: removed stale stray Proof\build-lock.flag'
    }

    # 1b. Crash reporters are evidence. Do not dismiss them from automation.
    $crashReporters = @(Get-Process CrashReportClient* -ErrorAction SilentlyContinue)
    if ($crashReporters.Count -gt 0) {
        Write-Output "Resume: crash reporter visible; leaving it open for evidence ($($crashReporters.Count))."
    }

    # 2. locked build (handles Saved\Proof\build-lock.flag + editor drain)
    powershell -ExecutionPolicy Bypass -File (Join-Path $ProjectRoot 'Proof\build-locked.ps1')

    # 3. gate
    $policyPath = Join-Path $ProjectRoot 'Saved\Proof\last-policy-build-and-test.json'
    $policy = Get-Content -LiteralPath $policyPath -Raw | ConvertFrom-Json
    if ($policy.BuildExitCode -ne 0 -or $policy.AutomationFailedCount -ne 0) {
        Write-Output "Resume: GATE FAILED - BuildExitCode=$($policy.BuildExitCode) AutomationFailedCount=$($policy.AutomationFailedCount). Stopping; no smokes, no loop."
        exit 1
    }
    Write-Output "Resume: gate green - BuildExitCode=0, $($policy.AutomationSucceededCount) tests passed."

    # 4. map smoke + table-load verdict
    powershell -ExecutionPolicy Bypass -File (Join-Path $ProjectRoot 'Proof\unreal-map-smoke.ps1')
    $newestLog = Get-ChildItem (Join-Path $ProjectRoot 'Saved\Logs\*.log') |
        Sort-Object LastWriteTime -Descending | Select-Object -First 1
    $tableFailures = @(Select-String -LiteralPath $newestLog.FullName -Pattern 'Failed to find /Game/StandIns/MapTable' -SimpleMatch)
    $tableLoaded = @(Select-String -LiteralPath $newestLog.FullName -Pattern 'StandIns/MapTable/wooden_table_021' -SimpleMatch)
    $verdict = [pscustomobject]@{
        When               = (Get-Date).ToString('s')
        SmokeExitCode      = $LASTEXITCODE
        NewestLog          = $newestLog.FullName
        TableLoadFailures  = $tableFailures.Count
        Table021References = $tableLoaded.Count
        TableFixVerified   = ($tableFailures.Count -eq 0 -and $tableLoaded.Count -gt 0)
    }
    $verdict | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $ProjectRoot 'Saved\Proof\last-table-fix-verify.json') -Encoding UTF8
    Write-Output "Resume: table verdict - failures=$($tableFailures.Count) (want 0), 021-refs=$($tableLoaded.Count)."

    # 5. perpetual loop monitor, persistent window (normal, never minimized - D3D12)
    Start-Process powershell -ArgumentList @(
        '-ExecutionPolicy', 'Bypass', '-NoExit',
        '-File', (Join-Path $ProjectRoot 'Proof\playtest-loop.ps1')
    ) -WindowStyle Normal
    Write-Output 'Resume: perpetual playtest loop relaunched in its own window.'
}
finally {
    Stop-Transcript | Out-Null
}
