param(
    [ValidateSet('current','wide_sites','lens_reseat')]
    [string]$Variant = 'lens_reseat',
    [ValidateRange(0.0, 1.0)]
    [double]$DebugOpacity = 0.35,
    [string]$NamePrefix = 'splitroot-map',
    [string]$ProjectRoot = ''
)

$ErrorActionPreference = 'Stop'
if (-not $ProjectRoot) {
    $ProjectRoot = (Resolve-Path (Join-Path (Split-Path -Parent $MyInvocation.MyCommand.Path) '..\..\..')).Path
}

$driver = Join-Path $ProjectRoot 'Proof\playtest-drive.ps1'
$composer = Join-Path $ProjectRoot 'games\splitroot\Proof\compose-map-view-overlays.py'
$outDir = Join-Path $ProjectRoot 'Saved\Proof\MapViews'
$shotName = "$NamePrefix-live-source.png"

if (-not (Test-Path -LiteralPath $driver)) {
    throw "Missing playtest driver: $driver"
}
if (-not (Test-Path -LiteralPath $composer)) {
    throw "Missing overlay composer: $composer"
}

function Invoke-QuietShot {
    $savedErrorActionPreference = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    try {
        $output = & powershell -ExecutionPolicy Bypass -File $driver -Action quietshot -Name $shotName 2>&1
        $exitCode = $LASTEXITCODE
    } catch {
        $output = @($_.Exception.Message)
        $exitCode = 1
    } finally {
        $ErrorActionPreference = $savedErrorActionPreference
    }
    return @{
        Output = $output
        ExitCode = $exitCode
    }
}

$quietResult = Invoke-QuietShot
$quietText = $quietResult.Output | Out-String
if ($quietResult.ExitCode -ne 0 -and ($quietText -match 'No playtest session|No Archon game window|Cannot find a process')) {
    $savedErrorActionPreference = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    try {
        $adoptOutput = & powershell -ExecutionPolicy Bypass -File $driver -Action adopt 2>&1
        $adoptExitCode = $LASTEXITCODE
    } catch {
        $adoptOutput = @($_.Exception.Message)
        $adoptExitCode = 1
    } finally {
        $ErrorActionPreference = $savedErrorActionPreference
    }
    $adoptOutput | ForEach-Object { Write-Output $_ }
    if ($adoptExitCode -ne 0) {
        throw "Could not adopt a running live game for quiet capture. Output: $($adoptOutput -join '; ')"
    }
	$quietResult = Invoke-QuietShot
}

if ($quietResult.ExitCode -ne 0) {
    throw "quietshot failed with exit code $($quietResult.ExitCode). Output: $($quietResult.Output -join '; ')"
}

$quietOutput = $quietResult.Output
$quietOutput | ForEach-Object { Write-Output $_ }

$actualPath = $null
foreach ($line in $quietOutput) {
    if ($line -match '^Screenshot=(.+)$') {
        $actualPath = $Matches[1]
    }
}

if (-not $actualPath -or -not (Test-Path -LiteralPath $actualPath)) {
    throw "quietshot did not produce a screenshot path. Output: $($quietOutput -join '; ')"
}

$composeOutput = & python $composer `
    --actual $actualPath `
    --variant $Variant `
    --debug-opacity $DebugOpacity `
    --out-dir $outDir `
    --name-prefix $NamePrefix
if ($LASTEXITCODE -ne 0) {
    throw "Overlay composer failed with exit code $LASTEXITCODE"
}

$composeOutput | ForEach-Object { Write-Output $_ }
