param(
    [string]$ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..\..')).Path,
    [string]$EngineRoot = 'C:\Program Files\Epic Games\UE_5.7',
    [int]$ServerStartupSeconds = 45,
    [int]$ClientSessionSeconds = 60,
    # Dedicated port: a real play session on the default 7777 (e.g. Jonathan
    # hosting from the desktop shortcut) must never receive smoke traffic.
    [int]$SmokePort = 7878
)

$ErrorActionPreference = 'Stop'

$projectFile = Join-Path $ProjectRoot 'ArchonFactoryCanary.uproject'
$editorCmd = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$proofDir = Join-Path $ProjectRoot 'Saved\Proof'
$serverLogPath = Join-Path $proofDir 'last-two-player-server.log'
$clientLogPath = Join-Path $proofDir 'last-two-player-client.log'
$jsonPath = Join-Path $proofDir 'last-two-player-join-smoke.json'

New-Item -ItemType Directory -Force -Path $proofDir | Out-Null

# Two real processes on loopback: a listen server hosting the valley match
# (long warmup so the match outlives the handshake) and a client joining by
# IP. The client world receives the replicated ValleyNetBeacon and rebuilds
# the cosmetic valley locally; the match GameMode spawns BOTH players as FPS
# characters server-side. Processes write separate log files via -abslog and
# are stopped by this script after the observation window.
$serverUrl = '/Engine/Maps/Entry?ArchonSplitrootValley?ArchonMatchLoop?game=/Script/ArchonFactoryCanary.ArchonMatchGameMode?ArchonMapId=splitroot_valley?listen'
$serverArgs = @(
    "`"$projectFile`"", $serverUrl,
    '-game', '-NullRHI', '-NoSound', '-NoSplash', '-unattended', '-nop4',
    "-port=$SmokePort",
    "-abslog=`"$serverLogPath`""
)
$clientArgs = @(
    "`"$projectFile`"", "127.0.0.1:$SmokePort",
    '-game', '-NullRHI', '-NoSound', '-NoSplash', '-unattended', '-nop4',
    '-ArchonClientFireProof', '-ArchonClientOrderProof',
    "-abslog=`"$clientLogPath`""
)

Remove-Item -LiteralPath $serverLogPath, $clientLogPath -Force -ErrorAction SilentlyContinue

$server = Start-Process -FilePath $editorCmd -ArgumentList $serverArgs -PassThru -WindowStyle Hidden
Start-Sleep -Seconds $ServerStartupSeconds
$client = Start-Process -FilePath $editorCmd -ArgumentList $clientArgs -PassThru -WindowStyle Hidden
Start-Sleep -Seconds $ClientSessionSeconds

foreach ($proc in @($client, $server)) {
    if ($proc -and -not $proc.HasExited) {
        Stop-Process -Id $proc.Id -Force -Confirm:$false -ErrorAction SilentlyContinue
    }
}
Start-Sleep -Seconds 5

$serverText = if (Test-Path -LiteralPath $serverLogPath) { Get-Content -LiteralPath $serverLogPath -Raw } else { '' }
$clientText = if (Test-Path -LiteralPath $clientLogPath) { Get-Content -LiteralPath $clientLogPath -Raw } else { '' }

$result = [pscustomobject]@{
    ProjectRoot = $ProjectRoot
    ServerUrl = $serverUrl

    ServerValleySpawned       = $serverText -match 'ArchonFactoryCanary: SplitrootValleySpawned .*cores=2 cosmeticOnly=false'
    ServerMatchLoopActive     = $serverText -match 'ArchonFactoryCanary: MatchLoopActive sites=3'
    ServerHostPlayerSpawned   = $serverText -match 'ArchonFactoryCanary: MatchPlayerSpawned playerIndex=0 pawn='
    ServerClientPlayerSpawned = $serverText -match 'ArchonFactoryCanary: MatchPlayerSpawned playerIndex=1 pawn='
    ServerSawRemoteJoin       = $serverText -match 'ArchonFactoryCanary: MatchPlayerJoined players=2 remote=true'

    ClientConnected           = $clientText -match 'Welcomed by server'
    ClientBeaconFired         = $clientText -match 'ArchonFactoryCanary: ValleyNetBeaconClientSpawn'
    ClientValleyRebuilt       = $clientText -match 'ArchonFactoryCanary: SplitrootValleySpawned .*cores=0 cosmeticOnly=true'

    # Cross-network combat: client trigger pull -> server RPC -> authoritative
    # fire -> projectile damages the Lenswright (team 1) core on the server.
    ClientFireRouted          = $clientText -match 'ArchonFactoryCanary: RuntimeWeaponInput routed=server_rpc firePressed=true'
    ClientCoreShotSent        = $clientText -match 'ArchonFactoryCanary: ClientFireProofCoreShot sent=true'
    ServerReceivedClientFire  = $serverText -match 'ArchonFactoryCanary: ServerWeaponFireReceived .*fired=true'
    ServerWeaponFired         = $serverText -match 'ArchonFactoryCanary: WeaponFired weapon='
    ServerCoreDamaged         = $serverText -match 'ArchonFactoryCanary: BaseCoreDamaged team=1 .*alive=true'

    # Cross-network RTS orders: client map-table order -> server RPC ->
    # server's interactor submits against the shared command surface.
    ClientOrderRouted         = $clientText -match 'ArchonFactoryCanary: RuntimeMapTableOrder routed=server_rpc order=0'
    ServerReceivedClientOrder = $serverText -match 'ArchonFactoryCanary: ServerRtsOrderReceived .*order=0 submitted=true'
    ServerOrderExecuted       = $serverText -match 'ArchonFactoryCanary: RuntimeMapTableOrder submitted=true order=0'

    # Match-state replication: the client receives the server's snapshot
    # (phase + supply + points) via the replicated match state actor.
    ClientSawMatchState       = $clientText -match 'ArchonFactoryCanary: MatchStateClientSnapshot phase='

    ServerLogPath = $serverLogPath
    ClientLogPath = $clientLogPath
}

$result | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath $jsonPath -Encoding UTF8
$result | ConvertTo-Json -Depth 4

if (
    -not $result.ServerValleySpawned -or
    -not $result.ServerMatchLoopActive -or
    -not $result.ServerHostPlayerSpawned -or
    -not $result.ServerClientPlayerSpawned -or
    -not $result.ServerSawRemoteJoin -or
    -not $result.ClientConnected -or
    -not $result.ClientBeaconFired -or
    -not $result.ClientValleyRebuilt -or
    -not $result.ClientFireRouted -or
    -not $result.ClientCoreShotSent -or
    -not $result.ServerReceivedClientFire -or
    -not $result.ServerWeaponFired -or
    -not $result.ServerCoreDamaged -or
    -not $result.ClientOrderRouted -or
    -not $result.ServerReceivedClientOrder -or
    -not $result.ServerOrderExecuted -or
    -not $result.ClientSawMatchState
) {
    exit 1
}
