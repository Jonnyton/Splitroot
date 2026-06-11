param(
    [string]$ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..\..')).Path,
    [string]$EngineRoot = 'C:\Program Files\Epic Games\UE_5.7'
)

$ErrorActionPreference = 'Stop'

$projectFile = Join-Path $ProjectRoot 'ArchonFactoryCanary.uproject'
$editorCmd = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$proofDir = Join-Path $ProjectRoot 'Saved\Proof'
$logPath = Join-Path $proofDir 'last-front-end-smoke.log'
$jsonPath = Join-Path $proofDir 'last-front-end-smoke.json'

New-Item -ItemType Directory -Force -Path $proofDir | Out-Null

# Front-end proof: boot a menu world (?ArchonMainMenu URL option — never a
# command-line flag, it must NOT survive the travel), then drive the exact
# HostMap path the menu button calls (?ArchonMainMenuProofHost). The host
# travel carries ArchonMatchProofQuit so the hosted valley ends the process.
# Asserts: menu boots once, host picks the valley, the match map starts,
# the menu does NOT respawn on the hosted map.
$mapUrl = '/Engine/Maps/Entry?ArchonMainMenu?ArchonMainMenuProofHost'
$argsList = @(
    $projectFile,
    $mapUrl,
    '-game',
    '-NullRHI',
    '-NoSound',
    '-NoSplash',
    '-unattended',
    '-nop4',
    '-stdout',
    '-FullStdOutLogOutput',
    # Dedicated listen port: the proof host must never collide with a real
    # play session (or the playtest-drive harness) on the default 7777.
    '-port=7879'
)

$output = & $editorCmd @argsList 2>&1
$exitCode = $LASTEXITCODE
$text = ($output | ForEach-Object { $_.ToString() }) -join [Environment]::NewLine
$text | Set-Content -LiteralPath $logPath -Encoding UTF8

$result = [pscustomobject]@{
    ProjectRoot = $ProjectRoot
    EngineRoot = $EngineRoot
    MapUrl = $mapUrl
    ExitCode = $exitCode

    MenuActiveWithMaps    = $text -match 'ArchonFactoryCanary: MainMenuActive maps=2'
    MenuActivatedOnce     = ([regex]::Matches($text, 'ArchonFactoryCanary: MainMenuActive')).Count -eq 1
    # The invisible-mouse bug (Jonathan 2026-06-10): the menu world must show
    # the cursor AND skip the FPS input bridge that would hide it again.
    MenuCursorShown       = $text -match 'ArchonFactoryCanary: MainMenuCursorShown cursor=true inputMode=uionly'
    MenuWorldIsMenuOnly   = $text -match 'ArchonFactoryCanary: MainMenuWorldIsMenuOnly bridgeSkipped=true'
    # The menu panel must come OFF the viewport when the host travel tears
    # down the menu world (it otherwise stays painted over the match).
    MenuPanelRemoved      = $text -match 'ArchonFactoryCanary: MainMenuPanelRemoved onWorldTeardown=true'
    HostPickedValley      = $text -match 'ArchonFactoryCanary: MainMenuHost map=splitroot_valley url=/Engine/Maps/Entry\?ArchonSplitrootValley\?ArchonMatchLoop\?game=/Script/ArchonFactoryCanary\.ArchonMatchGameMode\?ArchonMapId=splitroot_valley\?ArchonMatchProofQuit\?listen'
    HostIsListenServer    = $text -match '\?listen'
    MatchPlayerSpawned    = $text -match 'ArchonFactoryCanary: MatchPlayerSpawned playerIndex=0 pawn='
    HostedValleySpawned   = $text -match 'ArchonFactoryCanary: SplitrootValleySpawned placements=\d+ blocks=\d+ stones=\d+ bases=\d+ cores=2'
    HostedMatchLoopActive = $text -match 'ArchonFactoryCanary: MatchLoopActive sites=3'
    HostedWorldQuitPath   = $text -match 'ArchonFactoryCanary: MatchProofQuitScheduled map=Entry'
    QuitCommandHonored    = $text -match 'UGameEngine::HandleExitCommand'
    LogPath               = $logPath
}

$result | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath $jsonPath -Encoding UTF8
$result | ConvertTo-Json -Depth 4

if (
    $exitCode -ne 0 -or
    -not $result.MenuActiveWithMaps -or
    -not $result.MenuActivatedOnce -or
    -not $result.MenuCursorShown -or
    -not $result.MenuWorldIsMenuOnly -or
    -not $result.MenuPanelRemoved -or
    -not $result.HostPickedValley -or
    -not $result.HostIsListenServer -or
    -not $result.MatchPlayerSpawned -or
    -not $result.HostedValleySpawned -or
    -not $result.HostedMatchLoopActive -or
    -not $result.HostedWorldQuitPath -or
    -not $result.QuitCommandHonored
) {
    exit 1
}
