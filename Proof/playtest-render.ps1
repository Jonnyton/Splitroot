param(
    [string]$ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path,
    [string]$EngineRoot = 'C:\Program Files\Epic Games\UE_5.7',
    [int]$ResX = 1280,
    [int]$ResY = 720,
    [string]$Map = '/Game/FirstPerson/Lvl_FirstPerson',
    [string[]]$ExtraArgs = @()
)

# Playtest with rendering. Launches the canary with real graphics (NOT NullRHI),
# runs the first-60-seconds proof arc, and instructs the runtime to capture a
# PNG screenshot before quitting. Outputs a list of PNG paths the caller
# (an LLM session) can Read as images to "see" the build without a human.
#
# Long-term: iterate this script + the unreal-canary-playtest skill so future
# Rook sessions learn what a good frame looks like and what's broken.

$ErrorActionPreference = 'Stop'

$projectFile = Join-Path $ProjectRoot 'ArchonFactoryCanary.uproject'
$editorCmd = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$proofDir = Join-Path $ProjectRoot 'Saved\Proof'
$screenshotDir = Join-Path $ProjectRoot 'Saved\Screenshots\Windows'
$autoScreenshotPath = Join-Path $ProjectRoot 'Saved\AutoScreenshot.png'
$logPath = Join-Path $proofDir 'last-playtest-render.log'
$jsonPath = Join-Path $proofDir 'last-playtest-render.json'

New-Item -ItemType Directory -Force -Path $proofDir | Out-Null

# Wipe stale screenshots so the post-run list reflects only this playtest.
if (Test-Path -LiteralPath $screenshotDir -PathType Container) {
    Get-ChildItem -LiteralPath $screenshotDir -File -Filter '*.png' | Remove-Item -Force -ErrorAction SilentlyContinue
}
if (Test-Path -LiteralPath $autoScreenshotPath -PathType Leaf) {
    Remove-Item -LiteralPath $autoScreenshotPath -Force -ErrorAction SilentlyContinue
}

# Notes on flags:
#   no -NullRHI            -> real rendering (needed for HighResShot)
#   -RenderOffScreen       -> rendering without a foreground window (works on
#                             logged-in Windows session; on a headless box this
#                             still requires a graphics device).
#   -ResX/-ResY            -> backing-buffer size; HighResShot inherits this.
#   -ArchonRunFirst60...   -> spawns blockout actors + runs the proof arc.
#   -ArchonPlayTestScreenshots -> tells the runtime to take a screenshot 2.5s
#                                  after the arc completes, then quit 2.5s
#                                  later. Quit is initiated from C++ so we do
#                                  NOT pass -ExecCmds=quit here.
$argsList = @(
    $projectFile,
    $Map,
    '-game',
    '-RenderOffScreen',
    "-ResX=$ResX",
    "-ResY=$ResY",
    '-NoSound',
    '-NoSplash',
    '-unattended',
    '-nop4',
    '-stdout',
    '-FullStdOutLogOutput',
    '-ArchonRunFirst60SecondsProof',
    '-ArchonPlayTestScreenshots'
) + $ExtraArgs

$output = & $editorCmd @argsList 2>&1
$exitCode = $LASTEXITCODE
$text = ($output | ForEach-Object { $_.ToString() }) -join [Environment]::NewLine
$text | Set-Content -LiteralPath $logPath -Encoding UTF8

$screenshots = @()
$screenshotItems = @()
if (Test-Path -LiteralPath $autoScreenshotPath -PathType Leaf) {
    $screenshotItems += Get-Item -LiteralPath $autoScreenshotPath
}
if (Test-Path -LiteralPath $screenshotDir -PathType Container) {
    $screenshotItems += Get-ChildItem -LiteralPath $screenshotDir -File -Filter '*.png'
}
$screenshots = @(
    $screenshotItems |
        Sort-Object LastWriteTime |
        ForEach-Object { $_.FullName }
)

$result = [pscustomobject]@{
    ProjectRoot = $ProjectRoot
    EngineRoot = $EngineRoot
    Map = $Map
    Resolution = "${ResX}x${ResY}"
    ExtraArgs = $ExtraArgs
    ExitCode = $exitCode
    PlayTestScreenshotScheduled = $text -match 'ArchonFactoryCanary: PlayTestScreenshotScheduled'
    PlayTestCameraFramed        = $text -match 'ArchonFactoryCanary: PlayTestCameraFramed ready=true'
    PlayTestScreenshotTaken     = $text -match 'ArchonFactoryCanary: PlayTestScreenshotTaken'
    PlayTestQuitIssued          = $text -match 'ArchonFactoryCanary: PlayTestQuitIssued'
    First60ArcCompleted         = $text -match 'ArchonFactoryCanary: First60Arc completed=true'
    ScreenshotCount = $screenshots.Count
    Screenshots = $screenshots
    LogPath = $logPath
    ScreenshotDir = $screenshotDir
    AutoScreenshotPath = $autoScreenshotPath
}

$result | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath $jsonPath -Encoding UTF8
$result | ConvertTo-Json -Depth 4

if (
    $exitCode -ne 0 -or
    -not $result.First60ArcCompleted -or
    -not $result.PlayTestCameraFramed -or
    $result.ScreenshotCount -lt 1
) {
    exit 1
}
