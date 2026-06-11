param(
    [string]$ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..\..\..')).Path,
    [string]$StatePath = (Join-Path $PSScriptRoot 'state.json'),
    [string]$OutPath = (Join-Path $ProjectRoot 'Saved\RtsUiPreview\splitroot-rts-ui-preview.png'),
    [int]$Width = 1600,
    [int]$Height = 900
)

$ErrorActionPreference = 'Stop'

$bundledPython = Join-Path $env:USERPROFILE '.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe'
$pythonExe = if (Test-Path -LiteralPath $bundledPython -PathType Leaf) { $bundledPython } else { 'python' }
$scriptPath = Join-Path $PSScriptRoot 'render-rts-ui-preview.py'

& $pythonExe $scriptPath `
    --state $StatePath `
    --out $OutPath `
    --width $Width `
    --height $Height
