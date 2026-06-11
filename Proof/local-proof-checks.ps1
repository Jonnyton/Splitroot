param(
    [string]$ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
)

$ErrorActionPreference = 'Stop'

function Test-File($RelativePath) {
    $path = Join-Path $ProjectRoot $RelativePath
    [pscustomobject]@{
        Path = $RelativePath
        Exists = Test-Path -LiteralPath $path -PathType Leaf
    }
}

function Test-Directory($RelativePath) {
    $path = Join-Path $ProjectRoot $RelativePath
    [pscustomobject]@{
        Path = $RelativePath
        Exists = Test-Path -LiteralPath $path -PathType Container
    }
}

function Normalize-PathString($Path) {
    if ([string]::IsNullOrWhiteSpace($Path)) {
        return ''
    }

    return [System.IO.Path]::GetFullPath($Path).TrimEnd('\')
}

function Get-DesktopShortcutInfo($ShortcutPath, $ExpectedTargetPath, $ExpectedWorkingDirectory) {
    $exists = Test-Path -LiteralPath $ShortcutPath -PathType Leaf
    if (-not $exists) {
        return [pscustomobject]@{
            Exists = $false
            TargetPath = $null
            Arguments = $null
            WorkingDirectory = $null
            IconLocation = $null
            Description = $null
            ExpectedTargetPath = $ExpectedTargetPath
            TargetMatchesCurrentBuildLauncher = $false
            WorkingDirectoryMatchesProjectRoot = $false
        }
    }

    $shell = New-Object -ComObject WScript.Shell
    $shortcut = $shell.CreateShortcut($ShortcutPath)
    $targetMatches = [string]::Equals(
        (Normalize-PathString $shortcut.TargetPath),
        (Normalize-PathString $ExpectedTargetPath),
        [System.StringComparison]::OrdinalIgnoreCase
    )
    $workingDirectoryMatches = [string]::Equals(
        (Normalize-PathString $shortcut.WorkingDirectory),
        (Normalize-PathString $ExpectedWorkingDirectory),
        [System.StringComparison]::OrdinalIgnoreCase
    )

    [pscustomobject]@{
        Exists = $true
        TargetPath = $shortcut.TargetPath
        Arguments = $shortcut.Arguments
        WorkingDirectory = $shortcut.WorkingDirectory
        IconLocation = $shortcut.IconLocation
        Description = $shortcut.Description
        ExpectedTargetPath = $ExpectedTargetPath
        TargetMatchesCurrentBuildLauncher = $targetMatches
        WorkingDirectoryMatchesProjectRoot = $workingDirectoryMatches
    }
}

$engineRoot = 'C:\Program Files\Epic Games\UE_5.7'
$editorExe = Join-Path $engineRoot 'Engine\Binaries\Win64\UnrealEditor.exe'
$desktopShortcut = 'C:\Users\Jonathan\Desktop\SPLITROOT Current Build.lnk'
$currentBuildLauncherRelative = 'Launchers\Play-CurrentBuild.cmd'
$currentBuildLauncher = Join-Path $ProjectRoot $currentBuildLauncherRelative
$desktopShortcutInfo = Get-DesktopShortcutInfo $desktopShortcut $currentBuildLauncher $ProjectRoot

$checks = @(
    (Test-Directory 'Config'),
    (Test-Directory 'Content'),
    (Test-Directory 'FactoryContracts'),
    (Test-Directory 'Launchers'),
    (Test-Directory '.agents\skills'),
    (Test-Directory '.claude\agents'),
    (Test-Directory '.claude\skills'),
    (Test-File 'ArchonFactoryCanary.uproject'),
    (Test-File 'AGENTS.md'),
    (Test-File 'CLAUDE.md'),
    (Test-File 'CODEX.md'),
    (Test-File '.mcp.json'),
    (Test-File 'Config\DefaultGame.ini'),
    (Test-File 'Config\DefaultEngine.ini'),
    (Test-File 'FactoryContracts\session_routes.json'),
    (Test-File 'FactoryContracts\entitlement_policy.json'),
    (Test-File 'FactoryContracts\archon_adapter_contract.md'),
    (Test-File 'games\splitroot\FactoryContracts\factions.json'),
    (Test-File 'Proof\unreal-map-smoke.ps1'),
    (Test-File 'Proof\build-and-test-policy.ps1'),
    (Test-File 'Proof\playtest-render.ps1'),
    (Test-File 'scripts\sync-skills.ps1'),
    (Test-File 'scripts\validate_skills.py'),
    (Test-File '.agents\skills\using-agent-skills\SKILL.md'),
    (Test-File '.agents\skills\unreal-archon-game-factory\SKILL.md'),
    (Test-File '.agents\skills\unreal-canary-playtest\SKILL.md'),
    (Test-File '.claude\skills\unreal-archon-game-factory\SKILL.md'),
    (Test-File '.claude\skills\unreal-canary-playtest\SKILL.md'),
    (Test-File '.claude\settings.json'),
    (Test-File '.claude\agents\developer.md'),
    (Test-File '.claude\agents\verifier.md'),
    (Test-File '.claude\agents\navigator.md'),
    (Test-File '.claude\agents\game-designer.md'),
    (Test-File 'Source\ArchonFactoryCanary.Target.cs'),
    (Test-File 'Source\ArchonFactoryCanaryEditor.Target.cs'),
    (Test-File 'Source\ArchonFactoryCanary\ArchonFactoryCanary.Build.cs'),
    (Test-File 'Source\ArchonFactoryCanary\Public\ArchonSessionTypes.h'),
    (Test-File 'Source\ArchonFactoryCanary\Public\ArchonSessionPolicyLibrary.h'),
    (Test-File 'Source\ArchonFactoryCanary\Private\ArchonSessionPolicyLibrary.cpp'),
    (Test-File 'Source\ArchonFactoryCanary\Public\ArchonMapTableTypes.h'),
    (Test-File 'Source\ArchonFactoryCanary\Public\ArchonMapTablePolicyLibrary.h'),
    (Test-File 'Source\ArchonFactoryCanary\Private\ArchonMapTablePolicyLibrary.cpp'),
    (Test-File 'Source\ArchonFactoryCanary\Public\ArchonMapTableSelectionPolicyLibrary.h'),
    (Test-File 'Source\ArchonFactoryCanary\Private\ArchonMapTableSelectionPolicyLibrary.cpp'),
    (Test-File 'Source\ArchonFactoryCanary\Public\ArchonMapTableWidget.h'),
    (Test-File 'Source\ArchonFactoryCanary\Private\ArchonMapTableWidget.cpp'),
    (Test-File 'Source\ArchonFactoryCanary\Public\ArchonTeamRtsTypes.h'),
    (Test-File 'Source\ArchonFactoryCanary\Public\ArchonTeamRtsPolicyLibrary.h'),
    (Test-File 'Source\ArchonFactoryCanary\Private\ArchonTeamRtsPolicyLibrary.cpp'),
    (Test-File 'Source\ArchonFactoryCanary\Public\ArchonTeamRtsStateComponent.h'),
    (Test-File 'Source\ArchonFactoryCanary\Private\ArchonTeamRtsStateComponent.cpp'),
    (Test-File 'Source\ArchonFactoryCanary\Public\ArchonTeamVisibilityTypes.h'),
    (Test-File 'Source\ArchonFactoryCanary\Public\ArchonTeamVisibilityPolicyLibrary.h'),
    (Test-File 'Source\ArchonFactoryCanary\Private\ArchonTeamVisibilityPolicyLibrary.cpp'),
    (Test-File 'Source\ArchonFactoryCanary\Public\ArchonTeamVisibilityStateComponent.h'),
    (Test-File 'Source\ArchonFactoryCanary\Private\ArchonTeamVisibilityStateComponent.cpp'),
    (Test-File 'Source\ArchonFactoryCanary\Public\ArchonMapTableActor.h'),
    (Test-File 'Source\ArchonFactoryCanary\Private\ArchonMapTableActor.cpp'),
    (Test-File 'Source\ArchonFactoryCanary\Public\ArchonCanaryWorldSubsystem.h'),
    (Test-File 'Source\ArchonFactoryCanary\Private\ArchonCanaryWorldSubsystem.cpp'),
    (Test-File 'Source\ArchonFactoryCanary\Public\ArchonMapTableInteractionTypes.h'),
    (Test-File 'Source\ArchonFactoryCanary\Public\ArchonMapTableInteractorComponent.h'),
    (Test-File 'Source\ArchonFactoryCanary\Private\ArchonMapTableInteractorComponent.cpp'),
    (Test-File 'Source\ArchonFactoryCanary\Public\ArchonFpsInputProfile.h'),
    (Test-File 'Source\ArchonFactoryCanary\Private\ArchonFpsInputProfile.cpp'),
    (Test-File 'Source\ArchonFactoryCanary\Public\ArchonFactionTypes.h'),
    (Test-File 'Source\ArchonFactoryCanary\Public\ArchonFactionMovementPolicyLibrary.h'),
    (Test-File 'Source\ArchonFactoryCanary\Private\ArchonFactionMovementPolicyLibrary.cpp'),
    (Test-File 'Source\ArchonFactoryCanary\Public\ArchonFactionMovementComponent.h'),
    (Test-File 'Source\ArchonFactoryCanary\Private\ArchonFactionMovementComponent.cpp'),
    (Test-File 'Source\ArchonFactoryCanary\Public\ArchonCombatTypes.h'),
    (Test-File 'Source\ArchonFactoryCanary\Public\ArchonCombatPolicyLibrary.h'),
    (Test-File 'Source\ArchonFactoryCanary\Private\ArchonCombatPolicyLibrary.cpp'),
    (Test-File 'Source\ArchonFactoryCanary\Public\ArchonCombatHealthComponent.h'),
    (Test-File 'Source\ArchonFactoryCanary\Private\ArchonCombatHealthComponent.cpp'),
    (Test-File 'Source\ArchonFactoryCanary\Public\ArchonAiCombatTypes.h'),
    (Test-File 'Source\ArchonFactoryCanary\Public\ArchonAiCombatPolicyLibrary.h'),
    (Test-File 'Source\ArchonFactoryCanary\Private\ArchonAiCombatPolicyLibrary.cpp'),
    (Test-File 'Source\ArchonFactoryCanary\Public\ArchonAiCombatBehaviorComponent.h'),
    (Test-File 'Source\ArchonFactoryCanary\Private\ArchonAiCombatBehaviorComponent.cpp'),
    (Test-File 'Source\ArchonFactoryCanary\Public\ArchonWeaponTypes.h'),
    (Test-File 'Source\ArchonFactoryCanary\Public\ArchonWeaponPolicyLibrary.h'),
    (Test-File 'Source\ArchonFactoryCanary\Private\ArchonWeaponPolicyLibrary.cpp'),
    (Test-File 'Source\ArchonFactoryCanary\Public\ArchonRangedWeaponComponent.h'),
    (Test-File 'Source\ArchonFactoryCanary\Private\ArchonRangedWeaponComponent.cpp'),
    (Test-File 'Source\ArchonFactoryCanary\Public\ArchonVerdantThornsproutBow.h'),
    (Test-File 'Source\ArchonFactoryCanary\Private\ArchonVerdantThornsproutBow.cpp'),
    (Test-File 'Source\ArchonFactoryCanary\Public\ArchonArrowProjectile.h'),
    (Test-File 'Source\ArchonFactoryCanary\Private\ArchonArrowProjectile.cpp'),
    (Test-File 'Source\ArchonFactoryCanary\Public\ArchonPressureBoltProjectile.h'),
    (Test-File 'Source\ArchonFactoryCanary\Private\ArchonPressureBoltProjectile.cpp'),
    (Test-File 'Source\ArchonFactoryCanary\Public\ArchonLenswrightPressureBoltCrossbow.h'),
    (Test-File 'Source\ArchonFactoryCanary\Private\ArchonLenswrightPressureBoltCrossbow.cpp'),
    (Test-File 'Source\ArchonFactoryCanary\Public\ArchonLenswrightBracewrightActor.h'),
    (Test-File 'Source\ArchonFactoryCanary\Private\ArchonLenswrightBracewrightActor.cpp'),
    (Test-File 'Source\ArchonFactoryCanary\Public\ArchonLenswrightSundialOpticActor.h'),
    (Test-File 'Source\ArchonFactoryCanary\Private\ArchonLenswrightSundialOpticActor.cpp'),
    (Test-File 'Source\ArchonFactoryCanary\Public\ArchonCanaryFpsCharacter.h'),
    (Test-File 'Source\ArchonFactoryCanary\Private\ArchonCanaryFpsCharacter.cpp'),
    (Test-File 'Source\ArchonFactoryCanary\Public\ArchonCanaryRtsSquadActor.h'),
    (Test-File 'Source\ArchonFactoryCanary\Private\ArchonCanaryRtsSquadActor.cpp'),
    (Test-File 'Source\ArchonFactoryCanary\Public\ArchonPlayerInputBridgeComponent.h'),
    (Test-File 'Source\ArchonFactoryCanary\Private\ArchonPlayerInputBridgeComponent.cpp'),
    (Test-File 'Source\ArchonFactoryCanary\Private\Tests\ArchonSessionPolicyTests.cpp'),
    (Test-File 'Source\ArchonFactoryCanary\Private\Tests\ArchonMapTablePolicyTests.cpp'),
    (Test-File 'Source\ArchonFactoryCanary\Private\Tests\ArchonMapTableSelectionPolicyTests.cpp'),
    (Test-File 'Source\ArchonFactoryCanary\Private\Tests\ArchonMapTableWidgetTests.cpp'),
    (Test-File 'Source\ArchonFactoryCanary\Private\Tests\ArchonTeamRtsPolicyTests.cpp'),
    (Test-File 'Source\ArchonFactoryCanary\Private\Tests\ArchonTeamVisibilityPolicyTests.cpp'),
    (Test-File 'Source\ArchonFactoryCanary\Private\Tests\ArchonTeamVisibilityStateComponentTests.cpp'),
    (Test-File 'Source\ArchonFactoryCanary\Private\Tests\ArchonMapTableActorTests.cpp'),
    (Test-File 'Source\ArchonFactoryCanary\Private\Tests\ArchonBlockoutActorTests.cpp'),
    (Test-File 'Source\ArchonFactoryCanary\Private\Tests\ArchonCombatPolicyTests.cpp'),
    (Test-File 'Source\ArchonFactoryCanary\Private\Tests\ArchonCombatHealthComponentTests.cpp'),
    (Test-File 'Source\ArchonFactoryCanary\Private\Tests\ArchonAiCombatPolicyTests.cpp'),
    (Test-File 'Source\ArchonFactoryCanary\Private\Tests\ArchonLenswrightUnitsTests.cpp'),
    (Test-File 'Source\ArchonFactoryCanary\Private\Tests\ArchonWeaponPolicyTests.cpp'),
    (Test-File 'Source\ArchonFactoryCanary\Private\Tests\ArchonRangedWeaponComponentTests.cpp'),
    (Test-File 'Source\ArchonFactoryCanary\Private\Tests\ArchonMapTableInteractionTests.cpp'),
    (Test-File 'Source\ArchonFactoryCanary\Private\Tests\ArchonFactionMovementPolicyTests.cpp'),
    (Test-File 'Source\ArchonFactoryCanary\Private\Tests\ArchonFpsInputProfileTests.cpp'),
    (Test-File 'Source\ArchonFactoryCanary\Private\Tests\ArchonRuntimeInputBridgeTests.cpp'),
    (Test-File 'Source\ArchonFactoryCanary\Private\Tests\ArchonCanaryRtsSquadTests.cpp'),
    (Test-File $currentBuildLauncherRelative),
    (Test-File 'games\splitroot\Launchers\Play-ArchonFactoryCanary.cmd'),
    (Test-File 'games\splitroot\Launchers\Open-ArchonFactoryCanary.cmd'),
    (Test-File 'games\splitroot\Proof\first-60-seconds-smoke.ps1'),
    (Test-File 'games\splitroot\Proof\second-60-seconds-smoke.ps1'),
    (Test-File 'games\splitroot\Proof\match-loop-smoke.ps1'),
    (Test-File 'games\splitroot\Proof\map-rotation-smoke.ps1'),
    (Test-File 'games\splitroot\Proof\front-end-smoke.ps1'),
    (Test-File 'games\splitroot\Proof\two-player-join-smoke.ps1'),
    (Test-File 'games\splitroot\Proof\bot-match-smoke.ps1'),
    (Test-File 'games\splitroot\FactoryContracts\map_registry.json'),
    (Test-File 'games\splitroot\README.md'),
    (Test-File 'games\splitroot\FactoryContracts\faction_palette.json'),
    (Test-File 'games\splitroot\FactoryContracts\faction_audio_palette.json'),
    (Test-File 'games\splitroot\FactoryContracts\faction_silhouette.json'),
    (Test-File 'games\splitroot\FactoryContracts\strategic_audio_events.json'),
    (Test-File 'games\splitroot\FactoryContracts\asset_manifest.json'),
    (Test-File 'games\splitroot\FactoryContracts\asset_manifest.md')
)

$proofClaimState = @{
    ClaimsEditorUiLaunch = $false
    ClaimsHeadlessMapSmoke = $false
    ClaimsCompiledEditorModule = $false
    ClaimsPolicyAutomationTests = $false
    ClaimsPcFpsInputProfile = $false
    ClaimsRuntimeInputBridge = $false
    ClaimsRuntimeRtsSquad = $false
    ClaimsRuntimeRtsBridgeProofSequence = $false
    ClaimsTeamVisibilityPolicy = $false
    ClaimsRuntimeTeamVisibilityState = $false
    ClaimsMapTableSelectionPolicy = $false
    ClaimsMapTableWidgetContract = $false
    ClaimsRuntimeMapTableWidgetMounted = $false
    ClaimsVerdantRootVaultPolicy = $false
    ClaimsRuntimeRootVaultInputBridge = $false
    ClaimsRenderedPlaytestCapture = $false
    ClaimsBlockoutDebugColors = $false
    ClaimsCombatFundamentals = $false
    ClaimsRangedWeaponFoundation = $false
    ClaimsRuntimeVerdantBowInputBridge = $false
    ClaimsLenswrightAiCombat = $false
    ClaimsRuntimeLenswrightCombatProof = $false
    ClaimsFirst60SecondsSmoke = $false
    ClaimsSecond60SecondsSmoke = $false
    ClaimsCommandWhileYouWaitDeadOrder = $false
    ClaimsMatchLoopSmoke = $false
    ClaimsMapRotationSmoke = $false
    ClaimsFrontEndHostSmoke = $false
    ClaimsTwoPlayerJoinSmoke = $false
    ClaimsBotMatchSmoke = $false
    ClaimsDesktopCurrentBuildShortcut = [bool](
        $desktopShortcutInfo.Exists -and
        $desktopShortcutInfo.TargetMatchesCurrentBuildLauncher -and
        $desktopShortcutInfo.WorkingDirectoryMatchesProjectRoot
    )
    ClaimsSteamIntegration = $false
    ClaimsPackagedGame = $false
    ClaimsAaaOrSellReady = $false
}

$lastSmokeResultPath = Join-Path $ProjectRoot 'Saved\Proof\last-unreal-map-smoke.json'
$lastSmokeResult = $null
if (Test-Path -LiteralPath $lastSmokeResultPath -PathType Leaf) {
    $lastSmokeResult = Get-Content -LiteralPath $lastSmokeResultPath -Raw | ConvertFrom-Json
    $proofClaimState.ClaimsHeadlessMapSmoke = [bool](
        $lastSmokeResult.ExitCode -eq 0 -and
        $lastSmokeResult.StartedArchonFactoryCanary -and
        $lastSmokeResult.GameStarted -and
        $lastSmokeResult.MapLoadStarted -and
        $lastSmokeResult.MapLoadCompleted -and
        $lastSmokeResult.FirstPersonGameModeLoaded -and
        $lastSmokeResult.RuntimeMapTableSpawned -and
        $lastSmokeResult.RuntimeRtsSquadSpawned -and
        $lastSmokeResult.RuntimeTeamVisibilityInitialized -and
        $lastSmokeResult.RuntimeTeamVisibilityHasLitAndFog -and
        $lastSmokeResult.RuntimeFpsPawnPossessed -and
        $lastSmokeResult.RuntimeInputBridgeInstalled -and
        $lastSmokeResult.RuntimeMapTableWidgetOpened -and
        $lastSmokeResult.RuntimeMapTableWidgetSelected -and
        $lastSmokeResult.RuntimeMapTableWidgetOrderSubmitted -and
        $lastSmokeResult.RuntimeMapTableWidgetProofSequenceRan -and
        $lastSmokeResult.RuntimeMapTableWidgetClosed -and
        $lastSmokeResult.RuntimeRtsProofSequenceRan -and
        $lastSmokeResult.RuntimeRtsMoveOrderSubmitted -and
        $lastSmokeResult.RuntimeRtsAttackOrderSubmitted -and
        $lastSmokeResult.RuntimeRtsSquadAcceptedMove -and
        $lastSmokeResult.RuntimeRtsSquadAcceptedAttack -and
        $lastSmokeResult.RuntimeMapTableClosed -and
        $lastSmokeResult.RuntimeWeaponFireRouted -and
        $lastSmokeResult.RuntimeWeaponReloadRouted -and
        $lastSmokeResult.RuntimeWeaponProofSequenceRan -and
        $lastSmokeResult.RuntimeWeaponProofRequested -and
        $lastSmokeResult.RuntimeLenswrightBracewrightSpawned -and
        $lastSmokeResult.RuntimeLenswrightSundialOpticSpawned -and
        $lastSmokeResult.RuntimeLenswrightTargetDamaged -and
        $lastSmokeResult.RuntimeLenswrightAiDecisionShouldFire -and
        $lastSmokeResult.RuntimeLenswrightAiFireRouted -and
        $lastSmokeResult.RuntimeLenswrightAiDamagedPlayer -and
        $lastSmokeResult.RuntimeCombatProofSequenceRan -and
        $lastSmokeResult.RuntimeCombatProofRequested -and
        $lastSmokeResult.PauseMenuOpened -and
        $lastSmokeResult.PauseMenuLookScaleChanged -and
        $lastSmokeResult.PauseMenuClosed -and
        $lastSmokeResult.PauseMenuProofCompleted -and
        $lastSmokeResult.MapTableOverlayShown -and
        $lastSmokeResult.MapTableOverlayHidden -and
        $lastSmokeResult.MapTableBodyLocked -and
        $lastSmokeResult.MapTableBodyReleased -and
        $lastSmokeResult.InteractPromptShown -and
        $lastSmokeResult.InteractOpenedTable -and
        $lastSmokeResult.CameraToggledThird -and
        $lastSmokeResult.InteractProofCompleted -and
        $lastSmokeResult.QuitCommandHonored
    )
}

$lastFirst60ResultPath = Join-Path $ProjectRoot 'Saved\Proof\last-first-60-seconds-smoke.json'
$lastFirst60Result = $null
if (Test-Path -LiteralPath $lastFirst60ResultPath -PathType Leaf) {
    $lastFirst60Result = Get-Content -LiteralPath $lastFirst60ResultPath -Raw | ConvertFrom-Json
    $proofClaimState.ClaimsFirst60SecondsSmoke = [bool](
        $lastFirst60Result.ExitCode -eq 0 -and
        $lastFirst60Result.StartedArchonFactoryCanary -and
        $lastFirst60Result.GameStarted -and
        $lastFirst60Result.MapLoadCompleted -and
        $lastFirst60Result.BlockoutVerdantOutpostPresent -and
        $lastFirst60Result.BlockoutSplitrootTreePresent -and
        $lastFirst60Result.BlockoutLenswrightGhostPresent -and
        $lastFirst60Result.BlockoutCoverStoneCount -and
        $lastFirst60Result.First60VisibilityConfigured -and
        $lastFirst60Result.First60WidgetOpened -and
        $lastFirst60Result.First60WidgetSelectedCanarySquad -and
        $lastFirst60Result.First60WidgetIssuedMoveOrder -and
        $lastFirst60Result.First60WidgetClosed -and
        $lastFirst60Result.First60SquadAcceptedOrder -and
        $lastFirst60Result.First60SquadTransitionedToMoving -and
        $lastFirst60Result.First60RootVaultLaunch1 -and
        $lastFirst60Result.First60RootVaultLaunch2 -and
        $lastFirst60Result.First60RootVaultLaunch3 -and
        $lastFirst60Result.First60RootVaultCooldownEnforced -and
        $lastFirst60Result.First60RootVaultInputBridgeRouted -and
        $lastFirst60Result.First60ArcCompleted -and
        $lastFirst60Result.QuitCommandHonored
    )
}

$lastSecond60ResultPath = Join-Path $ProjectRoot 'Saved\Proof\last-second-60-seconds-smoke.json'
$lastSecond60Result = $null
if (Test-Path -LiteralPath $lastSecond60ResultPath -PathType Leaf) {
    $lastSecond60Result = Get-Content -LiteralPath $lastSecond60ResultPath -Raw | ConvertFrom-Json
    $proofClaimState.ClaimsSecond60SecondsSmoke = [bool](
        $lastSecond60Result.ExitCode -eq 0 -and
        $lastSecond60Result.First60ArcCompleted -and
        $lastSecond60Result.Second60EnemiesSpawned -and
        $lastSecond60Result.Second60PlayerWeaponReady -and
        $lastSecond60Result.Second60PlayerFiredAtEnemy -and
        $lastSecond60Result.Second60BracewrightFiredAtPlayer -and
        $lastSecond60Result.Second60PlayerTookDamage -and
        $lastSecond60Result.Second60PlayerDied -and
        $lastSecond60Result.Second60ObserverPawnPossessed -and
        $lastSecond60Result.Second60CommandWhileWaitOpened -and
        $lastSecond60Result.Second60CommandWhileWaitSubmitted -and
        $lastSecond60Result.Second60PlayerRespawned -and
        $lastSecond60Result.Second60DeadStateOrderSurvived -and
        $lastSecond60Result.Second60ArcCompleted -and
        $lastSecond60Result.QuitCommandHonored
    )
    $proofClaimState.ClaimsCommandWhileYouWaitDeadOrder = [bool](
        $proofClaimState.ClaimsSecond60SecondsSmoke -and
        $lastSecond60Result.Second60CommandWhileWaitSubmitted -and
        $lastSecond60Result.Second60DeadStateOrderSurvived
    )
}

$lastMapRotationResultPath = Join-Path $ProjectRoot 'Saved\Proof\last-map-rotation-smoke.json'
$lastMapRotationResult = $null
if (Test-Path -LiteralPath $lastMapRotationResultPath -PathType Leaf) {
    $lastMapRotationResult = Get-Content -LiteralPath $lastMapRotationResultPath -Raw | ConvertFrom-Json
    $proofClaimState.ClaimsMapRotationSmoke = [bool](
        $lastMapRotationResult.ExitCode -eq 0 -and
        $lastMapRotationResult.ValleySpawnedViaUrlOptions -and
        $lastMapRotationResult.MatchEndedCoreDestroyed -and
        $lastMapRotationResult.TravelingToCanaryRange -and
        $lastMapRotationResult.SecondWorldBooted -and
        $lastMapRotationResult.SecondWorldMapTableSpawned -and
        $lastMapRotationResult.QuitCommandHonored
    )
}

$lastFrontEndResultPath = Join-Path $ProjectRoot 'Saved\Proof\last-front-end-smoke.json'
$lastFrontEndResult = $null
if (Test-Path -LiteralPath $lastFrontEndResultPath -PathType Leaf) {
    $lastFrontEndResult = Get-Content -LiteralPath $lastFrontEndResultPath -Raw | ConvertFrom-Json
    $proofClaimState.ClaimsFrontEndHostSmoke = [bool](
        $lastFrontEndResult.ExitCode -eq 0 -and
        $lastFrontEndResult.MenuActiveWithMaps -and
        $lastFrontEndResult.MenuActivatedOnce -and
        $lastFrontEndResult.MenuCursorShown -and
        $lastFrontEndResult.MenuWorldIsMenuOnly -and
        $lastFrontEndResult.MenuPanelRemoved -and
        $lastFrontEndResult.HostPickedValley -and
        $lastFrontEndResult.HostedValleySpawned -and
        $lastFrontEndResult.HostedMatchLoopActive -and
        $lastFrontEndResult.QuitCommandHonored
    )
}

$lastTwoPlayerResultPath = Join-Path $ProjectRoot 'Saved\Proof\last-two-player-join-smoke.json'
$lastTwoPlayerResult = $null
if (Test-Path -LiteralPath $lastTwoPlayerResultPath -PathType Leaf) {
    $lastTwoPlayerResult = Get-Content -LiteralPath $lastTwoPlayerResultPath -Raw | ConvertFrom-Json
    $proofClaimState.ClaimsTwoPlayerJoinSmoke = [bool](
        $lastTwoPlayerResult.ServerValleySpawned -and
        $lastTwoPlayerResult.ServerMatchLoopActive -and
        $lastTwoPlayerResult.ServerHostPlayerSpawned -and
        $lastTwoPlayerResult.ServerClientPlayerSpawned -and
        $lastTwoPlayerResult.ServerSawRemoteJoin -and
        $lastTwoPlayerResult.ClientConnected -and
        $lastTwoPlayerResult.ClientBeaconFired -and
        $lastTwoPlayerResult.ClientValleyRebuilt -and
        $lastTwoPlayerResult.ClientFireRouted -and
        $lastTwoPlayerResult.ClientCoreShotSent -and
        $lastTwoPlayerResult.ServerReceivedClientFire -and
        $lastTwoPlayerResult.ServerWeaponFired -and
        $lastTwoPlayerResult.ServerCoreDamaged -and
        $lastTwoPlayerResult.ClientOrderRouted -and
        $lastTwoPlayerResult.ServerReceivedClientOrder -and
        $lastTwoPlayerResult.ServerOrderExecuted -and
        $lastTwoPlayerResult.ClientSawMatchState
    )
}

$lastBotMatchResultPath = Join-Path $ProjectRoot 'Saved\Proof\last-bot-match-smoke.json'
$lastBotMatchResult = $null
if (Test-Path -LiteralPath $lastBotMatchResultPath -PathType Leaf) {
    $lastBotMatchResult = Get-Content -LiteralPath $lastBotMatchResultPath -Raw | ConvertFrom-Json
    $proofClaimState.ClaimsBotMatchSmoke = [bool](
        $lastBotMatchResult.BotMatchSpawned -and
        $lastBotMatchResult.MatchLoopActive -and
        $lastBotMatchResult.SpectatorStart -and
        $lastBotMatchResult.BotWaypointReached -and
        $lastBotMatchResult.BotsEngaged -and
        $lastBotMatchResult.BotsFired -and
        $lastBotMatchResult.BotRespawned -and
        $lastBotMatchResult.CoreDamaged -and
        $lastBotMatchResult.CoreAttackReached -and
        $lastBotMatchResult.BothTeamsFought
    )
}

$lastMatchLoopResultPath = Join-Path $ProjectRoot 'Saved\Proof\last-match-loop-smoke.json'
$lastMatchLoopResult = $null
if (Test-Path -LiteralPath $lastMatchLoopResultPath -PathType Leaf) {
    $lastMatchLoopResult = Get-Content -LiteralPath $lastMatchLoopResultPath -Raw | ConvertFrom-Json
    $proofClaimState.ClaimsMatchLoopSmoke = [bool](
        $lastMatchLoopResult.ExitCode -eq 0 -and
        $lastMatchLoopResult.ValleySpawnedWithCores -and
        $lastMatchLoopResult.MatchLoopActive -and
        $lastMatchLoopResult.MatchWentLive -and
        $lastMatchLoopResult.ProofPlayerPlaced -and
        $lastMatchLoopResult.CentralSiteCaptured -and
        $lastMatchLoopResult.SupplyTickedWithSite -and
        $lastMatchLoopResult.ProofCoreKillIssued -and
        $lastMatchLoopResult.MatchEndedCoreDestroyed -and
        $lastMatchLoopResult.ScoreboardPhaseReached -and
        $lastMatchLoopResult.TravelPhaseReached -and
        $lastMatchLoopResult.TravelRequested -and
        $lastMatchLoopResult.QuitCommandHonored
    )
}

$lastPlaytestRenderPath = Join-Path $ProjectRoot 'Saved\Proof\last-playtest-render.json'
$lastPlaytestRender = $null
if (Test-Path -LiteralPath $lastPlaytestRenderPath -PathType Leaf) {
    $lastPlaytestRender = Get-Content -LiteralPath $lastPlaytestRenderPath -Raw | ConvertFrom-Json
    $playtestScreenshotPaths = @($lastPlaytestRender.Screenshots)
    $missingPlaytestScreenshots = @(
        $playtestScreenshotPaths |
            Where-Object { -not (Test-Path -LiteralPath $_ -PathType Leaf) }
    )
    $proofClaimState.ClaimsRenderedPlaytestCapture = [bool](
        $lastPlaytestRender.ExitCode -eq 0 -and
        $lastPlaytestRender.First60ArcCompleted -and
        $lastPlaytestRender.PlayTestScreenshotScheduled -and
        $lastPlaytestRender.PlayTestCameraFramed -and
        $lastPlaytestRender.PlayTestScreenshotTaken -and
        $lastPlaytestRender.PlayTestQuitIssued -and
        $lastPlaytestRender.ScreenshotCount -ge 1 -and
        $missingPlaytestScreenshots.Count -eq 0
    )
}

$compiledEditorModulePath = Join-Path $ProjectRoot 'Binaries\Win64\UnrealEditor-ArchonFactoryCanary.dll'
$proofClaimState.ClaimsCompiledEditorModule = Test-Path -LiteralPath $compiledEditorModulePath -PathType Leaf

$policyBuildResultPath = Join-Path $ProjectRoot 'Saved\Proof\last-policy-build-and-test.json'
$policyBuildResult = $null
if (Test-Path -LiteralPath $policyBuildResultPath -PathType Leaf) {
    $policyBuildResult = Get-Content -LiteralPath $policyBuildResultPath -Raw | ConvertFrom-Json
    $proofClaimState.ClaimsPolicyAutomationTests = [bool](
        $policyBuildResult.BuildExitCode -eq 0 -and
        $policyBuildResult.AutomationExitCode -eq 0 -and
        $policyBuildResult.CompiledEditorModuleExists -and
        $policyBuildResult.AutomationSucceeded
    )
    $testPaths = @($policyBuildResult.TestPaths)
    $proofClaimState.ClaimsPcFpsInputProfile = [bool](
        $proofClaimState.ClaimsPolicyAutomationTests -and
        $testPaths -contains 'ArchonFactory.Input.WASDMovementDefaults' -and
        $testPaths -contains 'ArchonFactory.Input.MouseLookDefaults' -and
        $testPaths -contains 'ArchonFactory.Input.CombatAndUseDefaults' -and
        $testPaths -contains 'ArchonFactory.Input.MobilityAndMenuDefaults'
    )
    $proofClaimState.ClaimsRuntimeInputBridge = [bool](
        $proofClaimState.ClaimsPolicyAutomationTests -and
        $testPaths -contains 'ArchonFactory.RuntimeInputBridge.PreviewUpdatesVisibleMapTable' -and
        $testPaths -contains 'ArchonFactory.RuntimeInputBridge.SubmitOrderMutatesVisibleSequence' -and
        $testPaths -contains 'ArchonFactory.RuntimeInputBridge.SteamOnlineFreeRemainsPreviewOnly' -and
        $testPaths -contains 'ArchonFactory.RuntimeInputBridge.ProofSequenceOpensSubmitsAndCloses' -and
        $testPaths -contains 'ArchonFactory.RuntimeInputBridge.PreviewMountsMapTableWidget' -and
        $testPaths -contains 'ArchonFactory.RuntimeInputBridge.WidgetProofSequenceSelectsAndOrders' -and
        $testPaths -contains 'ArchonFactory.RuntimeInputBridge.WidgetOrderAtUsesProvidedTarget' -and
        $testPaths -contains 'ArchonFactory.RuntimeInputBridge.RootVaultInputSuppressesStandardJumpWhenReady' -and
        $testPaths -contains 'ArchonFactory.RuntimeInputBridge.JumpBelowRootVaultWindowStaysStandard' -and
        $testPaths -contains 'ArchonFactory.RuntimeInputBridge.WeaponInputFiresWhenMapTableClosed' -and
        $testPaths -contains 'ArchonFactory.RuntimeInputBridge.WeaponInputDoesNotFireWhenMapTableOpen' -and
        $testPaths -contains 'ArchonFactory.RuntimeInputBridge.WeaponInputReloadsAfterFireCycle' -and
        $testPaths -contains 'ArchonFactory.RuntimeInputBridge.WeaponProofSequenceFiresAndReloads' -and
        $testPaths -contains 'ArchonFactory.RuntimeInputBridge.CanaryFpsCharacterHasCamera'
    )
    $proofClaimState.ClaimsRuntimeRtsSquad = [bool](
        $proofClaimState.ClaimsPolicyAutomationTests -and
        $testPaths -contains 'ArchonFactory.RuntimeRtsSquad.MoveOrderSetsVisibleMovementState' -and
        $testPaths -contains 'ArchonFactory.RuntimeRtsSquad.AttackOrderSetsVisibleAttackState' -and
        $proofClaimState.ClaimsHeadlessMapSmoke
    )
    $proofClaimState.ClaimsRuntimeRtsBridgeProofSequence = [bool](
        $proofClaimState.ClaimsRuntimeInputBridge -and
        $proofClaimState.ClaimsRuntimeRtsSquad -and
        $lastSmokeResult.RuntimeRtsProofSequenceRan -and
        $lastSmokeResult.RuntimeMapTableClosed
    )
    $proofClaimState.ClaimsTeamVisibilityPolicy = [bool](
        $proofClaimState.ClaimsPolicyAutomationTests -and
        $testPaths -contains 'ArchonFactory.Visibility.BlackCellStaysBlackWithoutSight' -and
        $testPaths -contains 'ArchonFactory.Visibility.SourceLightsCellInRadius' -and
        $testPaths -contains 'ArchonFactory.Visibility.ExploredCellFallsBackToFog' -and
        $testPaths -contains 'ArchonFactory.Visibility.GridAggregatesSourcesAndReportsNewlyLit' -and
        $testPaths -contains 'ArchonFactory.Visibility.BuildingSnapshotFreezesInFogAndUpdatesWhenLit'
    )
    $proofClaimState.ClaimsRuntimeTeamVisibilityState = [bool](
        $proofClaimState.ClaimsTeamVisibilityPolicy -and
        $testPaths -contains 'ArchonFactory.VisibilityState.ConfigureStoresTeamAndCells' -and
        $testPaths -contains 'ArchonFactory.VisibilityState.RecomputeLightsFriendlySources' -and
        $testPaths -contains 'ArchonFactory.VisibilityState.RecomputeTurnsStaleLightToFog' -and
        $testPaths -contains 'ArchonFactory.VisibilityState.BuildingSnapshotsFreezeAndUpdate' -and
        $lastSmokeResult.RuntimeTeamVisibilityInitialized -and
        $lastSmokeResult.RuntimeTeamVisibilityHasLitAndFog
    )
    $proofClaimState.ClaimsMapTableSelectionPolicy = [bool](
        $proofClaimState.ClaimsPolicyAutomationTests -and
        $testPaths -contains 'ArchonFactory.MapTable.NormalizeDragBoxFromAnyCornerOrder' -and
        $testPaths -contains 'ArchonFactory.MapTable.BoxContainsPointInclusiveEdges' -and
        $testPaths -contains 'ArchonFactory.MapTable.SelectionIncludesOnlyOwnTeamLivingInBox' -and
        $testPaths -contains 'ArchonFactory.MapTable.SelectionDegenerateBoxReturnsEmpty' -and
        $testPaths -contains 'ArchonFactory.MapTable.ClickSelectionPicksNearestWithinRadius' -and
        $testPaths -contains 'ArchonFactory.MapTable.ClickSelectionOutsideRadiusReturnsEmpty' -and
        $testPaths -contains 'ArchonFactory.MapTable.ClickSelectionStableTieBreak' -and
        $testPaths -contains 'ArchonFactory.MapTable.TableSpaceToWorldRoundTrip' -and
        $testPaths -contains 'ArchonFactory.MapTable.TableSpaceCornersMapToWorldCorners'
    )
    $proofClaimState.ClaimsMapTableWidgetContract = [bool](
        $proofClaimState.ClaimsMapTableSelectionPolicy -and
        $proofClaimState.ClaimsRuntimeTeamVisibilityState -and
        $testPaths -contains 'ArchonFactory.MapTableWidget.DragBoxSelectsFriendlySquad' -and
        $testPaths -contains 'ArchonFactory.MapTableWidget.RightClickOrderSubmitsTargetLocation' -and
        $testPaths -contains 'ArchonFactory.MapTableWidget.FormatsVisibilitySummary'
    )
    $proofClaimState.ClaimsRuntimeMapTableWidgetMounted = [bool](
        $proofClaimState.ClaimsMapTableWidgetContract -and
        $proofClaimState.ClaimsRuntimeInputBridge -and
        $lastSmokeResult.RuntimeMapTableWidgetOpened -and
        $lastSmokeResult.RuntimeMapTableWidgetSelected -and
        $lastSmokeResult.RuntimeMapTableWidgetOrderSubmitted -and
        $lastSmokeResult.RuntimeMapTableWidgetProofSequenceRan -and
        $lastSmokeResult.RuntimeMapTableWidgetClosed
    )
    $proofClaimState.ClaimsVerdantRootVaultPolicy = [bool](
        $proofClaimState.ClaimsPolicyAutomationTests -and
        $testPaths -contains 'ArchonFactory.Locomotion.VerdantVerbIsRootVault' -and
        $testPaths -contains 'ArchonFactory.Locomotion.OtherFactionsHaveNoVerbAtV0' -and
        $testPaths -contains 'ArchonFactory.Locomotion.NoLaunchWithoutSprint' -and
        $testPaths -contains 'ArchonFactory.Locomotion.NoLaunchBelowMinSprintWindow' -and
        $testPaths -contains 'ArchonFactory.Locomotion.LaunchAtOrAboveMinSprintWindow' -and
        $testPaths -contains 'ArchonFactory.Locomotion.NoLaunchWhileAirborneByDefault' -and
        $testPaths -contains 'ArchonFactory.Locomotion.LaunchAirborneWhenGroundedRequirementOff' -and
        $testPaths -contains 'ArchonFactory.Locomotion.LaunchAppliesExpectedImpulseValues' -and
        $testPaths -contains 'ArchonFactory.Locomotion.LaunchStartsFullCooldown' -and
        $testPaths -contains 'ArchonFactory.Locomotion.NoLaunchWhileOnCooldown' -and
        $testPaths -contains 'ArchonFactory.Locomotion.CooldownAdvancesAndReadyAtZero' -and
        $testPaths -contains 'ArchonFactory.Locomotion.CooldownClampsAtZero'
    )
    $proofClaimState.ClaimsRuntimeRootVaultInputBridge = [bool](
        $proofClaimState.ClaimsRuntimeInputBridge -and
        $proofClaimState.ClaimsVerdantRootVaultPolicy -and
        $proofClaimState.ClaimsFirst60SecondsSmoke -and
        $lastFirst60Result.First60RootVaultInputBridgeRouted
    )
    $proofClaimState.ClaimsBlockoutDebugColors = [bool](
        $proofClaimState.ClaimsPolicyAutomationTests -and
        $testPaths -contains 'ArchonFactory.Blockout.DebugColorsDistinguishFactionActors' -and
        $testPaths -contains 'ArchonFactory.Blockout.CoverStoneDebugColorNeutral'
    )
    $proofClaimState.ClaimsCombatFundamentals = [bool](
        $proofClaimState.ClaimsPolicyAutomationTests -and
        $testPaths -contains 'ArchonFactory.Combat.ResolveShotAcceptedOnBody' -and
        $testPaths -contains 'ArchonFactory.Combat.ResolveShotHeadUsesHeadDamage' -and
        $testPaths -contains 'ArchonFactory.Combat.ResolveShotLimbUsesLimbDamage' -and
        $testPaths -contains 'ArchonFactory.Combat.ResolveShotRejectsWeaponNone' -and
        $testPaths -contains 'ArchonFactory.Combat.ResolveShotRejectsDeadTarget' -and
        $testPaths -contains 'ArchonFactory.Combat.ResolveShotRejectsFriendlyFire' -and
        $testPaths -contains 'ArchonFactory.Combat.ResolveShotRejectsHitTypeNone' -and
        $testPaths -contains 'ArchonFactory.Combat.FalloffKeepsFullDamageBeforeStart' -and
        $testPaths -contains 'ArchonFactory.Combat.FalloffInterpolatesBetweenStartAndEnd' -and
        $testPaths -contains 'ArchonFactory.Combat.FalloffUsesMinDamageAfterEnd' -and
        $testPaths -contains 'ArchonFactory.Combat.DefaultWeaponProfilesUseSliceValues' -and
        $testPaths -contains 'ArchonFactory.Combat.ArmorModifierScalesFinalDamage' -and
        $testPaths -contains 'ArchonFactory.CombatHealth.ConfigureHealthSetsState' -and
        $testPaths -contains 'ArchonFactory.CombatHealth.ApplyHitAcceptedReducesHealth' -and
        $testPaths -contains 'ArchonFactory.CombatHealth.RejectedHitDoesNotMutate' -and
        $testPaths -contains 'ArchonFactory.CombatHealth.DirectDamageCanKill' -and
        $testPaths -contains 'ArchonFactory.CombatHealth.DeadTargetsRejectFurtherDamage' -and
        $testPaths -contains 'ArchonFactory.CombatHealth.HealToFullRestoresHealth' -and
        $testPaths -contains 'ArchonFactory.CombatHealth.ResetProofStateClearsCounters' -and
        $testPaths -contains 'ArchonFactory.CombatHealth.DeathDelegateBroadcastsOnce' -and
        $testPaths -contains 'ArchonFactory.CombatHealth.HealthChangedBroadcastsOnDamage'
    )
    $proofClaimState.ClaimsRangedWeaponFoundation = [bool](
        $proofClaimState.ClaimsCombatFundamentals -and
        $testPaths -contains 'ArchonFactory.Weapon.FireReadyAcceptedDecrementsAmmo' -and
        $testPaths -contains 'ArchonFactory.Weapon.FireRejectsWhenNotPressed' -and
        $testPaths -contains 'ArchonFactory.Weapon.FireRejectsOnCycle' -and
        $testPaths -contains 'ArchonFactory.Weapon.FireRejectsReloading' -and
        $testPaths -contains 'ArchonFactory.Weapon.FireRejectsEmpty' -and
        $testPaths -contains 'ArchonFactory.Weapon.ReloadStartAcceptedFromPartialAmmo' -and
        $testPaths -contains 'ArchonFactory.Weapon.AdvanceFireCycleReturnsReadyOrAutoReload' -and
        $testPaths -contains 'ArchonFactory.Weapon.AdvanceReloadCompletesToFullAmmo' -and
        $testPaths -contains 'ArchonFactory.RangedWeapon.ConfigureSetsStatsAndState' -and
        $testPaths -contains 'ArchonFactory.RangedWeapon.TryFireSpendsAmmoAndBroadcasts' -and
        $testPaths -contains 'ArchonFactory.RangedWeapon.TryFireBlocksOnCycle' -and
        $testPaths -contains 'ArchonFactory.RangedWeapon.TickReturnsToReadyAfterFireCycle' -and
        $testPaths -contains 'ArchonFactory.RangedWeapon.TryReloadStartsAndCompletes' -and
        $testPaths -contains 'ArchonFactory.RangedWeapon.VerdantBowDefaultsMatchContract' -and
        $testPaths -contains 'ArchonFactory.RangedWeapon.CanaryFpsCharacterOwnsVerdantBow'
    )
    $proofClaimState.ClaimsRuntimeVerdantBowInputBridge = [bool](
        $proofClaimState.ClaimsRangedWeaponFoundation -and
        $proofClaimState.ClaimsRuntimeInputBridge -and
        $testPaths -contains 'ArchonFactory.RuntimeInputBridge.WeaponInputFiresWhenMapTableClosed' -and
        $testPaths -contains 'ArchonFactory.RuntimeInputBridge.WeaponInputDoesNotFireWhenMapTableOpen' -and
        $testPaths -contains 'ArchonFactory.RuntimeInputBridge.WeaponInputReloadsAfterFireCycle' -and
        $testPaths -contains 'ArchonFactory.RuntimeInputBridge.WeaponProofSequenceFiresAndReloads' -and
        $lastSmokeResult.RuntimeWeaponFireRouted -and
        $lastSmokeResult.RuntimeWeaponReloadRouted -and
        $lastSmokeResult.RuntimeWeaponProofSequenceRan -and
        $lastSmokeResult.RuntimeWeaponProofRequested
    )
    $proofClaimState.ClaimsLenswrightAiCombat = [bool](
        $proofClaimState.ClaimsRangedWeaponFoundation -and
        $testPaths -contains 'ArchonFactory.AiCombat.SelectTargetPicksNearestEnemy' -and
        $testPaths -contains 'ArchonFactory.AiCombat.SelectTargetRejectsFriendlyDeadAndUnknown' -and
        $testPaths -contains 'ArchonFactory.AiCombat.SelectTargetRejectsOutOfRange' -and
        $testPaths -contains 'ArchonFactory.AiCombat.SelectTargetStableTieBreaksByTargetId' -and
        $testPaths -contains 'ArchonFactory.AiCombat.EvaluateDefensiveRangedEngagesTargetInRange' -and
        $testPaths -contains 'ArchonFactory.AiCombat.EvaluateDefensiveRangedHoldsBeyondEngageRange' -and
        $testPaths -contains 'ArchonFactory.AiCombat.EvaluateDefensiveRangedSeeksCoverAtLowHealth' -and
        $testPaths -contains 'ArchonFactory.AiCombat.EvaluateDefensiveRangedRetreatsAtCriticalHealth' -and
        $testPaths -contains 'ArchonFactory.AiCombat.EvaluateScoutNeverFiresAndRetreatsWhenCritical' -and
        $testPaths -contains 'ArchonFactory.Lenswright.PressureBoltCrossbowDefaultsMatchContract' -and
        $testPaths -contains 'ArchonFactory.Lenswright.PressureBoltProjectileSpeedUnderHitscan' -and
        $testPaths -contains 'ArchonFactory.Lenswright.BracewrightConfiguresCombatAndAi' -and
        $testPaths -contains 'ArchonFactory.Lenswright.SundialOpticHasVisionAndNoWeapon' -and
        $testPaths -contains 'ArchonFactory.Lenswright.BracewrightCanFirePressureBoltAtEnemy'
    )
    $proofClaimState.ClaimsRuntimeLenswrightCombatProof = [bool](
        $proofClaimState.ClaimsLenswrightAiCombat -and
        $proofClaimState.ClaimsRuntimeVerdantBowInputBridge -and
        $lastSmokeResult.RuntimeLenswrightBracewrightSpawned -and
        $lastSmokeResult.RuntimeLenswrightSundialOpticSpawned -and
        $lastSmokeResult.RuntimeLenswrightTargetDamaged -and
        $lastSmokeResult.RuntimeLenswrightAiDecisionShouldFire -and
        $lastSmokeResult.RuntimeLenswrightAiFireRouted -and
        $lastSmokeResult.RuntimeLenswrightAiDamagedPlayer -and
        $lastSmokeResult.RuntimeCombatProofSequenceRan -and
        $lastSmokeResult.RuntimeCombatProofRequested
    )
} else {
    $automationReportPath = Join-Path $ProjectRoot 'Saved\Proof\Automation\index.json'
    if (Test-Path -LiteralPath $automationReportPath -PathType Leaf) {
        $automationReport = Get-Content -LiteralPath $automationReportPath -Raw | ConvertFrom-Json
        $proofClaimState.ClaimsPolicyAutomationTests = [bool]($automationReport.succeeded -eq 4 -and $automationReport.failed -eq 0)
    }
}

$result = [pscustomobject]@{
    ProjectRoot = $ProjectRoot
    EngineRoot = $engineRoot
    EngineRootExists = Test-Path -LiteralPath $engineRoot -PathType Container
    UnrealEditorExists = Test-Path -LiteralPath $editorExe -PathType Leaf
    DesktopShortcut = $desktopShortcut
    DesktopShortcutExists = $desktopShortcutInfo.Exists
    DesktopShortcutInfo = $desktopShortcutInfo
    CurrentBuildLauncher = $currentBuildLauncher
    Checks = $checks
    Missing = @($checks | Where-Object { -not $_.Exists } | ForEach-Object { $_.Path })
    ProofClaimState = $proofClaimState
    LastUnrealMapSmoke = $lastSmokeResult
    LastFirst60SecondsSmoke = $lastFirst60Result
    LastSecond60SecondsSmoke = $lastSecond60Result
    LastMatchLoopSmoke = $lastMatchLoopResult
    LastMapRotationSmoke = $lastMapRotationResult
    LastFrontEndSmoke = $lastFrontEndResult
    LastTwoPlayerJoinSmoke = $lastTwoPlayerResult
    LastBotMatchSmoke = $lastBotMatchResult
    LastPlaytestRender = $lastPlaytestRender
    LastPolicyBuildAndTest = $policyBuildResult
    NextProofNeeded = @(
        'Open Unreal Editor UI against ArchonFactoryCanary.uproject',
        'Manually launch SPLITROOT Current Build.lnk for feel/playtest after smoke proof',
        'Manually verify native FPS pawn/controller controls and map-table interactions in editor or packaged build',
        'Create WBP_ArchonMapTableWidget presentation asset and verify drag visuals, selection arrows, and pulse marker',
        'Move the first-60 proof from FirstPerson bootstrap actors onto a dedicated SplitrootValley_V0 map asset',
        'Persist map-table actor placement or bootstrap policy for production maps',
        'Replace canary text-render map-table feedback with production RTS UI',
        'Run manual rendered playtest for Lenswright Bracewright/Sundial readability and pressure-bolt feel',
        'Package a Windows build'
    )
}

$result | ConvertTo-Json -Depth 6

if (
    $result.Missing.Count -gt 0 -or
    -not $result.EngineRootExists -or
    -not $result.UnrealEditorExists -or
    -not $result.DesktopShortcutExists -or
    -not $result.DesktopShortcutInfo.TargetMatchesCurrentBuildLauncher -or
    -not $result.DesktopShortcutInfo.WorkingDirectoryMatchesProjectRoot
) {
    exit 1
}
