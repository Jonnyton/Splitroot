param(
    [ValidateSet('current','wide_sites','lens_reseat')]
    [string]$Variant = 'lens_reseat',
    [ValidateRange(0.0, 1.0)]
    [double]$DebugOpacity = 0.35,
    [int]$Width = 2400,
    [int]$Height = 1600,
    [string]$NamePrefix = 'splitroot-standard-map',
    [string]$ProjectRoot = ''
)

$ErrorActionPreference = 'Stop'
if (-not $ProjectRoot) {
    $ProjectRoot = (Resolve-Path (Join-Path (Split-Path -Parent $MyInvocation.MyCommand.Path) '..\..\..')).Path
}

$scriptPath = Join-Path $ProjectRoot 'games\splitroot\Proof\render-standard-map-view.py'
if (-not (Test-Path -LiteralPath $scriptPath)) {
    throw "Missing standard map renderer: $scriptPath"
}

& python $scriptPath `
    --variant $Variant `
    --debug-opacity $DebugOpacity `
    --width $Width `
    --height $Height `
    --name-prefix $NamePrefix

if ($LASTEXITCODE -ne 0) {
    throw "Standard map renderer failed with exit code $LASTEXITCODE"
}
