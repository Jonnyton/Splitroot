param(
    [string]$ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path,
    [string]$EngineRoot = 'C:\Program Files\Epic Games\UE_5.7'
)

$ErrorActionPreference = 'Stop'

$projectFile = Join-Path $ProjectRoot 'ArchonFactoryCanary.uproject'
$buildBat = Join-Path $EngineRoot 'Engine\Build\BatchFiles\Build.bat'
$editorCmd = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$proofDir = Join-Path $ProjectRoot 'Saved\Proof'
$automationDir = Join-Path $proofDir 'Automation'
$buildLogPath = Join-Path $proofDir 'last-policy-build.log'
$automationLogPath = Join-Path $proofDir 'last-policy-automation.log'
$summaryPath = Join-Path $proofDir 'last-policy-build-and-test.json'

New-Item -ItemType Directory -Force -Path $proofDir | Out-Null
if (Test-Path -LiteralPath $automationDir) {
    Remove-Item -LiteralPath $automationDir -Recurse -Force
}
New-Item -ItemType Directory -Force -Path $automationDir | Out-Null

$buildOutput = & $buildBat `
    ArchonFactoryCanaryEditor `
    Win64 `
    Development `
    "-Project=$projectFile" `
    -WaitMutex `
    -NoHotReloadFromIDE 2>&1
$buildExitCode = $LASTEXITCODE
($buildOutput | ForEach-Object { $_.ToString() }) | Set-Content -LiteralPath $buildLogPath -Encoding UTF8

$automationStartUtc = (Get-Date).ToUniversalTime()
$automationExitCode = $null
if ($buildExitCode -eq 0) {
    $automationOutput = & $editorCmd `
        $projectFile `
        -NullRHI `
        -NoSound `
        -NoSplash `
        -unattended `
        -nop4 `
        -stdout `
        -FullStdOutLogOutput `
        '-ExecCmds=Automation RunTests ArchonFactory' `
        '-TestExit=Automation Test Queue Empty' `
        "-ReportExportPath=$automationDir" 2>&1
    $automationExitCode = $LASTEXITCODE
    ($automationOutput | ForEach-Object { $_.ToString() }) | Set-Content -LiteralPath $automationLogPath -Encoding UTF8
} else {
    "Build failed with exit code $buildExitCode; automation skipped." | Set-Content -LiteralPath $automationLogPath -Encoding UTF8
}

$reportPath = Join-Path $automationDir 'index.json'
$report = $null
if (Test-Path -LiteralPath $reportPath -PathType Leaf) {
    $reportInfo = Get-Item -LiteralPath $reportPath
    if ($reportInfo.LastWriteTimeUtc -ge $automationStartUtc) {
        $report = Get-Content -LiteralPath $reportPath -Raw | ConvertFrom-Json
    }
}

$result = [pscustomobject]@{
    ProjectRoot = $ProjectRoot
    ProjectFile = $projectFile
    BuildExitCode = $buildExitCode
    AutomationExitCode = $automationExitCode
    BuildLogPath = $buildLogPath
    AutomationLogPath = $automationLogPath
    AutomationReportPath = $reportPath
    CompiledEditorModuleExists = Test-Path -LiteralPath (Join-Path $ProjectRoot 'Binaries\Win64\UnrealEditor-ArchonFactoryCanary.dll') -PathType Leaf
    AutomationSucceeded = [bool]($report -and $report.succeeded -ge 4 -and $report.failed -eq 0)
    AutomationSucceededCount = if ($report) { $report.succeeded } else { $null }
    AutomationFailedCount = if ($report) { $report.failed } else { $null }
    TestPaths = if ($report) { @($report.tests | ForEach-Object { $_.fullTestPath }) } else { @() }
}

$result | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath $summaryPath -Encoding UTF8
$result | ConvertTo-Json -Depth 5

if ($buildExitCode -ne 0 -or $automationExitCode -ne 0 -or -not $result.CompiledEditorModuleExists -or -not $result.AutomationSucceeded) {
    exit 1
}
