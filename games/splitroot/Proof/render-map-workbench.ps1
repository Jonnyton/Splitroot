param(
    [ValidateSet('current', 'wide_sites', 'lens_reseat')]
    [string]$Variant = 'current',
    [switch]$All,
    [string]$ProjectRoot = ''
)

$ErrorActionPreference = 'Stop'
if (-not $ProjectRoot) {
    $ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..\..')).Path
}

$scriptPath = Join-Path $ProjectRoot 'games\splitroot\Proof\render-map-workbench.py'
$variants = if ($All) { @('current', 'wide_sites', 'lens_reseat') } else { @($Variant) }
foreach ($variantName in $variants) {
    python $scriptPath --variant $variantName
}
