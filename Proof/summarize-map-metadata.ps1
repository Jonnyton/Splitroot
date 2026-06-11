param(
    [string]$ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path,
    [int]$RecentArchivedLogs = 5,
    [int]$MaxLinesPerLog = 60000,
    [string]$OutputPath = ''
)

$ErrorActionPreference = 'Stop'

$scriptPath = Join-Path $PSScriptRoot 'summarize_map_metadata.py'
$args = @(
    $scriptPath,
    '--project-root', $ProjectRoot,
    '--recent-archived-logs', ([string]$RecentArchivedLogs),
    '--max-lines-per-log', ([string]$MaxLinesPerLog)
)

if (-not [string]::IsNullOrWhiteSpace($OutputPath)) {
    $args += @('--output-path', $OutputPath)
}

python @args
exit $LASTEXITCODE
