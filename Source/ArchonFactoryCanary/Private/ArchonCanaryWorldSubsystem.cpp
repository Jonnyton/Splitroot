#include "ArchonCanaryWorldSubsystem.h"
#include "ArchonAiCombatBehaviorComponent.h"
#include "ArchonArrowProjectile.h"
#include "ArchonBaseCoreActor.h"
#include "ArchonBaseDefenseTowerActor.h"
#include "ArchonBotAIController.h"
#include "ArchonBotBrainComponent.h"
#include "ArchonBotStrategyTuningLibrary.h"
#include "ArchonBlockoutActors.h"
#include "ArchonLaunchFlagLibrary.h"
#include "ArchonMainMenuController.h"
#include "ArchonMainMenuPanel.h"
#include "ArchonMapRegistryLibrary.h"
#include "ArchonMatchStateActor.h"
#include "ArchonMatchTypes.h"
#include "ArchonFactionPaletteLibrary.h"
#include "ArchonFactionPaletteTypes.h"
#include "ArchonSplitrootValleyLayoutLibrary.h"
#include "ArchonValleyBlockActor.h"
#include "ArchonValleyLayoutTypes.h"
#include "ArchonValleyNetBeacon.h"
#include "ArchonCanaryFpsCharacter.h"
#include "ArchonCanaryRtsSquadActor.h"
#include "ArchonCombatHealthComponent.h"
#include "ArchonCombatPolicyLibrary.h"
#include "ArchonFactionMovementComponent.h"
#include "ArchonLenswrightBracewrightActor.h"
#include "ArchonLenswrightPressureBoltCrossbow.h"
#include "ArchonLenswrightSundialOpticActor.h"
#include "ArchonMapTableActor.h"
#include "ArchonMapTableInteractionTypes.h"
#include "ArchonMapTableWidget.h"
#include "ArchonPlayerInputBridgeComponent.h"
#include "ArchonPressureBoltProjectile.h"
#include "ArchonRespawnObserverPawn.h"
#include "ArchonRespawnStateComponent.h"
#include "ArchonSessionTypes.h"
#include "ArchonTeamRtsStateComponent.h"
#include "ArchonTeamVisibilityStateComponent.h"
#include "ArchonTeamVisibilityTypes.h"
#include "ArchonTechBuildingActor.h"
#include "ArchonVerdantThornsproutBow.h"
#include "Components/LightComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/Engine.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/SkyLight.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/SpectatorPawn.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformFileManager.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"
#include "Misc/App.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

namespace
{
	FArchonMatchConfig BuildArchonMatchConfig(const UWorld* World)
	{
		FArchonMatchConfig Config;
		if (UArchonLaunchFlagLibrary::IsLaunchFlagActive(World, TEXT("ArchonMatchProof")))
		{
			// Proof clock: the smoke walks the whole lifecycle in ~15s of
			// real frames instead of a 20-minute match.
			Config.WarmupSeconds = 2.0f;
			Config.ScoreboardSeconds = 3.0f;
			Config.SiteCaptureSeconds = 2.0f;
			Config.SupplyTickIntervalSeconds = 3.0f;
		}
		if (UArchonLaunchFlagLibrary::IsLaunchFlagActive(World, TEXT("ArchonRunRuntimeRallyProof")))
		{
			Config.ReinforcementSquadSupplyCost = 5;
			Config.AutoReinforceBankMultiple = 1000.0f;
		}
		return Config;
	}

	struct FArchonHudBuildStamp
	{
		FString Version;
		FString EffectiveUtc;
		FString ModuleUtc;
	};

	const FDateTime GArchonHudProcessStartUtc = FDateTime::UtcNow();

	FString FormatUtcOrMissing(const FDateTime& Timestamp)
	{
		return Timestamp.GetTicks() > 0 ? Timestamp.ToIso8601() : FString(TEXT("missing"));
	}

	FString MakeHudBuildVersion(const FString& ModuleDllUtc)
	{
		FString Version = ModuleDllUtc;
		Version.ReplaceInline(TEXT("-"), TEXT(""));
		Version.ReplaceInline(TEXT(":"), TEXT(""));
		Version.ReplaceInline(TEXT("."), TEXT(""));
		Version.ReplaceInline(TEXT("Z"), TEXT(""));
		Version.ReplaceInline(TEXT("T"), TEXT("-"));
		if (Version.IsEmpty() || Version.Equals(TEXT("missing"), ESearchCase::IgnoreCase))
		{
			return FString(TEXT("vunknown"));
		}
		return FString::Printf(TEXT("v%s"), *Version.Left(15));
	}

	FString ResolveHudModuleDllPath()
	{
		return FPaths::ConvertRelativePathToFull(FPaths::Combine(
			FPaths::ProjectDir(),
			TEXT("Binaries"),
			TEXT("Win64"),
			TEXT("UnrealEditor-ArchonFactoryCanary.dll")));
	}

	FArchonHudBuildStamp ResolveLocalHudBuildStamp()
	{
		const FString ModuleUtc = FormatUtcOrMissing(IFileManager::Get().GetTimeStamp(*ResolveHudModuleDllPath()));
		return FArchonHudBuildStamp{
			MakeHudBuildVersion(ModuleUtc),
			GArchonHudProcessStartUtc.ToIso8601(),
			ModuleUtc
		};
	}

	bool ApplyProofProjectileHit(
		UWorld& World,
		TSubclassOf<AArchonArrowProjectile> ProjectileClass,
		const FArchonShotPayload& Shot,
		AActor* Target,
		AActor* Owner)
	{
		if (!Target)
		{
			return false;
		}

		UClass* ClassToSpawn = ProjectileClass ? ProjectileClass.Get() : AArchonArrowProjectile::StaticClass();
		FActorSpawnParameters SpawnParameters;
		SpawnParameters.Owner = Owner;
		SpawnParameters.Instigator = Cast<APawn>(Owner);
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		AArchonArrowProjectile* Projectile = World.SpawnActor<AArchonArrowProjectile>(
			ClassToSpawn,
			Shot.ShotOrigin,
			Shot.ShotDirection.Rotation(),
			SpawnParameters);
		if (!Projectile)
		{
			return false;
		}

		FHitResult Hit;
		Hit.ImpactPoint = Target->GetActorLocation();
		Hit.Location = Hit.ImpactPoint;
		Projectile->ConfigureShot(
			Shot,
			UArchonCombatPolicyLibrary::GetDefaultWeaponProfile(Shot.WeaponClass));
		Projectile->HandleHit(Target, nullptr, FVector::ZeroVector, Hit);
		return true;
	}
}

void UArchonCanaryWorldSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	if (!InWorld.IsGameWorld() || RuntimeMapTable)
	{
		return;
	}

	// Every human-visible session is a playtest worth learning from
	// (Jonathan 2026-06-10) — record it whether Rook's harness launched it
	// or Jonathan double-clicked the shortcut. Headless smokes are excluded
	// by the unattended/render gates inside.
	StartPlaytestRecorderIfHumanSession();
	ShowBuildVersionStampIfNeeded();

	SpawnSplitrootValleyIfRequested();
	SpawnMainMenuIfRequested();

	// Menu world stays menu-only. The runtime bridge below would possess an
	// FPS body, force FInputModeGameOnly, and hide the mouse cursor — which
	// left the menu unclickable (invisible-mouse bug, Jonathan 2026-06-10).
	if (MainMenu)
	{
		UE_LOG(LogTemp, Display, TEXT("ArchonFactoryCanary: MainMenuWorldIsMenuOnly bridgeSkipped=true"));
		return;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Name = TEXT("ArchonCanaryRuntimeMapTable");
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	RuntimeMapTable = InWorld.SpawnActor<AArchonMapTableActor>(
		AArchonMapTableActor::StaticClass(),
		bValleyActive ? ValleyMapTableLocation : FVector(350.0, 0.0, 120.0),
		FRotator::ZeroRotator,
		SpawnParameters);

	if (RuntimeMapTable)
	{
		RuntimeMapTable->ConfigureMapTable(0, EArchonSessionRoute::PrivateHost, false);
		UE_LOG(LogTemp, Display, TEXT("ArchonFactoryCanary: RuntimeMapTableSpawned name=%s route=PrivateHost team=0"), *RuntimeMapTable->GetName());
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("ArchonFactoryCanary: RuntimeMapTableSpawnFailed"));
	}

	SpawnRuntimeRtsSquad();
	InitializeRuntimeTeamVisibility();
	SpawnBlockoutActorsIfRequested();

	// Rotation-proof plumbing: a match-proof travel appends this option to
	// the next map's URL so the smoke can verify the new world booted and
	// then end the process from C++.
	if (UArchonLaunchFlagLibrary::IsLaunchFlagActive(&InWorld, TEXT("ArchonMatchProofQuit")))
	{
		UE_LOG(LogTemp, Display, TEXT("ArchonFactoryCanary: MatchProofQuitScheduled map=%s delaySeconds=2.0"), *InWorld.GetMapName());
		InWorld.GetTimerManager().SetTimer(
			MatchProofQuitTimer,
			this,
			&UArchonCanaryWorldSubsystem::IssuePlayTestQuit,
			2.0f,
			false);
	}

	InstallRuntimePlayerBridge();
	if (!RuntimeInputBridge)
	{
		InWorld.GetTimerManager().SetTimer(
			RuntimeBridgeInstallTimer,
			this,
			&UArchonCanaryWorldSubsystem::InstallRuntimePlayerBridge,
			0.25f,
			true);
	}
}

void UArchonCanaryWorldSubsystem::SpawnRuntimeRtsSquad()
{
	UWorld* World = GetWorld();
	if (!World || !World->IsGameWorld() || RuntimeRtsSquad)
	{
		return;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Name = TEXT("ArchonCanaryRuntimeRtsSquad");
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	RuntimeRtsSquad = World->SpawnActor<AArchonCanaryRtsSquadActor>(
		AArchonCanaryRtsSquadActor::StaticClass(),
		bValleyActive ? ValleySquadSpawnLocation : FVector(640.0f, -260.0f, 120.0f),
		FRotator::ZeroRotator,
		SpawnParameters);

	if (RuntimeRtsSquad)
	{
		RuntimeRtsSquad->ConfigureSquad(0, TEXT("canary_squad"));
		RuntimeRtsSquad->AttachTeamState(RuntimeMapTable ? RuntimeMapTable->GetTeamState() : nullptr);
		UE_LOG(LogTemp, Display, TEXT("ArchonFactoryCanary: RuntimeRtsSquadSpawned name=%s subject=canary_squad"), *RuntimeRtsSquad->GetName());
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("ArchonFactoryCanary: RuntimeRtsSquadSpawnFailed"));
	}
}

void UArchonCanaryWorldSubsystem::InitializeRuntimeTeamVisibility()
{
	if (!RuntimeMapTable)
	{
		return;
	}

	UArchonTeamVisibilityStateComponent* VisibilityState = RuntimeMapTable->GetTeamVisibilityState();
	if (!VisibilityState)
	{
		UE_LOG(LogTemp, Error, TEXT("ArchonFactoryCanary: RuntimeTeamVisibilityInitializeFailed reason=missing_visibility_state"));
		return;
	}

	const FVector MapTableLocation = RuntimeMapTable->GetActorLocation();
	const FVector SquadLocation = RuntimeRtsSquad
		? RuntimeRtsSquad->GetActorLocation()
		: FVector(640.0f, -260.0f, 120.0f);

	TArray<FArchonVisibilityGridCell> InitialCells;

	FArchonVisibilityGridCell MapTableCell;
	MapTableCell.Cell = FIntPoint(0, 0);
	MapTableCell.WorldCenter = MapTableLocation;
	MapTableCell.State = EArchonTeamVisibilityState::Black;
	InitialCells.Add(MapTableCell);

	FArchonVisibilityGridCell SquadCell;
	SquadCell.Cell = FIntPoint(1, 0);
	SquadCell.WorldCenter = SquadLocation;
	SquadCell.State = EArchonTeamVisibilityState::Black;
	InitialCells.Add(SquadCell);

	FArchonVisibilityGridCell StaleObjectiveCell;
	StaleObjectiveCell.Cell = FIntPoint(2, 0);
	StaleObjectiveCell.WorldCenter = FVector(1600.0f, 0.0f, 120.0f);
	StaleObjectiveCell.State = EArchonTeamVisibilityState::Lit;
	InitialCells.Add(StaleObjectiveCell);

	FArchonVisibilitySource MapTableSource;
	MapTableSource.TeamId = RuntimeMapTable->GetTeamId();
	MapTableSource.Location = MapTableLocation;
	MapTableSource.Radius = 260.0f;

	FArchonVisibilitySource SquadSource;
	SquadSource.TeamId = RuntimeMapTable->GetTeamId();
	SquadSource.Location = SquadLocation;
	SquadSource.Radius = 260.0f;

	VisibilityState->ConfigureVisibilityState(RuntimeMapTable->GetTeamId(), InitialCells);
	VisibilityState->SetFriendlySources({ MapTableSource, SquadSource });
	VisibilityState->RecomputeVisibility();

	UE_LOG(
		LogTemp,
		Display,
		TEXT("ArchonFactoryCanary: RuntimeTeamVisibilityInitialized team=%d cells=%d lit=%d fog=%d black=%d newlyLit=%d"),
		VisibilityState->GetTeamId(),
		VisibilityState->GetVisibilityCells().Num(),
		VisibilityState->CountCellsInState(EArchonTeamVisibilityState::Lit),
		VisibilityState->CountCellsInState(EArchonTeamVisibilityState::Fog),
		VisibilityState->CountCellsInState(EArchonTeamVisibilityState::Black),
		VisibilityState->GetLastNewlyLitCellCount());
}

void UArchonCanaryWorldSubsystem::InstallRuntimePlayerBridge()
{
	UWorld* World = GetWorld();
	if (!World || !World->IsGameWorld())
	{
		return;
	}

	++RuntimeBridgeInstallAttempts;
	APlayerController* PlayerController = World->GetFirstPlayerController();
	if (!PlayerController)
	{
		if (RuntimeBridgeInstallAttempts > 20)
		{
			World->GetTimerManager().ClearTimer(RuntimeBridgeInstallTimer);
			UE_LOG(LogTemp, Warning, TEXT("ArchonFactoryCanary: RuntimeInputBridgeInstallSkipped reason=no_player_controller"));
		}
		return;
	}

	if (!RuntimeFpsCharacter)
	{
		APawn* ExistingPawn = PlayerController->GetPawn();

		// The match GameMode already spawns FPS characters per player;
		// adopt that pawn instead of spawning a duplicate body.
		if (AArchonCanaryFpsCharacter* ExistingFpsCharacter = Cast<AArchonCanaryFpsCharacter>(ExistingPawn))
		{
			RuntimeFpsCharacter = ExistingFpsCharacter;
			PlayerController->SetInputMode(FInputModeGameOnly());
			PlayerController->SetShowMouseCursor(false);
			UE_LOG(LogTemp, Display, TEXT("ArchonFactoryCanary: RuntimeFpsPawnAdopted name=%s"), *RuntimeFpsCharacter->GetName());
		}
		else if (World->GetNetMode() == NM_Client)
		{
			// Bodies are server-authoritative; wait for the replicated pawn
			// (the install timer keeps retrying) rather than spawning a
			// client-local duplicate.
			return;
		}
	}

	if (!RuntimeFpsCharacter)
	{
		APawn* ExistingPawn = PlayerController->GetPawn();
		FVector SpawnLocation = ExistingPawn ? ExistingPawn->GetActorLocation() : FVector(0.0f, 0.0f, 180.0f);
		FRotator SpawnRotation = ExistingPawn ? ExistingPawn->GetActorRotation() : FRotator::ZeroRotator;
		if (bValleyActive)
		{
			SpawnLocation = ValleyPlayerSpawnLocation;
			SpawnRotation = ValleyPlayerSpawnRotation;
		}

		FActorSpawnParameters SpawnParameters;
		SpawnParameters.Name = TEXT("ArchonCanaryRuntimeFpsCharacter");
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		RuntimeFpsCharacter = World->SpawnActor<AArchonCanaryFpsCharacter>(
			AArchonCanaryFpsCharacter::StaticClass(),
			SpawnLocation,
			SpawnRotation,
			SpawnParameters);

		if (RuntimeFpsCharacter)
		{
			if (ExistingPawn && ExistingPawn != RuntimeFpsCharacter)
			{
				ExistingPawn->SetActorHiddenInGame(true);
				ExistingPawn->SetActorEnableCollision(false);
			}

			PlayerController->Possess(RuntimeFpsCharacter);
			PlayerController->SetInputMode(FInputModeGameOnly());
			PlayerController->SetShowMouseCursor(false);
			UE_LOG(LogTemp, Display, TEXT("ArchonFactoryCanary: RuntimeFpsPawnPossessed name=%s"), *RuntimeFpsCharacter->GetName());
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("ArchonFactoryCanary: RuntimeFpsPawnSpawnFailed"));
			return;
		}
	}

	InstallRuntimeRespawnState(*PlayerController);
	BindRuntimeFpsDeath(RuntimeFpsCharacter);

	if (!RuntimeInputBridge)
	{
		RuntimeInputBridge = NewObject<UArchonPlayerInputBridgeComponent>(PlayerController, TEXT("ArchonRuntimeInputBridge"));
		if (RuntimeInputBridge)
		{
			RuntimeInputBridge->RegisterComponent();

			FArchonMapTableInteractorConfig InteractorConfig;
			InteractorConfig.PlayerId = 0;
			InteractorConfig.TeamId = 0;
			RuntimeInputBridge->ConfigureBridge(PlayerController, RuntimeMapTable, InteractorConfig);
			RuntimeInputBridge->SetRespawnStateComponent(RuntimeRespawnState);
			RunRuntimeRtsProofSequenceIfRequested();
			RunRuntimeTechShopProofSequenceIfRequested();
			RunRuntimeRallyProofSequenceIfRequested();
			RunRuntimeWeaponProofSequenceIfRequested();
			RunRuntimeCombatProofSequenceIfRequested();
			RunRuntimeRespawnProofSequenceIfRequested();
			RunFirst60SecondsProofIfRequested();
			RunClientFireProofIfRequested();
			RunClientOrderProofIfRequested();
			RunPauseMenuProofIfRequested();
			RunInteractProofIfRequested();
		}
	}

	if (RuntimeInputBridge)
	{
		RuntimeInputBridge->SetRespawnStateComponent(RuntimeRespawnState);
	}

	if (RuntimeInputBridge)
	{
		World->GetTimerManager().ClearTimer(RuntimeBridgeInstallTimer);
	}
}

void UArchonCanaryWorldSubsystem::InstallRuntimeRespawnState(APlayerController& PlayerController)
{
	if (!RuntimeRespawnState)
	{
		RuntimeRespawnState = NewObject<UArchonRespawnStateComponent>(&PlayerController, TEXT("ArchonRuntimeRespawnState"));
		if (RuntimeRespawnState)
		{
			RuntimeRespawnState->RegisterComponent();
			FArchonRespawnTuning Tuning;
			RuntimeRespawnState->ConfigureRespawn(0, 0, Tuning);
			RuntimeRespawnState->OnRespawnReady.AddUObject(this, &UArchonCanaryWorldSubsystem::HandleRuntimeRespawnReady);
			UE_LOG(LogTemp, Display, TEXT("ArchonFactoryCanary: RuntimeRespawnStateInstalled player=0 team=0"));
		}
	}

	RegisterRuntimeRespawnSpawnPoints();
}

void UArchonCanaryWorldSubsystem::RegisterRuntimeRespawnSpawnPoints()
{
	if (!RuntimeRespawnState)
	{
		return;
	}

	FArchonRespawnSpawnPoint BaseSpawn;
	BaseSpawn.SpawnPointId = TEXT("canary_base_spawn");
	BaseSpawn.TeamId = 0;
	BaseSpawn.Location = RuntimeMapTable
		? RuntimeMapTable->GetActorLocation() + FVector(-350.0f, 0.0f, 80.0f)
		: FVector(0.0f, 0.0f, 180.0f);
	BaseSpawn.Rotation = FRotator::ZeroRotator;
	BaseSpawn.bIsForwardSpawn = false;
	BaseSpawn.bIsAvailable = true;
	RuntimeRespawnState->RegisterSpawnPoint(BaseSpawn);

	FArchonRespawnSpawnPoint ForwardSpawn;
	ForwardSpawn.SpawnPointId = TEXT("canary_forward_spawn");
	ForwardSpawn.TeamId = 0;
	ForwardSpawn.Location = RuntimeRtsSquad
		? RuntimeRtsSquad->GetActorLocation() + FVector(-120.0f, 0.0f, 80.0f)
		: FVector(640.0f, -260.0f, 180.0f);
	ForwardSpawn.Rotation = FRotator::ZeroRotator;
	ForwardSpawn.bIsForwardSpawn = true;
	ForwardSpawn.bIsAvailable = true;
	RuntimeRespawnState->RegisterSpawnPoint(ForwardSpawn);
}

void UArchonCanaryWorldSubsystem::BindRuntimeFpsDeath(AArchonCanaryFpsCharacter* FpsCharacter)
{
	if (!FpsCharacter)
	{
		return;
	}

	UArchonCombatHealthComponent* Health = FpsCharacter->GetHealth();
	if (!Health)
	{
		return;
	}

	Health->OnDeath.RemoveAll(this);
	Health->OnDeath.AddUObject(this, &UArchonCanaryWorldSubsystem::HandleRuntimeFpsDeath);
}

void UArchonCanaryWorldSubsystem::RunRuntimeRtsProofSequenceIfRequested()
{
	if (bRuntimeRtsProofSequenceRan || !RuntimeInputBridge)
	{
		return;
	}

	if (!FParse::Param(FCommandLine::Get(), TEXT("ArchonRunRuntimeRtsProof")))
	{
		return;
	}

	const bool bRuntimeWidgetProofSequenceRan = RuntimeInputBridge->RunRuntimeMapTableWidgetProofSequence();
	const bool bRuntimeRtsOnlyProofSequenceRan = RuntimeInputBridge->RunRuntimeRtsProofSequence();
	bRuntimeRtsProofSequenceRan = bRuntimeWidgetProofSequenceRan && bRuntimeRtsOnlyProofSequenceRan;
	if (bRuntimeRtsProofSequenceRan)
	{
		UE_LOG(LogTemp, Display, TEXT("ArchonFactoryCanary: RuntimeRtsProofSequenceRequested result=true widget=true rts=true"));
	}
	else
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("ArchonFactoryCanary: RuntimeRtsProofSequenceRequested result=false widget=%s rts=%s"),
			bRuntimeWidgetProofSequenceRan ? TEXT("true") : TEXT("false"),
			bRuntimeRtsOnlyProofSequenceRan ? TEXT("true") : TEXT("false"));
	}
}

void UArchonCanaryWorldSubsystem::RunRuntimeTechShopProofSequenceIfRequested()
{
	if (bRuntimeTechShopProofScheduled || bRuntimeTechShopProofSequenceRan || !RuntimeInputBridge)
	{
		return;
	}

	if (!FParse::Param(FCommandLine::Get(), TEXT("ArchonRunRuntimeTechShopProof")))
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	bRuntimeTechShopProofScheduled = true;
	World->GetTimerManager().SetTimer(
		RuntimeTechShopProofTimer,
		this,
		&UArchonCanaryWorldSubsystem::ExecuteRuntimeTechShopProofSequence,
		27.0f,
		false);
	UE_LOG(LogTemp, Display, TEXT("ArchonFactoryCanary: RuntimeTechShopProofScheduled delay=27.0"));
}

void UArchonCanaryWorldSubsystem::ExecuteRuntimeTechShopProofSequence()
{
	const bool bOpened = RuntimeInputBridge && RuntimeInputBridge->PreviewRuntimeMapTable();
	const bool bTechBuilt = RuntimeInputBridge && RuntimeInputBridge->RequestTechBuildingPurchase();
	const bool bClosed = RuntimeInputBridge && RuntimeInputBridge->CloseRuntimeMapTable();

	AArchonMatchStateActor* State = ResolveMatchState();
	const TArray<FName> BuiltTech = State ? State->GetBuiltTech(0) : TArray<FName>();
	const bool bArmoryLive = BuiltTech.Contains(TEXT("armory"));
	AArchonTechBuildingActor* ArmoryActor = nullptr;
	for (AArchonTechBuildingActor* Building : ValleyTechBuildings)
	{
		if (Building && Building->GetTeamId() == 0 && Building->GetTechId() == TEXT("armory"))
		{
			ArmoryActor = Building;
			break;
		}
	}
	UArchonCombatHealthComponent* ArmoryHealth = ArmoryActor ? ArmoryActor->GetHealth() : nullptr;
	const bool bArmoryActorLive = ArmoryHealth && ArmoryHealth->IsAlive();
	const FArchonDamageApplicationResult DestroyResult = ArmoryHealth
		? ArmoryHealth->ApplyDirectDamage(9999.0f, EArchonDamageType::Generic, 1, INDEX_NONE)
		: FArchonDamageApplicationResult();
	const TArray<FName> TechAfterLoss = State ? State->GetBuiltTech(0) : TArray<FName>();
	const bool bArmoryLost = !TechAfterLoss.Contains(TEXT("armory"));
	bRuntimeTechShopProofSequenceRan =
		bOpened &&
		bTechBuilt &&
		bClosed &&
		bArmoryLive &&
		bArmoryActorLive &&
		DestroyResult.bCausedDeath &&
		bArmoryLost;

	if (bRuntimeTechShopProofSequenceRan)
	{
		UE_LOG(
			LogTemp,
			Display,
			TEXT("ArchonFactoryCanary: RuntimeTechShopProof completed=true opened=%s techBuilt=%s closed=%s armoryLive=%s actorLive=%s destroyed=%s armoryLost=%s supply=%d"),
			bOpened ? TEXT("true") : TEXT("false"),
			bTechBuilt ? TEXT("true") : TEXT("false"),
			bClosed ? TEXT("true") : TEXT("false"),
			bArmoryLive ? TEXT("true") : TEXT("false"),
			bArmoryActorLive ? TEXT("true") : TEXT("false"),
			DestroyResult.bCausedDeath ? TEXT("true") : TEXT("false"),
			bArmoryLost ? TEXT("true") : TEXT("false"),
			State ? State->GetTeamSupply(0) : INDEX_NONE);
	}
	else
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("ArchonFactoryCanary: RuntimeTechShopProof completed=false opened=%s techBuilt=%s closed=%s armoryLive=%s actorLive=%s destroyed=%s armoryLost=%s supply=%d"),
			bOpened ? TEXT("true") : TEXT("false"),
			bTechBuilt ? TEXT("true") : TEXT("false"),
			bClosed ? TEXT("true") : TEXT("false"),
			bArmoryLive ? TEXT("true") : TEXT("false"),
			bArmoryActorLive ? TEXT("true") : TEXT("false"),
			DestroyResult.bCausedDeath ? TEXT("true") : TEXT("false"),
			bArmoryLost ? TEXT("true") : TEXT("false"),
			State ? State->GetTeamSupply(0) : INDEX_NONE);
	}

	IssuePlayTestQuit();
}

void UArchonCanaryWorldSubsystem::RunRuntimeRallyProofSequenceIfRequested()
{
	if (bRuntimeRallyProofScheduled || bRuntimeRallyProofSequenceRan || !RuntimeInputBridge)
	{
		return;
	}

	if (!FParse::Param(FCommandLine::Get(), TEXT("ArchonRunRuntimeRallyProof")))
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	bRuntimeRallyProofScheduled = true;
	World->GetTimerManager().SetTimer(
		RuntimeRallyProofTimer,
		this,
		&UArchonCanaryWorldSubsystem::ExecuteRuntimeRallyProofSequence,
		7.0f,
		false);
	UE_LOG(LogTemp, Display, TEXT("ArchonFactoryCanary: RuntimeRallyProofScheduled delay=7.0"));
}

void UArchonCanaryWorldSubsystem::ExecuteRuntimeRallyProofSequence()
{
	const int32 TeamId = RuntimeMapTable ? RuntimeMapTable->GetTeamId() : 0;
	const int32 FieldedBefore = (TeamId >= 0 && TeamId <= 1) ? FieldedBotsPerTeam[TeamId] : 0;
	const bool bOpened = RuntimeInputBridge && RuntimeInputBridge->PreviewRuntimeMapTable();
	const bool bRallySet = RuntimeInputBridge && RuntimeInputBridge->SubmitRuntimeMapTableWidgetRallyPointAt(
		FVector2D(2.0f / 3.0f, 0.50f),
		TEXT("runtime_rally_proof"));
	const bool bRallyStored = RuntimeMapTable &&
		RuntimeMapTable->GetTeamState() &&
		RuntimeMapTable->GetTeamState()->HasTeamRallyPoint();
	const FVector RallyPoint = bRallyStored
		? RuntimeMapTable->GetTeamState()->GetTeamRallyPoint()
		: FVector::ZeroVector;
	AArchonMatchStateActor* State = ResolveMatchState();
	const bool bPurchased = State && State->TryPurchaseReinforcement(TeamId);
	const bool bClosed = RuntimeInputBridge && RuntimeInputBridge->CloseRuntimeMapTable();
	const int32 FieldedAfter = (TeamId >= 0 && TeamId <= 1) ? FieldedBotsPerTeam[TeamId] : FieldedBefore;
	const bool bFielded = FieldedAfter > FieldedBefore;

	bRuntimeRallyProofSequenceRan =
		bOpened &&
		bRallySet &&
		bRallyStored &&
		bPurchased &&
		bClosed &&
		bFielded;

	if (bRuntimeRallyProofSequenceRan)
	{
		UE_LOG(
			LogTemp,
			Display,
			TEXT("ArchonFactoryCanary: RuntimeRallyProof completed=true opened=%s rallySet=%s rallyStored=%s purchased=%s closed=%s fieldedBefore=%d fieldedAfter=%d rally=%s supply=%d"),
			bOpened ? TEXT("true") : TEXT("false"),
			bRallySet ? TEXT("true") : TEXT("false"),
			bRallyStored ? TEXT("true") : TEXT("false"),
			bPurchased ? TEXT("true") : TEXT("false"),
			bClosed ? TEXT("true") : TEXT("false"),
			FieldedBefore,
			FieldedAfter,
			*RallyPoint.ToCompactString(),
			State ? State->GetTeamSupply(TeamId) : INDEX_NONE);
	}
	else
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("ArchonFactoryCanary: RuntimeRallyProof completed=false opened=%s rallySet=%s rallyStored=%s purchased=%s closed=%s fieldedBefore=%d fieldedAfter=%d rally=%s supply=%d"),
			bOpened ? TEXT("true") : TEXT("false"),
			bRallySet ? TEXT("true") : TEXT("false"),
			bRallyStored ? TEXT("true") : TEXT("false"),
			bPurchased ? TEXT("true") : TEXT("false"),
			bClosed ? TEXT("true") : TEXT("false"),
			FieldedBefore,
			FieldedAfter,
			*RallyPoint.ToCompactString(),
			State ? State->GetTeamSupply(TeamId) : INDEX_NONE);
	}

	IssuePlayTestQuit();
}

void UArchonCanaryWorldSubsystem::RunRuntimeWeaponProofSequenceIfRequested()
{
	if (bRuntimeWeaponProofSequenceRan || !RuntimeInputBridge || !RuntimeFpsCharacter)
	{
		return;
	}

	if (!FParse::Param(FCommandLine::Get(), TEXT("ArchonRunRuntimeWeaponProof")))
	{
		return;
	}

	bRuntimeWeaponProofSequenceRan = RuntimeInputBridge->RunRuntimeWeaponProofSequence(RuntimeFpsCharacter);
	if (bRuntimeWeaponProofSequenceRan)
	{
		UE_LOG(LogTemp, Display, TEXT("ArchonFactoryCanary: RuntimeWeaponProofSequenceRequested result=true"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("ArchonFactoryCanary: RuntimeWeaponProofSequenceRequested result=false"));
	}
}

void UArchonCanaryWorldSubsystem::SpawnRuntimeLenswrightCombatActors()
{
	if (RuntimeLenswrightBracewright && RuntimeLenswrightSundial)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World || !World->IsGameWorld())
	{
		return;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	if (!RuntimeLenswrightBracewright)
	{
		SpawnParameters.Name = TEXT("ArchonCanaryRuntimeLenswrightBracewright");
		RuntimeLenswrightBracewright = World->SpawnActor<AArchonLenswrightBracewrightActor>(
			AArchonLenswrightBracewrightActor::StaticClass(),
			FVector(2200.0f, 280.0f, 120.0f),
			FRotator(0.0f, 180.0f, 0.0f),
			SpawnParameters);

		if (RuntimeLenswrightBracewright)
		{
			RuntimeLenswrightBracewright->ConfigureBracewright(1, TEXT("runtime_lenswright_bracewright"));
			UE_LOG(
				LogTemp,
				Display,
				TEXT("ArchonFactoryCanary: RuntimeLenswrightBracewrightSpawned name=%s team=%d health=%.0f weapon=%d"),
				*RuntimeLenswrightBracewright->GetName(),
				RuntimeLenswrightBracewright->GetTeamId(),
				RuntimeLenswrightBracewright->GetHealth() ? RuntimeLenswrightBracewright->GetHealth()->GetMaxHealth() : 0.0f,
				RuntimeLenswrightBracewright->GetCrossbow()
					? static_cast<int32>(RuntimeLenswrightBracewright->GetCrossbow()->GetStats().WeaponClass)
					: 0);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("ArchonFactoryCanary: RuntimeLenswrightBracewrightSpawnFailed"));
		}
	}

	if (!RuntimeLenswrightSundial)
	{
		SpawnParameters.Name = TEXT("ArchonCanaryRuntimeLenswrightSundial");
		RuntimeLenswrightSundial = World->SpawnActor<AArchonLenswrightSundialOpticActor>(
			AArchonLenswrightSundialOpticActor::StaticClass(),
			FVector(2520.0f, 540.0f, 120.0f),
			FRotator(0.0f, 180.0f, 0.0f),
			SpawnParameters);

		if (RuntimeLenswrightSundial)
		{
			RuntimeLenswrightSundial->ConfigureSundial(1, TEXT("runtime_lenswright_sundial"));
			UE_LOG(
				LogTemp,
				Display,
				TEXT("ArchonFactoryCanary: RuntimeLenswrightSundialOpticSpawned name=%s visionRadius=%.0f weapon=false"),
				*RuntimeLenswrightSundial->GetName(),
				RuntimeLenswrightSundial->GetVisionRadius());
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("ArchonFactoryCanary: RuntimeLenswrightSundialOpticSpawnFailed"));
		}
	}
}

void UArchonCanaryWorldSubsystem::RunRuntimeCombatProofSequenceIfRequested()
{
	if (bRuntimeCombatProofSequenceRan || !RuntimeInputBridge || !RuntimeFpsCharacter)
	{
		return;
	}

	if (!FParse::Param(FCommandLine::Get(), TEXT("ArchonRunRuntimeCombatProof")))
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World || !World->IsGameWorld())
	{
		return;
	}

	if (RuntimeInputBridge->IsMapTableSurfaceOpen())
	{
		RuntimeInputBridge->CloseRuntimeMapTable();
	}

	SpawnRuntimeLenswrightCombatActors();

	UArchonVerdantThornsproutBow* Bow = RuntimeFpsCharacter->GetVerdantBow();
	UArchonCombatHealthComponent* PlayerHealth = RuntimeFpsCharacter->GetHealth();
	UArchonCombatHealthComponent* BracewrightHealth = RuntimeLenswrightBracewright
		? RuntimeLenswrightBracewright->GetHealth()
		: nullptr;
	UArchonLenswrightPressureBoltCrossbow* Crossbow = RuntimeLenswrightBracewright
		? RuntimeLenswrightBracewright->GetCrossbow()
		: nullptr;
	UArchonAiCombatBehaviorComponent* CombatBehavior = RuntimeLenswrightBracewright
		? RuntimeLenswrightBracewright->GetCombatBehavior()
		: nullptr;

	if (!Bow || !PlayerHealth || !BracewrightHealth || !Crossbow || !CombatBehavior || !RuntimeLenswrightSundial)
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("ArchonFactoryCanary: RuntimeCombatProofSequence completed=false reason=missing_components bow=%s playerHealth=%s targetHealth=%s crossbow=%s behavior=%s sundial=%s"),
			Bow ? TEXT("true") : TEXT("false"),
			PlayerHealth ? TEXT("true") : TEXT("false"),
			BracewrightHealth ? TEXT("true") : TEXT("false"),
			Crossbow ? TEXT("true") : TEXT("false"),
			CombatBehavior ? TEXT("true") : TEXT("false"),
			RuntimeLenswrightSundial ? TEXT("true") : TEXT("false"));
		return;
	}

	PlayerHealth->ResetProofState();
	BracewrightHealth->ResetProofState();

	const FVector PlayerLocation = RuntimeFpsCharacter->GetActorLocation();
	const FVector BracewrightLocation = RuntimeLenswrightBracewright->GetActorLocation();
	const FRotator AimRotation = (BracewrightLocation - PlayerLocation).Rotation();
	RuntimeFpsCharacter->SetActorRotation(FRotator(0.0f, AimRotation.Yaw, 0.0f));
	if (APlayerController* PlayerController = World->GetFirstPlayerController())
	{
		PlayerController->SetControlRotation(AimRotation);
	}

	FArchonAiTargetCandidate PlayerCandidate;
	PlayerCandidate.TargetId = TEXT("runtime_fps_player");
	PlayerCandidate.TargetTeamId = PlayerHealth->GetTeamId();
	PlayerCandidate.Location = PlayerLocation;
	PlayerCandidate.bIsAlive = PlayerHealth->IsAlive();

	const FArchonAiCombatDecision AiDecision = CombatBehavior->EvaluateCombat(
		BracewrightLocation,
		{ PlayerCandidate },
		BracewrightHealth->GetHealthFraction());

	UE_LOG(
		LogTemp,
		Display,
		TEXT("ArchonFactoryCanary: RuntimeLenswrightAiDecision stance=%d shouldFire=%s target=%s reason=%s"),
		static_cast<int32>(AiDecision.NewStance),
		AiDecision.bShouldFire ? TEXT("true") : TEXT("false"),
		*AiDecision.SelectedTargetId.ToString(),
		*AiDecision.Reason.ToString());

	FArchonShotPayload PlayerShot;
	bool bPlayerShotCaptured = false;
	const FDelegateHandle PlayerFireHandle = Bow->OnWeaponFired.AddLambda(
		[&PlayerShot, &bPlayerShotCaptured](const FArchonShotPayload& Shot)
		{
			PlayerShot = Shot;
			bPlayerShotCaptured = true;
		});
	const int32 InitialTargetHits = BracewrightHealth->GetTotalHitsTaken();
	const float InitialTargetHealth = BracewrightHealth->GetCurrentHealth();
	const bool bPlayerFired = RuntimeInputBridge->ApplyRuntimeWeaponInput(RuntimeFpsCharacter, true, false);
	Bow->OnWeaponFired.Remove(PlayerFireHandle);

	const bool bPlayerProjectileHit = bPlayerShotCaptured && ApplyProofProjectileHit(
		*World,
		AArchonArrowProjectile::StaticClass(),
		PlayerShot,
		RuntimeLenswrightBracewright,
		RuntimeFpsCharacter);

	const bool bTargetDamaged =
		bPlayerFired &&
		bPlayerProjectileHit &&
		BracewrightHealth->GetTotalHitsTaken() == InitialTargetHits + 1 &&
		BracewrightHealth->GetCurrentHealth() < InitialTargetHealth;

	UE_LOG(
		LogTemp,
		Display,
		TEXT("ArchonFactoryCanary: RuntimeLenswrightTargetDamaged damaged=%s playerFired=%s projectileHit=%s targetHealth=%.1f->%.1f hits=%d"),
		bTargetDamaged ? TEXT("true") : TEXT("false"),
		bPlayerFired ? TEXT("true") : TEXT("false"),
		bPlayerProjectileHit ? TEXT("true") : TEXT("false"),
		InitialTargetHealth,
		BracewrightHealth->GetCurrentHealth(),
		BracewrightHealth->GetTotalHitsTaken());

	FArchonShotPayload AiShot;
	bool bAiShotCaptured = false;
	const FDelegateHandle AiFireHandle = Crossbow->OnWeaponFired.AddLambda(
		[&AiShot, &bAiShotCaptured](const FArchonShotPayload& Shot)
		{
			AiShot = Shot;
			bAiShotCaptured = true;
		});
	const int32 InitialPlayerHits = PlayerHealth->GetTotalHitsTaken();
	const float InitialPlayerHealth = PlayerHealth->GetCurrentHealth();
	const FVector AiShotOrigin = BracewrightLocation + FVector(0.0f, 0.0f, 64.0f);
	const FVector AiShotDirection = (PlayerLocation - AiShotOrigin).GetSafeNormal();
	const bool bAiFired = AiDecision.bShouldFire && Crossbow->TryFire(AiShotOrigin, AiShotDirection);
	Crossbow->OnWeaponFired.Remove(AiFireHandle);

	const bool bAiProjectileHit = bAiShotCaptured && ApplyProofProjectileHit(
		*World,
		AArchonPressureBoltProjectile::StaticClass(),
		AiShot,
		RuntimeFpsCharacter,
		RuntimeLenswrightBracewright);

	const bool bPlayerDamaged =
		bAiFired &&
		bAiProjectileHit &&
		PlayerHealth->GetTotalHitsTaken() == InitialPlayerHits + 1 &&
		PlayerHealth->GetCurrentHealth() < InitialPlayerHealth;

	UE_LOG(
		LogTemp,
		Display,
		TEXT("ArchonFactoryCanary: RuntimeLenswrightAiFireRouted fired=%s projectileHit=%s shotCaptured=%s"),
		bAiFired ? TEXT("true") : TEXT("false"),
		bAiProjectileHit ? TEXT("true") : TEXT("false"),
		bAiShotCaptured ? TEXT("true") : TEXT("false"));
	UE_LOG(
		LogTemp,
		Display,
		TEXT("ArchonFactoryCanary: RuntimeLenswrightAiDamagedPlayer damaged=%s playerHealth=%.1f->%.1f hits=%d"),
		bPlayerDamaged ? TEXT("true") : TEXT("false"),
		InitialPlayerHealth,
		PlayerHealth->GetCurrentHealth(),
		PlayerHealth->GetTotalHitsTaken());

	bRuntimeCombatProofSequenceRan =
		RuntimeLenswrightBracewright != nullptr &&
		RuntimeLenswrightSundial != nullptr &&
		AiDecision.bShouldFire &&
		bTargetDamaged &&
		bAiFired &&
		bPlayerDamaged &&
		!RuntimeInputBridge->IsMapTableSurfaceOpen();

	if (bRuntimeCombatProofSequenceRan)
	{
		UE_LOG(
			LogTemp,
			Display,
			TEXT("ArchonFactoryCanary: RuntimeCombatProofSequence completed=true targetDamaged=%s aiShouldFire=%s aiFired=%s playerDamaged=%s mapTableOpen=%s"),
			bTargetDamaged ? TEXT("true") : TEXT("false"),
			AiDecision.bShouldFire ? TEXT("true") : TEXT("false"),
			bAiFired ? TEXT("true") : TEXT("false"),
			bPlayerDamaged ? TEXT("true") : TEXT("false"),
			RuntimeInputBridge->IsMapTableSurfaceOpen() ? TEXT("true") : TEXT("false"));
		UE_LOG(LogTemp, Display, TEXT("ArchonFactoryCanary: RuntimeCombatProofSequenceRequested result=true"));
	}
	else
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("ArchonFactoryCanary: RuntimeCombatProofSequence completed=false targetDamaged=%s aiShouldFire=%s aiFired=%s playerDamaged=%s mapTableOpen=%s"),
			bTargetDamaged ? TEXT("true") : TEXT("false"),
			AiDecision.bShouldFire ? TEXT("true") : TEXT("false"),
			bAiFired ? TEXT("true") : TEXT("false"),
			bPlayerDamaged ? TEXT("true") : TEXT("false"),
			RuntimeInputBridge->IsMapTableSurfaceOpen() ? TEXT("true") : TEXT("false"));
		UE_LOG(LogTemp, Error, TEXT("ArchonFactoryCanary: RuntimeCombatProofSequenceRequested result=false"));
	}
}

void UArchonCanaryWorldSubsystem::HandleRuntimeFpsDeath(const FArchonDamageApplicationResult& DamageResult)
{
	if (!RuntimeRespawnState || !RuntimeFpsCharacter)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World || !World->IsGameWorld())
	{
		return;
	}

	NotifyPlaytestEvent(TEXT("player_death"));

	UArchonCombatHealthComponent* Health = RuntimeFpsCharacter->GetHealth();
	if (Health)
	{
		Health->OnDeath.RemoveAll(this);
	}

	const FVector DeathLocation = RuntimeFpsCharacter->GetActorLocation();
	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParameters.Name = MakeUniqueObjectName(World, AArchonRespawnObserverPawn::StaticClass(), TEXT("ArchonRuntimeRespawnObserver"));

	RuntimeRespawnObserver = World->SpawnActor<AArchonRespawnObserverPawn>(
		AArchonRespawnObserverPawn::StaticClass(),
		DeathLocation + FVector(-200.0f, 0.0f, 200.0f),
		FRotator::ZeroRotator,
		SpawnParameters);
	if (RuntimeRespawnObserver)
	{
		RuntimeRespawnObserver->ConfigureObserverFromDeathLocation(DeathLocation);
	}

	APlayerController* PlayerController = World->GetFirstPlayerController();
	if (PlayerController && RuntimeRespawnObserver)
	{
		PlayerController->Possess(RuntimeRespawnObserver);
		PlayerController->SetInputMode(FInputModeGameOnly());
		PlayerController->SetShowMouseCursor(false);
	}

	RuntimeFpsCharacter->SetActorHiddenInGame(true);
	RuntimeFpsCharacter->SetActorEnableCollision(false);

	const bool bHandled = RuntimeRespawnState->HandlePlayerDeath(1, INDEX_NONE);
	const bool bObserverPossessed =
		PlayerController &&
		RuntimeRespawnObserver &&
		PlayerController->GetPawn() == RuntimeRespawnObserver;

	UE_LOG(
		LogTemp,
		Display,
		TEXT("ArchonFactoryCanary: RuntimeRespawnDeathHandled lifeState=%s observerPossessed=%s handled=%s causedDeath=%s"),
		RuntimeRespawnState->GetLifeState() == EArchonLifeState::Dead ? TEXT("Dead") : TEXT("Other"),
		bObserverPossessed ? TEXT("true") : TEXT("false"),
		bHandled ? TEXT("true") : TEXT("false"),
		DamageResult.bCausedDeath ? TEXT("true") : TEXT("false"));
}

void UArchonCanaryWorldSubsystem::HandleRuntimeRespawnReady(const FArchonRespawnSpawnPoint& SpawnPoint)
{
	UWorld* World = GetWorld();
	if (!World || !World->IsGameWorld() || !RuntimeRespawnState)
	{
		return;
	}

	APlayerController* PlayerController = World->GetFirstPlayerController();
	if (!PlayerController)
	{
		return;
	}

	AArchonCanaryFpsCharacter* PreviousFpsCharacter = RuntimeFpsCharacter;

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParameters.Name = MakeUniqueObjectName(World, AArchonCanaryFpsCharacter::StaticClass(), TEXT("ArchonCanaryRuntimeFpsCharacterRespawned"));

	AArchonCanaryFpsCharacter* RespawnedCharacter = World->SpawnActor<AArchonCanaryFpsCharacter>(
		AArchonCanaryFpsCharacter::StaticClass(),
		SpawnPoint.Location,
		SpawnPoint.Rotation,
		SpawnParameters);

	if (!RespawnedCharacter)
	{
		UE_LOG(LogTemp, Error, TEXT("ArchonFactoryCanary: RuntimeRespawnReady state=Respawning newPawn=false selected=%s"), *SpawnPoint.SpawnPointId.ToString());
		return;
	}

	RuntimeFpsCharacter = RespawnedCharacter;
	PlayerController->Possess(RuntimeFpsCharacter);
	PlayerController->SetInputMode(FInputModeGameOnly());
	PlayerController->SetShowMouseCursor(false);
	BindRuntimeFpsDeath(RuntimeFpsCharacter);
	if (RuntimeInputBridge)
	{
		RuntimeInputBridge->SetRespawnStateComponent(RuntimeRespawnState);
	}

	const bool bMarkedComplete = RuntimeRespawnState->MarkRespawnComplete();

	if (RuntimeRespawnObserver)
	{
		RuntimeRespawnObserver->Destroy();
		RuntimeRespawnObserver = nullptr;
	}

	if (PreviousFpsCharacter && PreviousFpsCharacter != RuntimeFpsCharacter)
	{
		PreviousFpsCharacter->Destroy();
	}

	UE_LOG(
		LogTemp,
		Display,
		TEXT("ArchonFactoryCanary: RuntimeRespawnReady state=%s newPawn=true selected=%s markComplete=%s"),
		RuntimeRespawnState->GetLifeState() == EArchonLifeState::Alive ? TEXT("Alive") : TEXT("Other"),
		*SpawnPoint.SpawnPointId.ToString(),
		bMarkedComplete ? TEXT("true") : TEXT("false"));

	NotifyPlaytestEvent(TEXT("player_respawn"));
}

void UArchonCanaryWorldSubsystem::RunRuntimeRespawnProofSequenceIfRequested()
{
	if (bRuntimeRespawnProofSequenceRan || !RuntimeInputBridge || !RuntimeFpsCharacter || !RuntimeRespawnState)
	{
		return;
	}

	if (!FParse::Param(FCommandLine::Get(), TEXT("ArchonRunRuntimeRespawnProof")))
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World || !World->IsGameWorld())
	{
		return;
	}

	if (RuntimeInputBridge->IsMapTableSurfaceOpen())
	{
		RuntimeInputBridge->CloseRuntimeMapTable();
	}

	RegisterRuntimeRespawnSpawnPoints();

	APlayerController* PlayerController = World->GetFirstPlayerController();
	AArchonCanaryFpsCharacter* InitialFpsCharacter = RuntimeFpsCharacter;
	UArchonCombatHealthComponent* PlayerHealth = RuntimeFpsCharacter->GetHealth();
	if (!PlayerHealth)
	{
		UE_LOG(LogTemp, Error, TEXT("ArchonFactoryCanary: RuntimeRespawnProofSequence completed=false reason=missing_player_health"));
		return;
	}

	PlayerHealth->ResetProofState();
	const FArchonDamageApplicationResult LethalDamageResult = PlayerHealth->ApplyDirectDamage(
		999.0f,
		EArchonDamageType::LenswrightPressureBolt,
		1,
		INDEX_NONE);

	const bool bDeathHandled = RuntimeRespawnState->GetLifeState() == EArchonLifeState::Dead;
	const bool bObserverPossessed =
		PlayerController &&
		RuntimeRespawnObserver &&
		PlayerController->GetPawn() == RuntimeRespawnObserver;

	const bool bDeadStateOpened = RuntimeInputBridge->PreviewRuntimeMapTable();
	UE_LOG(
		LogTemp,
		Display,
		TEXT("ArchonFactoryCanary: CommandWhileWaitDeadStateOpened opened=%s lifeState=%s"),
		bDeadStateOpened ? TEXT("true") : TEXT("false"),
		RuntimeRespawnState->GetLifeState() == EArchonLifeState::Dead ? TEXT("Dead") : TEXT("Other"));

	const int32 PreviousDeathCommands = RuntimeInputBridge->GetCommandsIssuedDuringDeath();
	const bool bDeathOrderSubmitted = bDeadStateOpened &&
		RuntimeInputBridge->SubmitRuntimeMapTableWidgetMoveOrderAt(
			FVector2D(0.80f, 0.50f),
			TEXT("dead_state_rally"));
	const int32 DeathCommandSequence = RuntimeMapTable ? RuntimeMapTable->GetCurrentCommandSequence() : 0;
	const bool bDeathOrderRouted =
		bDeathOrderSubmitted &&
		RuntimeInputBridge->GetCommandsIssuedDuringDeath() == PreviousDeathCommands + 1 &&
		RuntimeRtsSquad &&
		RuntimeRtsSquad->GetOrderState() == EArchonCanaryRtsSquadOrderState::Moving &&
		RuntimeRtsSquad->GetLastAppliedCommandSequence() == DeathCommandSequence;

	UE_LOG(
		LogTemp,
		Display,
		TEXT("ArchonFactoryCanary: CommandWhileWaitOrderSubmitted submitted=%s commandsDuringDeath=%d sequence=%d"),
		bDeathOrderRouted ? TEXT("true") : TEXT("false"),
		RuntimeInputBridge->GetCommandsIssuedDuringDeath(),
		DeathCommandSequence);

	const float SecondsToRespawn = RuntimeRespawnState->GetState().SecondsUntilRespawn + 0.05f;
	RuntimeRespawnState->TickComponent(SecondsToRespawn, LEVELTICK_All, nullptr);

	const bool bRespawnReady =
		RuntimeRespawnState->GetLifeState() == EArchonLifeState::Alive &&
		RuntimeFpsCharacter &&
		RuntimeFpsCharacter != InitialFpsCharacter &&
		PlayerController &&
		PlayerController->GetPawn() == RuntimeFpsCharacter;

	FArchonRtsCommandIntent LastIntent;
	if (RuntimeMapTable && RuntimeMapTable->GetTeamState())
	{
		LastIntent = RuntimeMapTable->GetTeamState()->GetLastAcceptedCommandIntent();
	}
	const bool bOrderSurvivedRespawn =
		bRespawnReady &&
		RuntimeMapTable &&
		RuntimeMapTable->GetCurrentCommandSequence() == DeathCommandSequence &&
		RuntimeRtsSquad &&
		RuntimeRtsSquad->GetLastAppliedCommandSequence() == DeathCommandSequence &&
		RuntimeRtsSquad->GetOrderState() == EArchonCanaryRtsSquadOrderState::Moving &&
		LastIntent.TargetId == TEXT("dead_state_rally");

	if (RuntimeInputBridge->IsMapTableSurfaceOpen())
	{
		RuntimeInputBridge->CloseRuntimeMapTable();
	}

	UE_LOG(
		LogTemp,
		Display,
		TEXT("ArchonFactoryCanary: CommandWhileWaitOrderSurvivedRespawn survived=%s sequence=%d"),
		bOrderSurvivedRespawn ? TEXT("true") : TEXT("false"),
		DeathCommandSequence);

	bRuntimeRespawnProofSequenceRan =
		LethalDamageResult.bCausedDeath &&
		bDeathHandled &&
		bObserverPossessed &&
		bDeadStateOpened &&
		bDeathOrderRouted &&
		bRespawnReady &&
		bOrderSurvivedRespawn;

	if (bRuntimeRespawnProofSequenceRan)
	{
		UE_LOG(
			LogTemp,
			Display,
			TEXT("ArchonFactoryCanary: RuntimeRespawnProofSequence completed=true deathHandled=%s observerPossessed=%s respawned=%s orderSurvived=%s"),
			bDeathHandled ? TEXT("true") : TEXT("false"),
			bObserverPossessed ? TEXT("true") : TEXT("false"),
			bRespawnReady ? TEXT("true") : TEXT("false"),
			bOrderSurvivedRespawn ? TEXT("true") : TEXT("false"));
		UE_LOG(LogTemp, Display, TEXT("ArchonFactoryCanary: RuntimeRespawnProofSequenceRequested result=true"));
	}
	else
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("ArchonFactoryCanary: RuntimeRespawnProofSequence completed=false causedDeath=%s deathHandled=%s observerPossessed=%s opened=%s order=%s respawned=%s survived=%s"),
			LethalDamageResult.bCausedDeath ? TEXT("true") : TEXT("false"),
			bDeathHandled ? TEXT("true") : TEXT("false"),
			bObserverPossessed ? TEXT("true") : TEXT("false"),
			bDeadStateOpened ? TEXT("true") : TEXT("false"),
			bDeathOrderRouted ? TEXT("true") : TEXT("false"),
			bRespawnReady ? TEXT("true") : TEXT("false"),
			bOrderSurvivedRespawn ? TEXT("true") : TEXT("false"));
		UE_LOG(LogTemp, Error, TEXT("ArchonFactoryCanary: RuntimeRespawnProofSequenceRequested result=false"));
	}
}

void UArchonCanaryWorldSubsystem::SpawnBlockoutActorsIfRequested()
{
	if (bBlockoutSpawned)
	{
		return;
	}
	if (!FParse::Param(FCommandLine::Get(), TEXT("ArchonRunFirst60SecondsProof")))
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World || !World->IsGameWorld())
	{
		return;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// Spawn relative to the existing FirstPerson PlayerStart so the actors land
	// in front of the camera (not at world origin where they'd be invisible).
	// Anchor: a few meters ahead of where the FirstPerson template drops the pawn.
	// On the valley map that template anchor sits inside the Central Splitroot,
	// so the proof cluster relocates to an annex north of the Verdant base,
	// outside the playtest hero-shot frustum — the arc drives these actors
	// by reference, not by position.
	const FVector Anchor = bValleyActive
		? FVector(-17500.0, 4500.0, 100.0)
		: FVector(800.0, 0.0, 100.0);

	BlockoutVerdantOutpost = World->SpawnActor<AArchonVerdantOutpostActor>(
		AArchonVerdantOutpostActor::StaticClass(),
		Anchor + FVector(0.0, -500.0, 0.0),
		FRotator::ZeroRotator,
		Params);

	BlockoutSplitrootTree = World->SpawnActor<AArchonSplitrootTreeCentralActor>(
		AArchonSplitrootTreeCentralActor::StaticClass(),
		Anchor + FVector(1500.0, 0.0, 0.0),
		FRotator::ZeroRotator,
		Params);

	BlockoutLenswrightGhost = World->SpawnActor<AArchonLenswrightOutpostGhostActor>(
		AArchonLenswrightOutpostGhostActor::StaticClass(),
		Anchor + FVector(800.0, 1000.0, 0.0),
		FRotator::ZeroRotator,
		Params);

	BlockoutCoverStones.Reset();
	for (int32 i = 1; i <= 12; ++i)
	{
		const double X = 120.0 * static_cast<double>(i);
		AArchonCoverStoneRootVaultActor* Stone = World->SpawnActor<AArchonCoverStoneRootVaultActor>(
			AArchonCoverStoneRootVaultActor::StaticClass(),
			Anchor + FVector(X, 200.0, 0.0),
			FRotator::ZeroRotator,
			Params);
		if (Stone)
		{
			BlockoutCoverStones.Add(Stone);
		}
	}

	UE_LOG(LogTemp, Display, TEXT("ArchonFactoryCanary: BlockoutActor name=VerdantOutpost count=%d"), BlockoutVerdantOutpost ? 1 : 0);
	UE_LOG(LogTemp, Display, TEXT("ArchonFactoryCanary: BlockoutActor name=SplitrootTreeCentral count=%d"), BlockoutSplitrootTree ? 1 : 0);
	UE_LOG(
		LogTemp,
		Display,
		TEXT("ArchonFactoryCanary: BlockoutActor name=LenswrightOutpostGhost count=%d ghost=%s"),
		BlockoutLenswrightGhost ? 1 : 0,
		(BlockoutLenswrightGhost && BlockoutLenswrightGhost->IsGhost()) ? TEXT("true") : TEXT("false"));
	UE_LOG(LogTemp, Display, TEXT("ArchonFactoryCanary: BlockoutActor name=CoverStoneRootVault count=%d"), BlockoutCoverStones.Num());

	bBlockoutSpawned = true;
}

void UArchonCanaryWorldSubsystem::SpawnSplitrootValleyIfRequested()
{
	if (bValleyActive)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World || !World->IsGameWorld())
	{
		return;
	}
	if (!UArchonLaunchFlagLibrary::IsLaunchFlagActive(World, TEXT("ArchonSplitrootValley")))
	{
		return;
	}

	SpawnSplitrootValleyInternal(/*bCosmeticOnly=*/false);
}

void UArchonCanaryWorldSubsystem::SpawnSplitrootValleyForClient()
{
	if (bValleyActive)
	{
		return;
	}
	UWorld* World = GetWorld();
	if (!World || !World->IsGameWorld() || World->GetNetMode() != NM_Client)
	{
		return;
	}
	SpawnSplitrootValleyInternal(/*bCosmeticOnly=*/true);

	// Clients read the same match truth through the replicated snapshot.
	ShowMatchHudIfNeeded();
}

void UArchonCanaryWorldSubsystem::SpawnSplitrootValleyInternal(bool bCosmeticOnly)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const TArray<FArchonValleyPlacement> Layout = UArchonSplitrootValleyLayoutLibrary::BuildSplitrootValleyLayout();

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	int32 BlocksSpawned = 0;
	int32 StonesSpawned = 0;
	int32 BasesSpawned = 0;
	int32 DecorTreesSpawned = 0;
	int32 DecorRocksSpawned = 0;
	int32 DecorFoliageSpawned = 0;
	for (const FArchonValleyPlacement& Placement : Layout)
	{
		switch (Placement.Piece)
		{
		case EArchonValleyPiece::GroundPlane:
		case EArchonValleyPiece::RidgeSlab:
		case EArchonValleyPiece::CentralTreeTrunk:
		case EArchonValleyPiece::CentralTreeCanopy:
		case EArchonValleyPiece::RootButtress:
		case EArchonValleyPiece::CoverStone:
		case EArchonValleyPiece::KinwildCamp:
		case EArchonValleyPiece::ResourceSite:
		{
			Params.Name = MakeUniqueObjectName(World, AArchonValleyBlockActor::StaticClass(), TEXT("ArchonValleyBlock"));
			AArchonValleyBlockActor* BlockActor = World->SpawnActor<AArchonValleyBlockActor>(
				AArchonValleyBlockActor::StaticClass(),
				Placement.Location,
				Placement.Rotation,
				Params);
			if (BlockActor)
			{
				BlockActor->ConfigureBlock(Placement);
				ValleyActors.Add(BlockActor);
				++BlocksSpawned;
				if (Placement.Piece == EArchonValleyPiece::CoverStone)
				{
					++StonesSpawned;
				}
				if (Placement.Piece == EArchonValleyPiece::KinwildCamp)
				{
					++BasesSpawned;
				}
			}
			break;
		}
		case EArchonValleyPiece::BaseCore:
		{
			if (bCosmeticOnly)
			{
				// Gameplay objective — the server's replicated core is the
				// only one a client may see.
				break;
			}
			Params.Name = MakeUniqueObjectName(World, AArchonBaseCoreActor::StaticClass(), TEXT("ArchonValleyBaseCore"));
			AArchonBaseCoreActor* Core = World->SpawnActor<AArchonBaseCoreActor>(
				AArchonBaseCoreActor::StaticClass(),
				Placement.Location,
				Placement.Rotation,
				Params);
			if (Core)
			{
				Core->ConfigureCore(Placement, BuildArchonMatchConfig(World).CoreMaxHealth);
				ValleyBaseCores.Add(Core);
				ValleyActors.Add(Core);
				++BlocksSpawned;
			}
			break;
		}
		case EArchonValleyPiece::DecorTree:
		case EArchonValleyPiece::DecorRock:
		case EArchonValleyPiece::DecorFoliage:
		{
			// Cosmetic dressing from imported Fab kits. Keep these meshes
			// non-blocking and cluster-specific so they read like authored
			// terrain, not a repeated placeholder scatter.
			const uint32 Hash = GetTypeHash(Placement.PieceId);
			const FString PieceIdString = Placement.PieceId.ToString();
			const bool bTree = Placement.Piece == EArchonValleyPiece::DecorTree;
			const bool bFoliage = Placement.Piece == EArchonValleyPiece::DecorFoliage;
			TArray<const TCHAR*> MeshPaths;
			if (bFoliage)
			{
				MeshPaths = {
					TEXT("/Game/StandIns/Nature/grass_large.grass_large"),
					TEXT("/Game/StandIns/Nature/grass_leafsLarge.grass_leafsLarge"),
					TEXT("/Game/StandIns/Nature/flower_yellowA.flower_yellowA"),
					TEXT("/Game/StandIns/Nature/flower_redA.flower_redA"),
					TEXT("/Game/StandIns/Nature/flower_purpleA.flower_purpleA"),
					TEXT("/Game/StandIns/Nature/plant_bush.plant_bush"),
					TEXT("/Game/StandIns/Nature/plant_bushLarge.plant_bushLarge"),
					TEXT("/Game/StandIns/Nature/plant_bushSmall.plant_bushSmall")
				};
			}
			else if (bTree)
			{
				MeshPaths = {
					TEXT("/Game/Fab/English_Oak/3_english_oak_set_quercus_robur_updated/StaticMeshes/Object_2.Object_2"),
					TEXT("/Game/Fab/English_Oak/3_english_oak_set_quercus_robur_updated/StaticMeshes/Object_3.Object_3"),
					TEXT("/Game/Fab/English_Oak/3_english_oak_set_quercus_robur_updated/StaticMeshes/Object_4.Object_4"),
					TEXT("/Game/Fab/English_Oak/3_english_oak_set_quercus_robur_updated/StaticMeshes/Object_5.Object_5"),
					TEXT("/Game/Fab/English_Oak/3_english_oak_set_quercus_robur_updated/StaticMeshes/Object_6.Object_6"),
					TEXT("/Game/Fab/Library/Old_red_wood_tree_stump/old_red_stump.old_red_stump"),
					TEXT("/Game/Fab/Library/Old_green_stump/old_green_stump.old_green_stump")
				};
			}
			else if (PieceIdString.StartsWith(TEXT("north_ice_shelf")))
			{
				MeshPaths = {
					TEXT("/Game/Fab/Library/Wall_of_ice/wall_of_ice/StaticMeshes/Object_7.Object_7"),
					TEXT("/Game/Fab/Library/Wall_of_ice/wall_of_ice/StaticMeshes/Object_8.Object_8"),
					TEXT("/Game/Fab/Library/Wall_of_ice/wall_of_ice/StaticMeshes/Object_10.Object_10"),
					TEXT("/Game/Fab/Library/Wall_of_ice/wall_of_ice/StaticMeshes/Object_12.Object_12"),
					TEXT("/Game/Fab/Library/Rock_terrain_with_light_snow/rock_terrain_with_light_snow/StaticMeshes/Object_10.Object_10")
				};
			}
			else if (PieceIdString.StartsWith(TEXT("lens_ruin_stones")))
			{
				MeshPaths = {
					TEXT("/Game/Fab/Castle_Rocca_Calascio/castle_of_rocca_calascio/StaticMeshes/Object_4.Object_4"),
					TEXT("/Game/Fab/Castle_Rocca_Calascio/castle_of_rocca_calascio/StaticMeshes/Object_5.Object_5"),
					TEXT("/Game/Fab/Castle_Rocca_Calascio/castle_of_rocca_calascio/StaticMeshes/Object_6.Object_6"),
					TEXT("/Game/Fab/Castle_Rocca_Calascio/castle_of_rocca_calascio/StaticMeshes/Object_11.Object_11"),
					TEXT("/Game/Fab/Castle_Rocca_Calascio/castle_of_rocca_calascio/StaticMeshes/Object_13.Object_13"),
					TEXT("/Game/Fab/Library/Rock_terrain_with_light_snow/rock_terrain_with_light_snow/StaticMeshes/Object_6.Object_6")
				};
			}
			else
			{
				MeshPaths = {
					TEXT("/Game/Fab/Library/Rock_terrain_with_light_snow/rock_terrain_with_light_snow/StaticMeshes/Object_2.Object_2"),
					TEXT("/Game/Fab/Library/Rock_terrain_with_light_snow/rock_terrain_with_light_snow/StaticMeshes/Object_3.Object_3"),
					TEXT("/Game/Fab/Library/Rock_terrain_with_light_snow/rock_terrain_with_light_snow/StaticMeshes/Object_4.Object_4"),
					TEXT("/Game/Fab/Library/Rock_terrain_with_light_snow/rock_terrain_with_light_snow/StaticMeshes/Object_6.Object_6"),
					TEXT("/Game/Fab/Castle_Rocca_Calascio/castle_of_rocca_calascio/StaticMeshes/Object_12.Object_12"),
					TEXT("/Game/Fab/Castle_Rocca_Calascio/castle_of_rocca_calascio/StaticMeshes/Object_14.Object_14")
				};
			}
			const int32 MeshIndex = static_cast<int32>(Hash % static_cast<uint32>(MeshPaths.Num()));
			const TCHAR* MeshPath = MeshPaths[MeshIndex];
			UStaticMesh* DecorMesh = LoadObject<UStaticMesh>(nullptr, MeshPath);
			if (!DecorMesh)
			{
				break;
			}
			Params.Name = NAME_None;
			AStaticMeshActor* Decor = World->SpawnActor<AStaticMeshActor>(
				AStaticMeshActor::StaticClass(), Placement.Location, Placement.Rotation, Params);
			if (Decor)
			{
				Decor->SetMobility(EComponentMobility::Movable);
				Decor->GetStaticMeshComponent()->SetStaticMesh(DecorMesh);
				const double PlayerHeightUnits = 192.0;
				const bool bNorthIceShelf = Placement.PieceId.ToString().StartsWith(TEXT("north_ice_shelf"));
				const FVector DesiredDimensions = Placement.Scale * (bFoliage ? 160.0 : bTree ? 1400.0 : bNorthIceShelf ? 1900.0 : 900.0);
				const FVector MeshDimensions = DecorMesh->GetBounds().BoxExtent * 2.0;
				const double BoxFitScale = FMath::Clamp(
					FMath::Min3(
						MeshDimensions.X > 1.0 ? DesiredDimensions.X / MeshDimensions.X : 1.0,
						MeshDimensions.Y > 1.0 ? DesiredDimensions.Y / MeshDimensions.Y : 1.0,
						MeshDimensions.Z > 1.0 ? DesiredDimensions.Z / MeshDimensions.Z : 1.0),
					0.05,
					80.0);
				const double MinimumVisualHeight = bFoliage ? 0.0 : bTree ? PlayerHeightUnits * 2.0 : bNorthIceShelf ? PlayerHeightUnits * 5.0 : PlayerHeightUnits * 1.8;
				const double HeightScaleFloor = MeshDimensions.Z > 1.0 ? MinimumVisualHeight / MeshDimensions.Z : BoxFitScale;
				const double FitScale = FMath::Clamp(FMath::Max(BoxFitScale, HeightScaleFloor), 0.05, 80.0);
				Decor->SetActorScale3D(FVector(FitScale));
				// Decorative dressing must never eat arrows, feet, or bot paths.
				Decor->GetStaticMeshComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				UE_LOG(
					LogTemp,
					Display,
					TEXT("ArchonFactoryCanary: ValleyDecorScale id=%s mesh=%s playerHeight=%.0f desired=%s meshBounds=%s fit=%.3f collision=none"),
					*Placement.PieceId.ToString(),
					MeshPath,
					PlayerHeightUnits,
					*DesiredDimensions.ToCompactString(),
					*MeshDimensions.ToCompactString(),
					FitScale);
				ValleyActors.Add(Decor);
				++BlocksSpawned;
				if (Placement.Piece == EArchonValleyPiece::DecorTree)
				{
					++DecorTreesSpawned;
				}
				else if (Placement.Piece == EArchonValleyPiece::DecorRock)
				{
					++DecorRocksSpawned;
				}
				else if (Placement.Piece == EArchonValleyPiece::DecorFoliage)
				{
					++DecorFoliageSpawned;
				}
			}
			break;
		}
		case EArchonValleyPiece::DefenseTower:
		{
			if (bCosmeticOnly)
			{
				// Gameplay actor — replicated from the server.
				break;
			}
			Params.Name = MakeUniqueObjectName(World, AArchonBaseDefenseTowerActor::StaticClass(), TEXT("ArchonValleyDefenseTower"));
			AArchonBaseDefenseTowerActor* Tower = World->SpawnActor<AArchonBaseDefenseTowerActor>(
				AArchonBaseDefenseTowerActor::StaticClass(),
				Placement.Location,
				Placement.Rotation,
				Params);
			if (Tower)
			{
				Tower->ConfigureTower(Placement);
				ValleyActors.Add(Tower);
				++BlocksSpawned;
			}
			break;
		}
		case EArchonValleyPiece::VerdantOutpost:
		{
			Params.Name = MakeUniqueObjectName(World, AArchonVerdantOutpostActor::StaticClass(), TEXT("ArchonValleyVerdantOutpost"));
			if (AActor* Outpost = World->SpawnActor<AArchonVerdantOutpostActor>(
				AArchonVerdantOutpostActor::StaticClass(), Placement.Location, Placement.Rotation, Params))
			{
				ValleyActors.Add(Outpost);
				++BasesSpawned;
			}
			break;
		}
		case EArchonValleyPiece::LenswrightOutpost:
		{
			Params.Name = MakeUniqueObjectName(World, AArchonLenswrightOutpostGhostActor::StaticClass(), TEXT("ArchonValleyLenswrightOutpost"));
			if (AActor* Outpost = World->SpawnActor<AArchonLenswrightOutpostGhostActor>(
				AArchonLenswrightOutpostGhostActor::StaticClass(), Placement.Location, Placement.Rotation, Params))
			{
				ValleyActors.Add(Outpost);
				++BasesSpawned;
			}
			break;
		}
		case EArchonValleyPiece::MapTable:
			ValleyMapTableLocation = Placement.Location;
			break;
		case EArchonValleyPiece::PlayerSpawn:
			ValleyPlayerSpawnLocation = Placement.Location;
			ValleyPlayerSpawnRotation = Placement.Rotation;
			break;
		case EArchonValleyPiece::SquadSpawn:
			ValleySquadSpawnLocation = Placement.Location;
			break;
		default:
			break;
		}
	}

	SpawnValleyLightingIfNeeded();

	bValleyActive = true;
	UE_LOG(
		LogTemp,
		Display,
		TEXT("ArchonFactoryCanary: SplitrootValleySpawned placements=%d blocks=%d stones=%d bases=%d cores=%d cosmeticOnly=%s"),
		Layout.Num(),
		BlocksSpawned,
		StonesSpawned,
		BasesSpawned,
		ValleyBaseCores.Num(),
		bCosmeticOnly ? TEXT("true") : TEXT("false"));
	UE_LOG(
		LogTemp,
		Display,
		TEXT("ArchonFactoryCanary: ValleyMapPass id=quarry_lens_reseat_pass_20260611 routeClearance=1500 hotZoneClearance=1800 decorTrees=%d decorRocks=%d decorFoliage=%d resourceCollision=nonblocking"),
		DecorTreesSpawned,
		DecorRocksSpawned,
		DecorFoliageSpawned);

	if (bCosmeticOnly)
	{
		return;
	}

	// Tells joining network clients to rebuild the cosmetic valley locally.
	Params.Name = TEXT("ArchonValleyNetBeacon");
	World->SpawnActor<AArchonValleyNetBeacon>(
		AArchonValleyNetBeacon::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, Params);

	SpawnMatchLoopIfRequested();
	SpawnBotMatchIfRequested();
}

void UArchonCanaryWorldSubsystem::SpawnMatchLoopIfRequested()
{
	if (MatchState || !bValleyActive)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World || !World->IsGameWorld())
	{
		return;
	}
	if (!UArchonLaunchFlagLibrary::IsLaunchFlagActive(World, TEXT("ArchonMatchLoop")))
	{
		return;
	}

	AArchonBaseCoreActor* TeamACore = nullptr;
	AArchonBaseCoreActor* TeamBCore = nullptr;
	for (AArchonBaseCoreActor* Core : ValleyBaseCores)
	{
		if (!Core)
		{
			continue;
		}
		if (Core->GetTeamId() == 0)
		{
			TeamACore = Core;
		}
		else if (Core->GetTeamId() == 1)
		{
			TeamBCore = Core;
		}
	}
	if (!TeamACore || !TeamBCore)
	{
		UE_LOG(LogTemp, Error, TEXT("ArchonFactoryCanary: MatchLoopSpawnFailed reason=missing_base_core"));
		return;
	}

	const TArray<FArchonValleyPlacement> Layout = UArchonSplitrootValleyLayoutLibrary::BuildSplitrootValleyLayout();
	const TArray<FArchonValleyPlacement> Sites =
		UArchonSplitrootValleyLayoutLibrary::FilterPlacementsByPiece(Layout, EArchonValleyPiece::ResourceSite);

	FActorSpawnParameters Params;
	Params.Name = TEXT("ArchonMatchState");
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	MatchState = World->SpawnActor<AArchonMatchStateActor>(
		AArchonMatchStateActor::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, Params);
	if (!MatchState)
	{
		UE_LOG(LogTemp, Error, TEXT("ArchonFactoryCanary: MatchLoopSpawnFailed reason=spawn_failed"));
		return;
	}

	MatchState->InitializeMatch(
		BuildArchonMatchConfig(World),
		Sites,
		TeamACore,
		TeamBCore,
		UArchonLaunchFlagLibrary::IsLaunchFlagActive(World, TEXT("ArchonMatchProof")));

	// Income loop: the match state sells reinforcements; this subsystem
	// is the only thing that knows how to field the bodies.
	MatchState->OnReinforcementPurchased.AddUObject(
		this, &UArchonCanaryWorldSubsystem::HandleReinforcementPurchased);
	MatchState->OnTechBuildingPurchased.AddUObject(
		this, &UArchonCanaryWorldSubsystem::HandleTechBuildingPurchased);

	ShowMatchHudIfNeeded();
}

void UArchonCanaryWorldSubsystem::SpawnBotMatchIfRequested()
{
	UWorld* World = GetWorld();
	if (!World || !World->IsGameWorld() || BotPlayers.Num() > 0 || World->GetNetMode() == NM_Client)
	{
		return;
	}

	if (!UArchonLaunchFlagLibrary::IsLaunchFlagActive(World, TEXT("ArchonBotMatch")))
	{
		return;
	}

	// Both teams' cores must exist (the match loop found them already).
	AArchonBaseCoreActor* TeamCores[2] = { nullptr, nullptr };
	for (AArchonBaseCoreActor* Core : ValleyBaseCores)
	{
		if (Core && Core->GetTeamId() >= 0 && Core->GetTeamId() <= 1)
		{
			TeamCores[Core->GetTeamId()] = Core;
		}
	}
	if (!TeamCores[0] || !TeamCores[1])
	{
		UE_LOG(LogTemp, Error, TEXT("ArchonFactoryCanary: BotMatchSpawnFailed reason=missing_cores"));
		return;
	}

	int32 BotsPerTeam = 8;
	FParse::Value(FCommandLine::Get(), TEXT("ArchonBotCountPerTeam="), BotsPerTeam);
	BotsPerTeam = FMath::Clamp(BotsPerTeam, 1, 8);

	// Lane spread: each bot routes through an assigned resource site
	// before the enemy core (first watched 8v8 was a single-lane
	// meatgrinder; sites also get captured by presence on the way).
	const TArray<FArchonValleyPlacement> BotLayout = UArchonSplitrootValleyLayoutLibrary::BuildSplitrootValleyLayout();
	const TArray<FArchonValleyPlacement> RouteSites =
		UArchonSplitrootValleyLayoutLibrary::FilterPlacementsByPiece(BotLayout, EArchonValleyPiece::ResourceSite);
	const FArchonBotStrategyTuning StrategyTuning = UArchonBotStrategyTuningLibrary::GetSplitrootBotStrategyTuning();

	for (int32 Team = 0; Team < 2; ++Team)
	{
		for (int32 Slot = 0; Slot < BotsPerTeam; ++Slot)
		{
			SpawnFieldedBot(Team, TeamCores[Team], TeamCores[1 - Team], RouteSites, StrategyTuning);
		}
	}

	UE_LOG(
		LogTemp,
		Display,
		TEXT("ArchonFactoryCanary: BotMatchSpawned perTeam=%d total=%d"),
		BotsPerTeam,
		BotPlayers.Num());

	ShowSpectatorBannerIfNeeded();
}

AArchonCanaryFpsCharacter* UArchonCanaryWorldSubsystem::SpawnFieldedBot(
	int32 Team,
	AArchonBaseCoreActor* OwnCore,
	AArchonBaseCoreActor* EnemyCore,
	const TArray<FArchonValleyPlacement>& RouteSites,
	const FArchonBotStrategyTuning& StrategyTuning)
{
	UWorld* World = GetWorld();
	if (!World || !OwnCore || Team < 0 || Team > 1)
	{
		return nullptr;
	}

	// Spawn ring around the team's own core, on the ground.
	const FVector CoreLocation = OwnCore->GetActorLocation();
	const float Angle = (2.0f * PI * NextBotIndex) / 8.0f;
	const FVector SpawnLocation(
		CoreLocation.X + FMath::Cos(Angle) * 900.0f,
		CoreLocation.Y + FMath::Sin(Angle) * 900.0f,
		250.0f);

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	AArchonCanaryFpsCharacter* Bot = World->SpawnActor<AArchonCanaryFpsCharacter>(
		AArchonCanaryFpsCharacter::StaticClass(),
		SpawnLocation,
		FRotator::ZeroRotator,
		Params);
	if (!Bot)
	{
		UE_LOG(LogTemp, Error, TEXT("ArchonFactoryCanary: BotSpawnFailed team=%d index=%d"), Team, NextBotIndex);
		return nullptr;
	}

	// A native bot controller provides UE perception/team identity while
	// the brain remains the SPLITROOT decision shell.
	Bot->AIControllerClass = AArchonBotAIController::StaticClass();
	Bot->SpawnDefaultController();
	AArchonBotAIController* BotController = Cast<AArchonBotAIController>(Bot->GetController());
	if (BotController)
	{
		BotController->ConfigureBotTeamWithTuning(Team, NextBotIndex, StrategyTuning);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("ArchonFactoryCanary: BotAIControllerMissing bot=%d team=%d"), NextBotIndex, Team);
	}

	UArchonBotBrainComponent* Brain = NewObject<UArchonBotBrainComponent>(Bot, TEXT("ArchonBotBrain"));
	if (Brain)
	{
		Brain->RegisterComponent();
		Brain->ConfigureBotWithTuning(Team, EnemyCore, OwnCore, NextBotIndex, StrategyTuning);
		if (RouteSites.Num() > 0)
		{
			Brain->SetRouteWaypoint(RouteSites[NextBotIndex % RouteSites.Num()].Location);
		}
	}

	BotPlayers.Add(Bot);
	++FieldedBotsPerTeam[Team];
	++NextBotIndex;
	return Bot;
}

void UArchonCanaryWorldSubsystem::HandleReinforcementPurchased(int32 TeamId, int32 SquadSize)
{
	if (TeamId < 0 || TeamId > 1)
	{
		return;
	}

	// Re-resolve cores (the purchase can outlive the pointers cached at
	// bot-match spawn).
	AArchonBaseCoreActor* TeamCores[2] = { nullptr, nullptr };
	for (AArchonBaseCoreActor* Core : ValleyBaseCores)
	{
		if (Core && Core->GetTeamId() >= 0 && Core->GetTeamId() <= 1)
		{
			TeamCores[Core->GetTeamId()] = Core;
		}
	}
	if (!TeamCores[TeamId] || !TeamCores[1 - TeamId])
	{
		return;
	}

	const FArchonMatchConfig MatchConfig = BuildArchonMatchConfig(GetWorld());
	const TArray<FArchonValleyPlacement> Layout = UArchonSplitrootValleyLayoutLibrary::BuildSplitrootValleyLayout();
	const TArray<FArchonValleyPlacement> RouteSites =
		UArchonSplitrootValleyLayoutLibrary::FilterPlacementsByPiece(Layout, EArchonValleyPiece::ResourceSite);
	TArray<FArchonValleyPlacement> ReinforcementRouteSites = RouteSites;
	if (RuntimeMapTable &&
		RuntimeMapTable->GetTeamId() == TeamId &&
		RuntimeMapTable->GetTeamState() &&
		RuntimeMapTable->GetTeamState()->HasTeamRallyPoint())
	{
		FArchonValleyPlacement RallyRoute;
		RallyRoute.PieceId = TEXT("team_rally_point");
		RallyRoute.Piece = EArchonValleyPiece::SquadSpawn;
		RallyRoute.TeamId = TeamId;
		RallyRoute.Location = RuntimeMapTable->GetTeamState()->GetTeamRallyPoint();
		ReinforcementRouteSites.Reset();
		ReinforcementRouteSites.Add(RallyRoute);
		UE_LOG(
			LogTemp,
			Display,
			TEXT("ArchonFactoryCanary: ReinforcementRallyApplied team=%d location=%s"),
			TeamId,
			*RallyRoute.Location.ToCompactString());
	}

	int32 Fielded = 0;
	const FArchonBotStrategyTuning StrategyTuning = UArchonBotStrategyTuningLibrary::GetSplitrootBotStrategyTuning();
	for (int32 Index = 0; Index < SquadSize; ++Index)
	{
		if (FieldedBotsPerTeam[TeamId] >= MatchConfig.MaxFieldedBotsPerTeam)
		{
			// SC-style: the bank burned at the cap; the commander's job
			// is to spend before it overflows. Tuning hypothesis.
			UE_LOG(
				LogTemp,
				Display,
				TEXT("ArchonFactoryCanary: ReinforcementAtFieldCap team=%d cap=%d"),
				TeamId,
				MatchConfig.MaxFieldedBotsPerTeam);
			break;
		}
		if (SpawnFieldedBot(TeamId, TeamCores[TeamId], TeamCores[1 - TeamId], ReinforcementRouteSites, StrategyTuning))
		{
			++Fielded;
		}
	}

	if (Fielded > 0)
	{
		UE_LOG(
			LogTemp,
			Display,
			TEXT("ArchonFactoryCanary: ReinforcementFielded team=%d count=%d fieldedTotal=%d"),
			TeamId,
			Fielded,
			FieldedBotsPerTeam[TeamId]);
	}
}

void UArchonCanaryWorldSubsystem::HandleTechBuildingPurchased(int32 TeamId, FName TechId)
{
	if (TeamId < 0 || TeamId > 1 || TechId.IsNone())
	{
		return;
	}

	for (AArchonTechBuildingActor* ExistingBuilding : ValleyTechBuildings)
	{
		if (ExistingBuilding &&
			ExistingBuilding->GetTeamId() == TeamId &&
			ExistingBuilding->GetTechId() == TechId &&
			ExistingBuilding->GetHealth() &&
			ExistingBuilding->GetHealth()->IsAlive())
		{
			UE_LOG(
				LogTemp,
				Display,
				TEXT("ArchonFactoryCanary: TechBuildingPlacementSkipped team=%d tech=%s reason=already_alive"),
				TeamId,
				*TechId.ToString());
			return;
		}
	}

	UWorld* World = GetWorld();
	AArchonMatchStateActor* State = ResolveMatchState();
	if (!World || !State)
	{
		return;
	}

	FLinearColor TeamColor = TeamId == 0
		? FLinearColor(0.18f, 0.85f, 0.24f, 1.0f)
		: FLinearColor(0.18f, 0.72f, 0.95f, 1.0f);
	for (AArchonBaseCoreActor* Core : ValleyBaseCores)
	{
		if (Core && Core->GetTeamId() == TeamId)
		{
			TeamColor = TeamId == 0
				? FLinearColor(0.18f, 0.85f, 0.24f, 1.0f)
				: FLinearColor(0.18f, 0.72f, 0.95f, 1.0f);
			break;
		}
	}

	FActorSpawnParameters Params;
	Params.Name = MakeUniqueObjectName(World, AArchonTechBuildingActor::StaticClass(), TEXT("ArchonTechBuilding"));
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	AArchonTechBuildingActor* Building = World->SpawnActor<AArchonTechBuildingActor>(
		AArchonTechBuildingActor::StaticClass(),
		ResolveTechBuildingSlot(TeamId, TechId),
		FRotator::ZeroRotator,
		Params);
	if (!Building)
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("ArchonFactoryCanary: TechBuildingPlacementFailed team=%d tech=%s"),
			TeamId,
			*TechId.ToString());
		return;
	}

	Building->ConfigureTechBuilding(TeamId, TechId, State, TeamColor);
	ValleyTechBuildings.Add(Building);
	ValleyActors.Add(Building);
}

FVector UArchonCanaryWorldSubsystem::ResolveTechBuildingSlot(int32 TeamId, FName TechId) const
{
	if (TechId == TEXT("armory"))
	{
		return TeamId == 0
			? FVector(-14850.0f, -1650.0f, 260.0f)
			: FVector(11450.0f, 6900.0f, 260.0f);
	}
	if (TechId == TEXT("beast_den"))
	{
		return TeamId == 0
			? FVector(-15250.0f, -1050.0f, 260.0f)
			: FVector(11850.0f, 7500.0f, 260.0f);
	}
	if (TechId == TEXT("grove"))
	{
		return TeamId == 0
			? FVector(-14450.0f, -1050.0f, 260.0f)
			: FVector(11050.0f, 7500.0f, 260.0f);
	}
	return TeamId == 0
		? FVector(-15650.0f, -1050.0f, 260.0f)
		: FVector(12250.0f, 7500.0f, 260.0f);
}

APawn* UArchonCanaryWorldSubsystem::ResolveSpectatorPawn()
{
	if (SpectatorPawn && IsValid(SpectatorPawn))
	{
		return SpectatorPawn;
	}
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}
	for (TActorIterator<ASpectatorPawn> It(World); It; ++It)
	{
		SpectatorPawn = *It;
		return SpectatorPawn;
	}
	return nullptr;
}

void UArchonCanaryWorldSubsystem::CycleBotView(int32 Direction)
{
	UWorld* World = GetWorld();
	APlayerController* PlayerController = World ? World->GetFirstPlayerController() : nullptr;
	if (!PlayerController || BotPlayers.Num() == 0 || ControlledBot)
	{
		return;
	}

	const int32 Count = BotPlayers.Num();
	ViewedBotIndex = ViewedBotIndex == INDEX_NONE
		? (Direction >= 0 ? 0 : Count - 1)
		: (ViewedBotIndex + Direction + Count) % Count;
	AArchonCanaryFpsCharacter* Bot = BotPlayers[ViewedBotIndex];
	if (!Bot || !IsValid(Bot))
	{
		return;
	}

	PlayerController->SetViewTargetWithBlend(Bot, 0.25f);
	const UArchonCombatHealthComponent* Health = Bot->GetHealth();
	UE_LOG(
		LogTemp,
		Display,
		TEXT("ArchonFactoryCanary: SpectatorView bot=%d team=%d"),
		ViewedBotIndex,
		Health ? Health->GetTeamId() : INDEX_NONE);
	NotifyPlaytestEvent(TEXT("spectator_view"));
}

void UArchonCanaryWorldSubsystem::ReturnToSharedView()
{
	UWorld* World = GetWorld();
	APlayerController* PlayerController = World ? World->GetFirstPlayerController() : nullptr;
	APawn* Spectator = ResolveSpectatorPawn();
	if (!PlayerController || !Spectator || ControlledBot)
	{
		return;
	}

	ViewedBotIndex = INDEX_NONE;
	PlayerController->SetViewTargetWithBlend(Spectator, 0.25f);
	UE_LOG(LogTemp, Display, TEXT("ArchonFactoryCanary: SpectatorSharedView"));
}

void UArchonCanaryWorldSubsystem::FocusMapTableNearestBot()
{
	UWorld* World = GetWorld();
	APlayerController* PlayerController = World ? World->GetFirstPlayerController() : nullptr;
	if (!PlayerController || BotPlayers.Num() == 0 || ControlledBot)
	{
		return;
	}

	const FVector TableLocation = RuntimeMapTable
		? RuntimeMapTable->GetActorLocation()
		: ValleyMapTableLocation;
	const int32 TableTeamId = RuntimeMapTable ? RuntimeMapTable->GetTeamId() : 0;

	AArchonCanaryFpsCharacter* BestBot = nullptr;
	int32 BestIndex = INDEX_NONE;
	float BestDistanceSq = TNumericLimits<float>::Max();

	for (int32 Index = 0; Index < BotPlayers.Num(); ++Index)
	{
		AArchonCanaryFpsCharacter* Candidate = BotPlayers[Index];
		if (!Candidate || !IsValid(Candidate))
		{
			continue;
		}

		const UArchonCombatHealthComponent* CandidateHealth = Candidate->GetHealth();
		if (!CandidateHealth || !CandidateHealth->IsAlive() || CandidateHealth->GetTeamId() != TableTeamId)
		{
			continue;
		}

		const float DistanceSq = FVector::DistSquared2D(Candidate->GetActorLocation(), TableLocation);
		if (DistanceSq < BestDistanceSq)
		{
			BestDistanceSq = DistanceSq;
			BestIndex = Index;
			BestBot = Candidate;
		}
	}

	if (!BestBot)
	{
		for (int32 Index = 0; Index < BotPlayers.Num(); ++Index)
		{
			AArchonCanaryFpsCharacter* Candidate = BotPlayers[Index];
			if (!Candidate || !IsValid(Candidate))
			{
				continue;
			}

			const UArchonCombatHealthComponent* CandidateHealth = Candidate->GetHealth();
			if (!CandidateHealth || !CandidateHealth->IsAlive())
			{
				continue;
			}

			const float DistanceSq = FVector::DistSquared2D(Candidate->GetActorLocation(), TableLocation);
			if (DistanceSq < BestDistanceSq)
			{
				BestDistanceSq = DistanceSq;
				BestIndex = Index;
				BestBot = Candidate;
			}
		}
	}

	if (!BestBot)
	{
		UE_LOG(LogTemp, Display, TEXT("ArchonFactoryCanary: SpectatorMapTableView selected=false reason=no_living_bot"));
		return;
	}

	ViewedBotIndex = BestIndex;
	PlayerController->SetViewTargetWithBlend(BestBot, 0.25f);

	const UArchonCombatHealthComponent* BestHealth = BestBot->GetHealth();
	UE_LOG(
		LogTemp,
		Display,
		TEXT("ArchonFactoryCanary: SpectatorMapTableView selected=true bot=%d team=%d distance=%.0f pawn=%s"),
		ViewedBotIndex,
		BestHealth ? BestHealth->GetTeamId() : INDEX_NONE,
		FMath::Sqrt(BestDistanceSq),
		*BestBot->GetName());
	NotifyPlaytestEvent(TEXT("spectator_map_table"));
}

void UArchonCanaryWorldSubsystem::ToggleBotTakeOver()
{
	UWorld* World = GetWorld();
	APlayerController* PlayerController = World ? World->GetFirstPlayerController() : nullptr;
	if (!PlayerController || BotPlayers.Num() == 0)
	{
		return;
	}

	if (ControlledBot)
	{
		// Release: the brain wakes up and plays on from wherever the
		// human left the body.
		if (UArchonCombatHealthComponent* ControlledHealth = ControlledBot->GetHealth())
		{
			ControlledHealth->OnDeath.RemoveAll(this);
		}
		if (UArchonBotBrainComponent* Brain = ControlledBot->FindComponentByClass<UArchonBotBrainComponent>())
		{
			Brain->SetHumanControlled(false);
		}
		AArchonCanaryFpsCharacter* Released = ControlledBot;
		ControlledBot = nullptr;
		if (APawn* Spectator = ResolveSpectatorPawn())
		{
			PlayerController->Possess(Spectator);
			PlayerController->SetControlRotation(FRotator(-65.0f, 0.0f, 0.0f));
		}
		ViewedBotIndex = INDEX_NONE;
		UE_LOG(LogTemp, Display, TEXT("ArchonFactoryCanary: SpectatorReleased pawn=%s"), *Released->GetName());
		NotifyPlaytestEvent(TEXT("spectator_release"));
		return;
	}

	// Take over the viewed player, or "player 1" (first living bot).
	AArchonCanaryFpsCharacter* Target = nullptr;
	if (ViewedBotIndex != INDEX_NONE && BotPlayers.IsValidIndex(ViewedBotIndex))
	{
		Target = BotPlayers[ViewedBotIndex];
	}
	if (!Target || !IsValid(Target) || (Target->GetHealth() && !Target->GetHealth()->IsAlive()))
	{
		for (AArchonCanaryFpsCharacter* Candidate : BotPlayers)
		{
			if (Candidate && IsValid(Candidate) && Candidate->GetHealth() && Candidate->GetHealth()->IsAlive())
			{
				Target = Candidate;
				break;
			}
		}
	}
	if (!Target)
	{
		return;
	}

	if (UArchonBotBrainComponent* Brain = Target->FindComponentByClass<UArchonBotBrainComponent>())
	{
		Brain->SetHumanControlled(true);
	}
	ControlledBot = Target;
	PlayerController->Possess(Target);
	// Humans die like everyone else (Jonathan 2026-06-10: "I seemed to
	// never die"): death auto-releases the body back to its brain, which
	// ragdolls and respawns it while the human spectates.
	if (UArchonCombatHealthComponent* TargetHealth = Target->GetHealth())
	{
		TargetHealth->OnDeath.RemoveAll(this);
		TargetHealth->OnDeath.AddUObject(this, &UArchonCanaryWorldSubsystem::HandleControlledBotDeath);
	}
	UE_LOG(LogTemp, Display, TEXT("ArchonFactoryCanary: SpectatorTookOver pawn=%s"), *Target->GetName());
	NotifyPlaytestEvent(TEXT("spectator_takeover"));
}

void UArchonCanaryWorldSubsystem::HandleControlledBotDeath(const FArchonDamageApplicationResult& DamageResult)
{
	if (!ControlledBot)
	{
		return;
	}
	UE_LOG(LogTemp, Display, TEXT("ArchonFactoryCanary: HumanBodyDied pawn=%s"), *ControlledBot->GetName());
	NotifyPlaytestEvent(TEXT("human_death"));
	ToggleBotTakeOver();
}

AArchonMatchStateActor* UArchonCanaryWorldSubsystem::ResolveMatchState()
{
	if (MatchState && IsValid(MatchState))
	{
		return MatchState;
	}
	if (WeakMatchState.IsValid())
	{
		return WeakMatchState.Get();
	}
	// Clients receive the replicated match state actor; find and cache.
	UWorld* World = GetWorld();
	if (World)
	{
		for (TActorIterator<AArchonMatchStateActor> It(World); It; ++It)
		{
			WeakMatchState = *It;
			return WeakMatchState.Get();
		}
	}
	return nullptr;
}

void UArchonCanaryWorldSubsystem::ShowBuildVersionStampIfNeeded()
{
	if (BuildVersionWidget.IsValid() || !GEngine || !GEngine->GameViewport)
	{
		return;
	}

	TWeakObjectPtr<UArchonCanaryWorldSubsystem> WeakThis(this);
	TSharedRef<SWidget> Stamp =
		SNew(SOverlay)
		.Visibility(EVisibility::HitTestInvisible)
		+ SOverlay::Slot()
		.HAlign(HAlign_Right)
		.VAlign(VAlign_Top)
		.Padding(0.0f, 18.0f, 18.0f, 0.0f)
		[
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
			.BorderBackgroundColor(FSlateColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.50f)))
			.Padding(FMargin(10.0f, 5.0f))
			[
				SNew(STextBlock)
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
				.ColorAndOpacity(FSlateColor(FLinearColor(0.92f, 0.96f, 0.85f)))
				.Justification(ETextJustify::Right)
				.Text_Lambda([WeakThis]()
				{
					UArchonCanaryWorldSubsystem* Self = WeakThis.Get();
					AArchonMatchStateActor* State = Self ? Self->ResolveMatchState() : nullptr;
					if (State)
					{
						const FArchonMatchNetSnapshot Snapshot = State->GetNetSnapshot();
						if (!Snapshot.BuildVersion.IsEmpty() && !Snapshot.BuildEffectiveUtc.IsEmpty())
						{
							return FText::FromString(FString::Printf(
								TEXT("BUILD %s\nEFFECTIVE %s"),
								*Snapshot.BuildVersion,
								*Snapshot.BuildEffectiveUtc));
						}
					}

					const FArchonHudBuildStamp LocalStamp = ResolveLocalHudBuildStamp();
					return FText::FromString(FString::Printf(
						TEXT("BUILD %s\nEFFECTIVE %s"),
						*LocalStamp.Version,
						*LocalStamp.EffectiveUtc));
				})
			]
		];

	BuildVersionWidget = Stamp;
	GEngine->GameViewport->AddViewportWidgetContent(Stamp, /*ZOrder=*/65);
	UE_LOG(LogTemp, Display, TEXT("ArchonFactoryCanary: BuildVersionHudShown visible=true"));
}

void UArchonCanaryWorldSubsystem::ShowMatchHudIfNeeded()
{
	if (MatchHudWidget.IsValid() || !GEngine || !GEngine->GameViewport)
	{
		return;
	}

	TWeakObjectPtr<UArchonCanaryWorldSubsystem> WeakThis(this);

	// Match readout (top) + body readout (bottom-left). Pure Slate, all
	// data through lambdas off the replicated match snapshot — clients
	// and the host render the same truth.
	TSharedRef<SWidget> Hud =
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.HAlign(HAlign_Center)
		.Padding(0.0f, 52.0f, 0.0f, 0.0f)
		[
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
			.BorderBackgroundColor(FSlateColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.45f)))
			.Padding(FMargin(14.0f, 5.0f))
			[
				SNew(STextBlock)
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 15))
				.ColorAndOpacity(FSlateColor(FLinearColor(0.92f, 0.96f, 0.85f)))
				.Text_Lambda([WeakThis]()
				{
					UArchonCanaryWorldSubsystem* Self = WeakThis.Get();
					AArchonMatchStateActor* State = Self ? Self->ResolveMatchState() : nullptr;
					if (!State)
					{
						return FText::GetEmpty();
					}
					const FArchonMatchNetSnapshot Snapshot = State->GetNetSnapshot();
					const int32 Minutes = FMath::FloorToInt(Snapshot.PhaseSeconds / 60.0f);
					const int32 Seconds = FMath::FloorToInt(Snapshot.PhaseSeconds) % 60;
					switch (Snapshot.Phase)
					{
					case EArchonMatchPhase::Warmup:
						return FText::FromString(FString::Printf(TEXT("WARMUP  %d:%02d"), Minutes, Seconds));
					case EArchonMatchPhase::Live:
					{
						FString Line = FString::Printf(
							TEXT("LIVE %d:%02d    VERDANT %d — %d LENSWRIGHT    SUPPLY %d | %d"),
							Minutes, Seconds,
							Snapshot.PointsA, Snapshot.PointsB,
							Snapshot.SupplyA, Snapshot.SupplyB);
						// Fair-senses base alert: every player on a team
						// knows WHICH building is being hit and how hurt
						// it is — never where the attacker stands.
						if (!Snapshot.BaseAlertA.IsEmpty())
						{
							Line += FString::Printf(TEXT("\nVERDANT BASE UNDER ATTACK — %s"), *Snapshot.BaseAlertA);
						}
						if (!Snapshot.BaseAlertB.IsEmpty())
						{
							Line += FString::Printf(TEXT("\nLENSWRIGHT BASE UNDER ATTACK — %s"), *Snapshot.BaseAlertB);
						}
						return FText::FromString(Line);
					}
					case EArchonMatchPhase::MatchEnd:
					{
						const TCHAR* WinnerName =
							Snapshot.Winner == EArchonMatchWinner::TeamA ? TEXT("VERDANT WINS") :
							Snapshot.Winner == EArchonMatchWinner::TeamB ? TEXT("LENSWRIGHT WINS") :
							TEXT("DRAW");
						const float Countdown = FMath::Max(0.0f, Snapshot.ScoreboardSeconds - Snapshot.PhaseSeconds);
						const FString NextMap = Snapshot.NextMapId.IsNone()
							? FString(TEXT("pending"))
							: Snapshot.NextMapId.ToString();
						const FString Reason = Snapshot.MatchEndReason.IsEmpty()
							? FString(TEXT("complete"))
							: Snapshot.MatchEndReason;
						return FText::FromString(FString::Printf(
							TEXT("MATCH OVER - %s    %d - %d    %s    next %s in %.0fs"),
							WinnerName,
							Snapshot.PointsA,
							Snapshot.PointsB,
							*Reason,
							*NextMap,
							Countdown));
					}
					case EArchonMatchPhase::Traveling:
					{
						const FString NextMap = Snapshot.NextMapId.IsNone()
							? FString(TEXT("match"))
							: Snapshot.NextMapId.ToString();
						return FText::FromString(FString::Printf(TEXT("LOADING NEXT MATCH - %s"), *NextMap));
					}
					}
					return FText::GetEmpty();
				})
			]
		]
		+ SVerticalBox::Slot()
		.FillHeight(1.0f)
		[
			SNew(SSpacer)
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.HAlign(HAlign_Left)
		.Padding(24.0f, 0.0f, 0.0f, 24.0f)
		[
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
			.BorderBackgroundColor(FSlateColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.45f)))
			.Padding(FMargin(12.0f, 5.0f))
			.Visibility_Lambda([WeakThis]()
			{
				const UArchonCanaryWorldSubsystem* Self = WeakThis.Get();
				UWorld* World = Self ? Self->GetWorld() : nullptr;
				APlayerController* PlayerController = World ? World->GetFirstPlayerController() : nullptr;
				const AArchonCanaryFpsCharacter* Body = PlayerController ? Cast<AArchonCanaryFpsCharacter>(PlayerController->GetPawn()) : nullptr;
				return Body ? EVisibility::Visible : EVisibility::Collapsed;
			})
			[
				SNew(STextBlock)
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 15))
				.ColorAndOpacity(FSlateColor(FLinearColor(0.92f, 0.96f, 0.85f)))
				.Text_Lambda([WeakThis]()
				{
					const UArchonCanaryWorldSubsystem* Self = WeakThis.Get();
					UWorld* World = Self ? Self->GetWorld() : nullptr;
					APlayerController* PlayerController = World ? World->GetFirstPlayerController() : nullptr;
					const AArchonCanaryFpsCharacter* Body = PlayerController ? Cast<AArchonCanaryFpsCharacter>(PlayerController->GetPawn()) : nullptr;
					if (!Body)
					{
						return FText::GetEmpty();
					}
					const UArchonCombatHealthComponent* Health = Body->GetHealth();
					const UArchonVerdantThornsproutBow* Bow = Body->GetVerdantBow();
					return FText::FromString(FString::Printf(
						TEXT("HP %.0f/%.0f    ARROWS %d"),
						Health ? Health->GetCurrentHealth() : 0.0f,
						Health ? Health->GetMaxHealth() : 0.0f,
						Bow ? Bow->GetState().CurrentAmmo : 0));
				})
			]
		];
	MatchHudWidget = Hud;
	GEngine->GameViewport->AddViewportWidgetContent(Hud, /*ZOrder=*/45);
	UE_LOG(LogTemp, Display, TEXT("ArchonFactoryCanary: MatchHudShown visible=true"));
}

void UArchonCanaryWorldSubsystem::ShowSpectatorBannerIfNeeded()
{
	if (SpectatorBannerWidget.IsValid() || !GEngine || !GEngine->GameViewport)
	{
		return;
	}

	TWeakObjectPtr<UArchonCanaryWorldSubsystem> WeakThis(this);
	TSharedRef<SWidget> Banner =
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.HAlign(HAlign_Center)
		.Padding(0.0f, 18.0f, 0.0f, 0.0f)
		[
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
			.BorderBackgroundColor(FSlateColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.55f)))
			.Padding(FMargin(14.0f, 6.0f))
			[
				SNew(STextBlock)
				.Font(FCoreStyle::GetDefaultFontStyle("Bold", 14))
				.ColorAndOpacity(FSlateColor(FLinearColor(0.85f, 0.95f, 0.70f)))
				.Text_Lambda([WeakThis]()
				{
					const UArchonCanaryWorldSubsystem* Self = WeakThis.Get();
					if (!Self)
					{
						return FText::GetEmpty();
					}
					if (Self->ControlledBot)
					{
						return FText::FromString(FString::Printf(
							TEXT("CONTROLLING %s    F: release to AI"),
							*Self->ControlledBot->GetName()));
					}
					if (Self->ViewedBotIndex != INDEX_NONE)
					{
						return FText::FromString(FString::Printf(
							TEXT("VIEWING PLAYER %d    N: next    M: map-table bot    F: take control"),
							Self->ViewedBotIndex));
					}
					return FText::FromString(
						TEXT("MATCH VIEW    N: player views    M: map-table bot    F: take control    ESC: menu"));
				})
			]
		];
	SpectatorBannerWidget = Banner;
	GEngine->GameViewport->AddViewportWidgetContent(Banner, /*ZOrder=*/55);
	UE_LOG(LogTemp, Display, TEXT("ArchonFactoryCanary: SpectatorBannerShown visible=true"));
}

void UArchonCanaryWorldSubsystem::SpawnMainMenuIfRequested()
{
	UWorld* World = GetWorld();
	if (!World || !World->IsGameWorld() || MainMenu)
	{
		return;
	}
	// URL-option only (no command-line check): -ArchonMainMenu on the
	// process command line would persist across the host travel and
	// respawn the menu on the gameplay map.
	if (!UArchonLaunchFlagLibrary::HasUrlOption(World->URL, TEXT("ArchonMainMenu")))
	{
		return;
	}

	MainMenu = NewObject<UArchonMainMenuController>(World);
	const TArray<FArchonMapEntry> Hostable = UArchonMainMenuController::GetHostableMaps();
	UE_LOG(LogTemp, Display, TEXT("ArchonFactoryCanary: MainMenuActive maps=%d"), Hostable.Num());

	// Clickable panel only where there's a viewport; headless smokes drive
	// the controller directly through the proof timer below.
	if (GEngine && GEngine->GameViewport)
	{
		TSharedRef<SArchonMainMenuPanel> Panel = SNew(SArchonMainMenuPanel).MenuController(MainMenu);
		MainMenuPanelWidget = Panel;
		GEngine->GameViewport->AddViewportWidgetContent(Panel, /*ZOrder=*/100);
		ApplyMainMenuInputMode();
		// Re-assert after world settle: the player controller can log in
		// after begin play, and nothing may clobber the menu's cursor.
		World->GetTimerManager().SetTimer(
			MainMenuCursorTimer,
			this,
			&UArchonCanaryWorldSubsystem::ApplyMainMenuInputMode,
			0.5f,
			false);
		UE_LOG(LogTemp, Display, TEXT("ArchonFactoryCanary: MainMenuPanelShown"));
	}

	// Visual proof: screenshot the rendered menu (UI included — the world
	// capture path deliberately excludes Slate) and quit.
	if (UArchonLaunchFlagLibrary::HasUrlOption(World->URL, TEXT("ArchonMainMenuScreenshot")))
	{
		World->GetTimerManager().SetTimer(
			MainMenuProofTimer,
			FTimerDelegate::CreateWeakLambda(this, [this]()
			{
				const FString FileName = FPaths::ProjectSavedDir() / TEXT("Screenshots") / TEXT("Windows") / TEXT("MainMenu_0001.png");
				FPlatformFileManager::Get().GetPlatformFile().CreateDirectoryTree(*FPaths::GetPath(FileName));
				FScreenshotRequest::RequestScreenshot(FileName, /*bInShowUI=*/ true, /*bAddFilenameSuffix=*/ false);
				UE_LOG(LogTemp, Display, TEXT("ArchonFactoryCanary: MainMenuScreenshotTaken path=%s"), *FileName);
				if (UWorld* TimerWorld = GetWorld())
				{
					TimerWorld->GetTimerManager().SetTimer(
						PlayTestQuitTimer,
						this,
						&UArchonCanaryWorldSubsystem::IssuePlayTestQuit,
						2.5f,
						false);
				}
			}),
			3.0f,
			false);
		return;
	}

	// Proof: drive the same HostMap path a button click will call, after
	// the world settles. The hosted map quits itself via ArchonMatchProofQuit.
	if (UArchonLaunchFlagLibrary::HasUrlOption(World->URL, TEXT("ArchonMainMenuProofHost")) && Hostable.Num() > 0)
	{
		const FName ProofMapId = Hostable[0].MapId;
		World->GetTimerManager().SetTimer(
			MainMenuProofTimer,
			FTimerDelegate::CreateWeakLambda(this, [this, ProofMapId]()
			{
				if (MainMenu)
				{
					MainMenu->HostMap(ProofMapId, { TEXT("ArchonMatchProofQuit") });
				}
			}),
			1.5f,
			false);
	}
}

void UArchonCanaryWorldSubsystem::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PlaytestRecorderTimer);
	}
	PlaytestRecorderDir.Reset();
	PlaytestRecorderShotIndex = 0;
	LastPlaytestEventShotSeconds.Reset();

	// The game viewport outlives this world across ServerTravel/ClientTravel;
	// pull the menu panel off it or it stays painted over the hosted match.
	if (MainMenuPanelWidget.IsValid() && GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(MainMenuPanelWidget.ToSharedRef());
		UE_LOG(LogTemp, Display, TEXT("ArchonFactoryCanary: MainMenuPanelRemoved onWorldTeardown=true"));
	}
	MainMenuPanelWidget.Reset();

	if (SpectatorBannerWidget.IsValid() && GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(SpectatorBannerWidget.ToSharedRef());
	}
	SpectatorBannerWidget.Reset();

	if (BuildVersionWidget.IsValid() && GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(BuildVersionWidget.ToSharedRef());
	}
	BuildVersionWidget.Reset();

	if (MatchHudWidget.IsValid() && GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(MatchHudWidget.ToSharedRef());
	}
	MatchHudWidget.Reset();

	Super::Deinitialize();
}

void UArchonCanaryWorldSubsystem::ApplyMainMenuInputMode()
{
	UWorld* World = GetWorld();
	APlayerController* PlayerController = World ? World->GetFirstPlayerController() : nullptr;
	if (!PlayerController)
	{
		UE_LOG(LogTemp, Warning, TEXT("ArchonFactoryCanary: MainMenuCursorShown cursor=false reason=no_player_controller"));
		return;
	}

	PlayerController->SetShowMouseCursor(true);
	PlayerController->SetInputMode(FInputModeUIOnly());
	UE_LOG(
		LogTemp,
		Display,
		TEXT("ArchonFactoryCanary: MainMenuCursorShown cursor=%s inputMode=uionly"),
		PlayerController->ShouldShowMouseCursor() ? TEXT("true") : TEXT("false"));
}

void UArchonCanaryWorldSubsystem::SpawnValleyLightingIfNeeded()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// Authored maps keep their own lighting; only empty worlds get the
	// art-direction lighting anchor spawned at runtime.
	for (TActorIterator<ADirectionalLight> It(World); It; ++It)
	{
		UE_LOG(LogTemp, Display, TEXT("ArchonFactoryCanary: ValleyLighting spawned=false reason=existing_directional_light"));
		return;
	}

	const FArchonLightingAnchor Anchor = UArchonFactionPaletteLibrary::GetLightingAnchor();

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ADirectionalLight* Sun = World->SpawnActor<ADirectionalLight>(
		ADirectionalLight::StaticClass(),
		FVector(0.0, 0.0, 8000.0),
		FRotator(Anchor.DirectionalPitchDegrees, Anchor.DirectionalYawDegrees, 0.0f),
		Params);
	if (Sun)
	{
		Sun->SetMobility(EComponentMobility::Movable);
		if (ULightComponent* SunLight = Sun->GetLightComponent())
		{
			SunLight->SetIntensity(Anchor.DirectionalIntensityLux);
			SunLight->SetLightColor(Anchor.DirectionalColor);
		}
		ValleyActors.Add(Sun);
	}

	if (AActor* Atmosphere = World->SpawnActor<ASkyAtmosphere>(
		ASkyAtmosphere::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, Params))
	{
		ValleyActors.Add(Atmosphere);
	}

	ASkyLight* SkyLight = World->SpawnActor<ASkyLight>(
		ASkyLight::StaticClass(), FVector(0.0, 0.0, 6000.0), FRotator::ZeroRotator, Params);
	if (SkyLight)
	{
		if (USkyLightComponent* SkyComponent = SkyLight->GetLightComponent())
		{
			SkyComponent->SetMobility(EComponentMobility::Movable);
			SkyComponent->bRealTimeCapture = true;
			SkyComponent->SetIntensity(Anchor.SkyLightIntensity);
			SkyComponent->MarkRenderStateDirty();
		}
		ValleyActors.Add(SkyLight);
	}

	AExponentialHeightFog* Fog = World->SpawnActor<AExponentialHeightFog>(
		AExponentialHeightFog::StaticClass(), FVector(0.0, 0.0, 0.0), FRotator::ZeroRotator, Params);
	if (Fog)
	{
		if (UExponentialHeightFogComponent* FogComponent = Fog->GetComponent())
		{
			FogComponent->SetFogDensity(Anchor.ExponentialFogDensity);
			FogComponent->SetFogInscatteringColor(Anchor.ExponentialFogColor);
		}
		ValleyActors.Add(Fog);
	}

	// Lock exposure so blockout albedo reads true. Auto-exposure normalizes
	// the dim warm scene to cream and erases the palette (found in the
	// 2026-06-09 iteration-6 playtest frame).
	APostProcessVolume* Exposure = World->SpawnActor<APostProcessVolume>(
		APostProcessVolume::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, Params);
	if (Exposure)
	{
		Exposure->bUnbound = 1;
		Exposure->Settings.bOverride_AutoExposureMinBrightness = true;
		Exposure->Settings.bOverride_AutoExposureMaxBrightness = true;
		// EV100 lock. ~6 lux * ~0.35 albedo puts correct exposure near EV 2.5;
		// locking at 1.0 overexposed the valley to tan (iteration-7 frame).
		Exposure->Settings.AutoExposureMinBrightness = 2.6f;
		Exposure->Settings.AutoExposureMaxBrightness = 2.6f;
		ValleyActors.Add(Exposure);
	}

	UE_LOG(LogTemp, Display, TEXT("ArchonFactoryCanary: ValleyLighting spawned=true anchorPitch=%.0f anchorYaw=%.0f"),
		Anchor.DirectionalPitchDegrees,
		Anchor.DirectionalYawDegrees);
}

void UArchonCanaryWorldSubsystem::RunFirst60SecondsProofIfRequested()
{
	if (bFirst60ProofSequenceRan || !RuntimeInputBridge)
	{
		return;
	}
	if (!FParse::Param(FCommandLine::Get(), TEXT("ArchonRunFirst60SecondsProof")))
	{
		return;
	}

	// Phase 1 — blockout sanity already logged in SpawnBlockoutActorsIfRequested.

	// Phase 2 — visibility state: assert at least one lit cell exists (already logged
	// via RuntimeTeamVisibilityInitialized). Emit a first-60-specific flag too.
	bool bAnyLit = false;
	if (RuntimeMapTable)
	{
		if (UArchonTeamVisibilityStateComponent* VS = RuntimeMapTable->GetTeamVisibilityState())
		{
			bAnyLit = VS->CountCellsInState(EArchonTeamVisibilityState::Lit) > 0;
		}
	}
	UE_LOG(
		LogTemp,
		Display,
		TEXT("ArchonFactoryCanary: First60Visibility configured=true team=0 initialLit=%s"),
		bAnyLit ? TEXT("true") : TEXT("false"));

	// Phase 3 — open table → submit move with explicit target location → close.
	const bool bOpened = RuntimeInputBridge->PreviewRuntimeMapTable();
	UE_LOG(LogTemp, Display, TEXT("ArchonFactoryCanary: First60Widget opened=%s"), bOpened ? TEXT("true") : TEXT("false"));

	bool bOrderSubmitted = false;
	bool bSquadAccepted = false;
	bool bSquadMoving = false;
	FName First60SelectedSquadId = NAME_None;
	if (RuntimeMapTable && RuntimeRtsSquad)
	{
		// Route the first-60 move through the runtime widget so smoke covers
		// the same selection/order path the player uses at the map table.
		if (UArchonMapTableWidget* Widget = RuntimeInputBridge->GetRuntimeMapTableWidget())
		{
			const TArray<FName> SelectedSquadIds = Widget->GetSelectedSquadIds();
			First60SelectedSquadId = SelectedSquadIds.Num() > 0 ? SelectedSquadIds[0] : NAME_None;
		}

		const int32 PreviousSequence = RuntimeMapTable->GetCurrentCommandSequence();
		bOrderSubmitted = RuntimeInputBridge->SubmitRuntimeMapTableWidgetMoveOrderAt(
			FVector2D(2.0f / 3.0f, 0.50f),
			TEXT("central_splitroot"));
		const FArchonRtsCommandIntent LastIntent = RuntimeMapTable->GetTeamState()
			? RuntimeMapTable->GetTeamState()->GetLastAcceptedCommandIntent()
			: FArchonRtsCommandIntent();
		bSquadAccepted =
			bOrderSubmitted &&
			RuntimeMapTable->GetCurrentCommandSequence() > PreviousSequence &&
			LastIntent.SubjectId == TEXT("canary_squad") &&
			LastIntent.TargetId == TEXT("central_splitroot") &&
			LastIntent.bTargetLocationValid;
		bSquadMoving = RuntimeRtsSquad->GetOrderState() == EArchonCanaryRtsSquadOrderState::Moving;
	}
	UE_LOG(
		LogTemp,
		Display,
		TEXT("ArchonFactoryCanary: First60Widget selected=%s"),
		*First60SelectedSquadId.ToString());
	UE_LOG(
		LogTemp,
		Display,
		TEXT("ArchonFactoryCanary: First60Widget order=MoveSquad target=splitroot_central submitted=%s"),
		bOrderSubmitted ? TEXT("true") : TEXT("false"));
	UE_LOG(
		LogTemp,
		Display,
		TEXT("ArchonFactoryCanary: First60Squad accepted=%s sequence=%d"),
		bSquadAccepted ? TEXT("true") : TEXT("false"),
		RuntimeMapTable ? RuntimeMapTable->GetCurrentCommandSequence() : 0);
	UE_LOG(
		LogTemp,
		Display,
		TEXT("ArchonFactoryCanary: First60Squad state=%s"),
		bSquadMoving ? TEXT("Moving") : TEXT("Idle"));

	const bool bClosed = RuntimeInputBridge->CloseRuntimeMapTable();
	UE_LOG(LogTemp, Display, TEXT("ArchonFactoryCanary: First60Widget closed=%s"), bClosed ? TEXT("true") : TEXT("false"));

	// Phase 4 - root-vault chain. Route through the runtime FPS input bridge,
	// while ticking the movement component explicitly for headless smoke. Each cycle:
	// build the sprint window, jump to LAUNCH, then
	// immediately attempt another vault to prove cooldown enforcement, then
	// advance past cooldown for the next cycle.
	int32 RootVaultsLaunched = 0;
	if (RuntimeFpsCharacter)
	{
		if (UArchonFactionMovementComponent* FM = RuntimeFpsCharacter->GetFactionMovement())
		{
			for (int32 LaunchIndex = 1; LaunchIndex <= 3; ++LaunchIndex)
			{
				// Two ticks of 0.1s satisfies the 0.15s sprint-held anti-accident window.
				RuntimeInputBridge->ApplyRuntimeFactionMovementInput(RuntimeFpsCharacter, true, false, false);
				FM->TickComponent(0.1f, ELevelTick::LEVELTICK_All, nullptr);
				RuntimeInputBridge->ApplyRuntimeFactionMovementInput(RuntimeFpsCharacter, true, false, false);
				FM->TickComponent(0.1f, ELevelTick::LEVELTICK_All, nullptr);
				RuntimeInputBridge->ApplyRuntimeFactionMovementInput(RuntimeFpsCharacter, true, true, false);
				FM->TickComponent(0.05f, ELevelTick::LEVELTICK_All, nullptr); // LAUNCH

				// Immediately attempt a second vault before cooldown clears.
				// Sprint is still held and the window is satisfied. The component
				// must refuse the launch and increment CooldownBlockedCount.
				RuntimeInputBridge->ApplyRuntimeFactionMovementInput(RuntimeFpsCharacter, true, true, false);
				FM->TickComponent(0.05f, ELevelTick::LEVELTICK_All, nullptr); // BLOCKED

				// Advance past the 3s cooldown to ready the next cycle.
				FM->TickComponent(3.1f, ELevelTick::LEVELTICK_All, nullptr);
				RuntimeInputBridge->ApplyRuntimeFactionMovementInput(RuntimeFpsCharacter, false, false, false);
			}
			RootVaultsLaunched = FM->GetLaunchCount();
		}
	}
	UE_LOG(
		LogTemp,
		Display,
		TEXT("ArchonFactoryCanary: First60RootVault totalLaunches=%d"),
		RootVaultsLaunched);

	const bool bArcCompleted =
		(BlockoutVerdantOutpost != nullptr) &&
		(BlockoutSplitrootTree != nullptr) &&
		(BlockoutLenswrightGhost != nullptr) &&
		(BlockoutCoverStones.Num() == 12) &&
		bAnyLit &&
		bOpened &&
		bOrderSubmitted &&
		bSquadAccepted &&
		bSquadMoving &&
		bClosed &&
		(RootVaultsLaunched >= 3);

	UE_LOG(
		LogTemp,
		Display,
		TEXT("ArchonFactoryCanary: First60Arc completed=%s rootVaults=%d squadMoving=%s"),
		bArcCompleted ? TEXT("true") : TEXT("false"),
		RootVaultsLaunched,
		bSquadMoving ? TEXT("true") : TEXT("false"));

	bFirst60ProofSequenceRan = true;

	RunSecond60SecondsProofIfRequested();

	SchedulePlayTestScreenshotsIfRequested();
}

void UArchonCanaryWorldSubsystem::SpawnSecond60Enemies()
{
	UWorld* World = GetWorld();
	if (!World || !World->IsGameWorld() || Second60Bracewrights.Num() > 0)
	{
		return;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// The Lenswright push spills out of their ghost outpost toward the player's
	// approach lane. Anchor on the blockout ghost when present so the fight reads
	// correctly in rendered playtests, not just in logs.
	const FVector Anchor = BlockoutLenswrightGhost
		? BlockoutLenswrightGhost->GetActorLocation()
		: FVector(1600.0, 1000.0, 100.0);

	const FVector BracewrightOffsets[2] = { FVector(-350.0, -250.0, 20.0), FVector(-150.0, 250.0, 20.0) };
	const TCHAR* BracewrightIds[2] = { TEXT("second60_bracewright_a"), TEXT("second60_bracewright_b") };
	for (int32 Index = 0; Index < 2; ++Index)
	{
		Params.Name = MakeUniqueObjectName(World, AArchonLenswrightBracewrightActor::StaticClass(), TEXT("ArchonSecond60Bracewright"));
		AArchonLenswrightBracewrightActor* Bracewright = World->SpawnActor<AArchonLenswrightBracewrightActor>(
			AArchonLenswrightBracewrightActor::StaticClass(),
			Anchor + BracewrightOffsets[Index],
			FRotator(0.0f, 180.0f, 0.0f),
			Params);
		if (Bracewright)
		{
			Bracewright->ConfigureBracewright(1, BracewrightIds[Index]);
			Second60Bracewrights.Add(Bracewright);
		}
	}

	Params.Name = MakeUniqueObjectName(World, AArchonLenswrightSundialOpticActor::StaticClass(), TEXT("ArchonSecond60Sundial"));
	Second60Sundial = World->SpawnActor<AArchonLenswrightSundialOpticActor>(
		AArchonLenswrightSundialOpticActor::StaticClass(),
		Anchor + FVector(150.0, 0.0, 20.0),
		FRotator(0.0f, 180.0f, 0.0f),
		Params);
	if (Second60Sundial)
	{
		Second60Sundial->ConfigureSundial(1, TEXT("second60_sundial"));
	}
}

void UArchonCanaryWorldSubsystem::RunSecond60SecondsProofIfRequested()
{
	if (bSecond60ProofSequenceRan || !bFirst60ProofSequenceRan || !RuntimeInputBridge || !RuntimeFpsCharacter)
	{
		return;
	}
	if (!FParse::Param(FCommandLine::Get(), TEXT("ArchonRunSecond60SecondsProof")))
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World || !World->IsGameWorld())
	{
		return;
	}

	const double ArcStartSeconds = FPlatformTime::Seconds();

	// Phase 1 — the Lenswright squad arrives.
	SpawnSecond60Enemies();
	int32 SpawnedBracewrights = 0;
	for (const TObjectPtr<AArchonLenswrightBracewrightActor>& Bracewright : Second60Bracewrights)
	{
		if (Bracewright)
		{
			++SpawnedBracewrights;
		}
	}
	const bool bEnemiesSpawned = SpawnedBracewrights == 2 && Second60Sundial != nullptr;
	UE_LOG(
		LogTemp,
		Display,
		TEXT("ArchonFactoryCanary: Second60Enemies spawned=%s bracewright=%d sundial=%d"),
		bEnemiesSpawned ? TEXT("true") : TEXT("false"),
		SpawnedBracewrights,
		Second60Sundial ? 1 : 0);

	AArchonLenswrightBracewrightActor* BracewrightA = Second60Bracewrights.Num() > 0 ? Second60Bracewrights[0].Get() : nullptr;
	AArchonLenswrightBracewrightActor* BracewrightB = Second60Bracewrights.Num() > 1 ? Second60Bracewrights[1].Get() : nullptr;
	UArchonVerdantThornsproutBow* Bow = RuntimeFpsCharacter->GetVerdantBow();
	UArchonCombatHealthComponent* PlayerHealth = RuntimeFpsCharacter->GetHealth();
	if (!bEnemiesSpawned || !BracewrightA || !BracewrightB || !Bow || !PlayerHealth || !RuntimeRespawnState || !RuntimeMapTable || !RuntimeRtsSquad)
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("ArchonFactoryCanary: Second60Arc failed=true phase=Setup reason=missing_components enemies=%s bow=%s playerHealth=%s respawnState=%s mapTable=%s squad=%s"),
			bEnemiesSpawned ? TEXT("true") : TEXT("false"),
			Bow ? TEXT("true") : TEXT("false"),
			PlayerHealth ? TEXT("true") : TEXT("false"),
			RuntimeRespawnState ? TEXT("true") : TEXT("false"),
			RuntimeMapTable ? TEXT("true") : TEXT("false"),
			RuntimeRtsSquad ? TEXT("true") : TEXT("false"));
		bSecond60ProofSequenceRan = true;
		return;
	}

	// Phase 2 — the player's bow is in hand, quiver full.
	const FArchonWeaponState BowState = Bow->GetState();
	const bool bWeaponReady = Bow->IsReady();
	UE_LOG(
		LogTemp,
		Display,
		TEXT("ArchonFactoryCanary: Second60Player weapon=VerdantThornsproutBow ammo=%d/%d ready=%s"),
		BowState.CurrentAmmo,
		Bow->GetStats().QuiverCapacity,
		bWeaponReady ? TEXT("true") : TEXT("false"));

	// Phase 3 — player engages: 3 shots split across both Bracewrights so neither
	// dies (bow body damage 35 vs 80 health) and B stays healthy enough to return
	// fire instead of retreating.
	const FVector PlayerLocation = RuntimeFpsCharacter->GetActorLocation();
	const FRotator AimRotation = (BracewrightA->GetActorLocation() - PlayerLocation).Rotation();
	RuntimeFpsCharacter->SetActorRotation(FRotator(0.0f, AimRotation.Yaw, 0.0f));
	APlayerController* PlayerController = World->GetFirstPlayerController();
	if (PlayerController)
	{
		PlayerController->SetControlRotation(AimRotation);
	}

	const int32 InitialShotsFired = Bow->GetShotsFired();
	auto FireBowShotAt = [&](AActor* Target, bool bThroughInputBridge) -> bool
	{
		if (!Target)
		{
			return false;
		}
		FArchonShotPayload Shot;
		bool bShotCaptured = false;
		const FDelegateHandle FireHandle = Bow->OnWeaponFired.AddLambda(
			[&Shot, &bShotCaptured](const FArchonShotPayload& InShot)
			{
				Shot = InShot;
				bShotCaptured = true;
			});
		bool bFired = false;
		if (bThroughInputBridge)
		{
			bFired = RuntimeInputBridge->ApplyRuntimeWeaponInput(RuntimeFpsCharacter, true, false);
			RuntimeInputBridge->ApplyRuntimeWeaponInput(RuntimeFpsCharacter, false, false);
		}
		else
		{
			const FVector Origin = RuntimeFpsCharacter->GetActorLocation() + FVector(0.0f, 0.0f, 64.0f);
			const FVector Direction = (Target->GetActorLocation() - Origin).GetSafeNormal();
			bFired = Bow->TryFire(Origin, Direction);
		}
		Bow->OnWeaponFired.Remove(FireHandle);
		const bool bProjectileApplied = bFired && bShotCaptured && ApplyProofProjectileHit(
			*World,
			AArchonArrowProjectile::StaticClass(),
			Shot,
			Target,
			RuntimeFpsCharacter);
		Bow->TickComponent(Bow->GetStats().FireCycleSeconds + 0.05f, ELevelTick::LEVELTICK_All, nullptr);
		return bProjectileApplied;
	};

	UArchonCombatHealthComponent* HealthA = BracewrightA->GetHealth();
	UArchonCombatHealthComponent* HealthB = BracewrightB->GetHealth();
	const bool bShot1 = FireBowShotAt(BracewrightA, true);
	const bool bShot2 = FireBowShotAt(BracewrightB, false);
	const bool bShot3 = FireBowShotAt(BracewrightA, false);
	const int32 PlayerShotsFired = Bow->GetShotsFired() - InitialShotsFired;
	const int32 EnemyHitsA = HealthA ? HealthA->GetTotalHitsTaken() : 0;
	const int32 EnemyHitsB = HealthB ? HealthB->GetTotalHitsTaken() : 0;
	const bool bPlayerEngaged = bShot1 && bShot2 && bShot3 && PlayerShotsFired >= 3 && EnemyHitsA >= 2 && EnemyHitsB >= 1;
	UE_LOG(
		LogTemp,
		Display,
		TEXT("ArchonFactoryCanary: Second60Player firedShots=%d enemyHitsA=%d enemyHitsB=%d engaged=%s"),
		PlayerShotsFired,
		EnemyHitsA,
		EnemyHitsB,
		bPlayerEngaged ? TEXT("true") : TEXT("false"));

	// Phase 4 — Bracewright B returns fire through its AI decision.
	UArchonLenswrightPressureBoltCrossbow* CrossbowB = BracewrightB->GetCrossbow();
	UArchonAiCombatBehaviorComponent* BehaviorB = BracewrightB->GetCombatBehavior();
	FArchonAiTargetCandidate PlayerCandidate;
	PlayerCandidate.TargetId = TEXT("runtime_fps_player");
	PlayerCandidate.TargetTeamId = PlayerHealth->GetTeamId();
	PlayerCandidate.Location = RuntimeFpsCharacter->GetActorLocation();
	PlayerCandidate.bIsAlive = PlayerHealth->IsAlive();

	const FArchonAiCombatDecision AiDecision = (BehaviorB && HealthB)
		? BehaviorB->EvaluateCombat(BracewrightB->GetActorLocation(), { PlayerCandidate }, HealthB->GetHealthFraction())
		: FArchonAiCombatDecision();

	FArchonShotPayload AiShot;
	bool bAiShotCaptured = false;
	int32 AiShotsFired = 0;
	bool bPlayerDamaged = false;
	if (CrossbowB)
	{
		const FDelegateHandle AiFireHandle = CrossbowB->OnWeaponFired.AddLambda(
			[&AiShot, &bAiShotCaptured](const FArchonShotPayload& InShot)
			{
				AiShot = InShot;
				bAiShotCaptured = true;
			});
		const int32 InitialPlayerHits = PlayerHealth->GetTotalHitsTaken();
		const float InitialPlayerHealth = PlayerHealth->GetCurrentHealth();
		const FVector AiOrigin = BracewrightB->GetActorLocation() + FVector(0.0f, 0.0f, 64.0f);
		const FVector AiDirection = (RuntimeFpsCharacter->GetActorLocation() - AiOrigin).GetSafeNormal();
		const bool bAiFired = AiDecision.bShouldFire && CrossbowB->TryFire(AiOrigin, AiDirection);
		CrossbowB->OnWeaponFired.Remove(AiFireHandle);
		AiShotsFired = bAiFired ? 1 : 0;

		const bool bAiProjectileHit = bAiFired && bAiShotCaptured && ApplyProofProjectileHit(
			*World,
			AArchonPressureBoltProjectile::StaticClass(),
			AiShot,
			RuntimeFpsCharacter,
			BracewrightB);
		bPlayerDamaged =
			bAiProjectileHit &&
			PlayerHealth->GetTotalHitsTaken() == InitialPlayerHits + 1 &&
			PlayerHealth->GetCurrentHealth() < InitialPlayerHealth;
	}
	UE_LOG(
		LogTemp,
		Display,
		TEXT("ArchonFactoryCanary: Second60Bracewright firedShots=%d stance=%d shouldFire=%s"),
		AiShotsFired,
		static_cast<int32>(AiDecision.NewStance),
		AiDecision.bShouldFire ? TEXT("true") : TEXT("false"));
	UE_LOG(
		LogTemp,
		Display,
		TEXT("ArchonFactoryCanary: Second60Player damageTaken=%d health=%.1f"),
		PlayerHealth->GetTotalHitsTaken(),
		PlayerHealth->GetCurrentHealth());

	// Phase 5 — the pressure bolts win this round. Lethal damage uses the
	// Lenswright damage type so the death cause names the faction's weapon,
	// never a firearm.
	AArchonCanaryFpsCharacter* PreDeathCharacter = RuntimeFpsCharacter;
	const FArchonDamageApplicationResult LethalResult = PlayerHealth->ApplyDirectDamage(
		999.0f,
		EArchonDamageType::LenswrightPressureBolt,
		1,
		INDEX_NONE);
	const bool bPlayerDied = LethalResult.bCausedDeath && RuntimeRespawnState->GetLifeState() == EArchonLifeState::Dead;
	UE_LOG(
		LogTemp,
		Display,
		TEXT("ArchonFactoryCanary: Second60Player died=%s cause=lenswright_pressure_bolt"),
		bPlayerDied ? TEXT("true") : TEXT("false"));

	const bool bObserverPossessed =
		PlayerController &&
		RuntimeRespawnObserver &&
		PlayerController->GetPawn() == RuntimeRespawnObserver;
	UE_LOG(
		LogTemp,
		Display,
		TEXT("ArchonFactoryCanary: Second60Observer pawnPossessed=%s"),
		bObserverPossessed ? TEXT("true") : TEXT("false"));

	// Phase 6 — the Archon heartbeat: command the squad from the grave.
	const bool bTableOpened = RuntimeInputBridge->PreviewRuntimeMapTable();
	UE_LOG(
		LogTemp,
		Display,
		TEXT("ArchonFactoryCanary: Second60CommandWhileWait tableOpened=%s lifeState=%s"),
		bTableOpened ? TEXT("true") : TEXT("false"),
		RuntimeRespawnState->GetLifeState() == EArchonLifeState::Dead ? TEXT("Dead") : TEXT("Other"));

	const int32 PreviousDeathCommands = RuntimeInputBridge->GetCommandsIssuedDuringDeath();
	const bool bOrderSubmitted = bTableOpened && RuntimeInputBridge->SubmitRuntimeMapTableWidgetMoveOrderAt(
		FVector2D(0.35f, 0.30f),
		TEXT("splitroot_central_north"));
	const int32 DeathCommandSequence = RuntimeMapTable->GetCurrentCommandSequence();
	const bool bOrderRouted =
		bOrderSubmitted &&
		RuntimeInputBridge->GetCommandsIssuedDuringDeath() == PreviousDeathCommands + 1 &&
		RuntimeRtsSquad->GetOrderState() == EArchonCanaryRtsSquadOrderState::Moving &&
		RuntimeRtsSquad->GetLastAppliedCommandSequence() == DeathCommandSequence;
	UE_LOG(
		LogTemp,
		Display,
		TEXT("ArchonFactoryCanary: Second60CommandWhileWait orderSubmitted=%s sequence=%d"),
		bOrderRouted ? TEXT("true") : TEXT("false"),
		DeathCommandSequence);

	// Phase 7 — respawn timer runs out; a new body stands up at the base spawn.
	const float SecondsToRespawn = RuntimeRespawnState->GetState().SecondsUntilRespawn + 0.05f;
	RuntimeRespawnState->TickComponent(SecondsToRespawn, ELevelTick::LEVELTICK_All, nullptr);
	const bool bRespawned =
		RuntimeRespawnState->GetLifeState() == EArchonLifeState::Alive &&
		RuntimeFpsCharacter &&
		RuntimeFpsCharacter != PreDeathCharacter &&
		PlayerController &&
		PlayerController->GetPawn() == RuntimeFpsCharacter;
	UE_LOG(
		LogTemp,
		Display,
		TEXT("ArchonFactoryCanary: Second60Player respawned=%s lifeState=%s"),
		bRespawned ? TEXT("true") : TEXT("false"),
		RuntimeRespawnState->GetLifeState() == EArchonLifeState::Alive ? TEXT("Alive") : TEXT("Other"));

	// Phase 8 — the dead-state order outlived the body that issued it.
	FArchonRtsCommandIntent LastIntent;
	if (RuntimeMapTable->GetTeamState())
	{
		LastIntent = RuntimeMapTable->GetTeamState()->GetLastAcceptedCommandIntent();
	}
	const bool bSquadStillMoving = RuntimeRtsSquad->GetOrderState() == EArchonCanaryRtsSquadOrderState::Moving;
	const bool bOrderSurvived =
		bRespawned &&
		RuntimeMapTable->GetCurrentCommandSequence() == DeathCommandSequence &&
		RuntimeRtsSquad->GetLastAppliedCommandSequence() == DeathCommandSequence &&
		bSquadStillMoving &&
		LastIntent.TargetId == TEXT("splitroot_central_north");
	if (RuntimeInputBridge->IsMapTableSurfaceOpen())
	{
		RuntimeInputBridge->CloseRuntimeMapTable();
	}
	UE_LOG(
		LogTemp,
		Display,
		TEXT("ArchonFactoryCanary: Second60SquadOrderSurvivedRespawn lastSequence=%d stillMoving=%s"),
		DeathCommandSequence,
		(bOrderSurvived && bSquadStillMoving) ? TEXT("true") : TEXT("false"));

	const double ArcDurationSeconds = FPlatformTime::Seconds() - ArcStartSeconds;
	const bool bArcCompleted =
		bEnemiesSpawned &&
		bWeaponReady &&
		bPlayerEngaged &&
		AiShotsFired >= 1 &&
		bPlayerDamaged &&
		bPlayerDied &&
		bObserverPossessed &&
		bTableOpened &&
		bOrderRouted &&
		bRespawned &&
		bOrderSurvived;

	if (!bArcCompleted)
	{
		const TCHAR* FailedPhase =
			!bEnemiesSpawned ? TEXT("SpawnEnemies") :
			!bWeaponReady ? TEXT("VerifyPlayerWeaponReady") :
			!bPlayerEngaged ? TEXT("PlayerFiresAtBracewright") :
			AiShotsFired < 1 ? TEXT("BracewrightFiresAtPlayer") :
			!bPlayerDamaged ? TEXT("PlayerTakesDamage") :
			!bPlayerDied ? TEXT("ForcePlayerDeath") :
			!bObserverPossessed ? TEXT("VerifyObserverPawnPossessed") :
			!bTableOpened ? TEXT("OpenTableDuringDeath") :
			!bOrderRouted ? TEXT("SubmitOrderDuringDeath") :
			!bRespawned ? TEXT("WaitForRespawn") :
			TEXT("VerifyDeadStateOrderSurvived");
		UE_LOG(
			LogTemp,
			Error,
			TEXT("ArchonFactoryCanary: Second60Arc failed=true phase=%s reason=assertion_failed"),
			FailedPhase);
	}

	UE_LOG(
		LogTemp,
		Display,
		TEXT("ArchonFactoryCanary: Second60Arc completed=%s durationSeconds=%.1f"),
		bArcCompleted ? TEXT("true") : TEXT("false"),
		ArcDurationSeconds);

	bSecond60ProofSequenceRan = true;
}

bool UArchonCanaryWorldSubsystem::SubmitNetworkedRtsOrder(EArchonRtsOrderKind OrderKind)
{
	if (!RuntimeInputBridge)
	{
		UE_LOG(LogTemp, Warning, TEXT("ArchonFactoryCanary: NetworkedRtsOrderDropped reason=no_runtime_bridge"));
		return false;
	}

	return RuntimeInputBridge->SubmitRuntimeRtsOrder(OrderKind);
}

void UArchonCanaryWorldSubsystem::RunClientOrderProofIfRequested()
{
	UWorld* World = GetWorld();
	if (bClientOrderProofScheduled || !World || !RuntimeInputBridge || !RuntimeFpsCharacter)
	{
		return;
	}

	if (World->GetNetMode() != NM_Client ||
		!FParse::Param(FCommandLine::Get(), TEXT("ArchonClientOrderProof")))
	{
		return;
	}

	bClientOrderProofScheduled = true;
	World->GetTimerManager().SetTimer(
		ClientOrderProofTimer,
		this,
		&UArchonCanaryWorldSubsystem::ExecuteClientRtsOrderProof,
		6.0f,
		false);
	UE_LOG(LogTemp, Display, TEXT("ArchonFactoryCanary: ClientOrderProofScheduled delay=6.0"));
}

void UArchonCanaryWorldSubsystem::ExecuteClientRtsOrderProof()
{
	if (!RuntimeInputBridge)
	{
		return;
	}

	const bool bRouted = RuntimeInputBridge->SubmitRuntimeRtsOrder(EArchonRtsOrderKind::MoveSquad);
	UE_LOG(
		LogTemp,
		Display,
		TEXT("ArchonFactoryCanary: ClientRtsOrderProofSent routed=%s"),
		bRouted ? TEXT("true") : TEXT("false"));
}

void UArchonCanaryWorldSubsystem::RunInteractProofIfRequested()
{
	if (bInteractProofRan || !RuntimeInputBridge || !RuntimeFpsCharacter || !RuntimeMapTable)
	{
		return;
	}

	if (!FParse::Param(FCommandLine::Get(), TEXT("ArchonInteractProof")))
	{
		return;
	}

	bInteractProofRan = true;

	// Walk-up arc, synchronously: far (no prompt) -> near (prompt) ->
	// E opens the table -> close -> camera toggle round trip.
	const FVector TableLocation = RuntimeMapTable->GetActorLocation();
	RuntimeFpsCharacter->SetActorLocation(TableLocation + FVector(5000.0f, 0.0f, 90.0f));
	RuntimeInputBridge->RefreshInteractPrompt();
	const bool bFarHidden = !RuntimeInputBridge->IsInteractPromptVisible();

	RuntimeFpsCharacter->SetActorLocation(TableLocation + FVector(350.0f, 0.0f, 90.0f));
	RuntimeInputBridge->RefreshInteractPrompt();
	const bool bNearShown = RuntimeInputBridge->IsInteractPromptVisible();

	const bool bOpened = RuntimeInputBridge->TryInteract();
	RuntimeInputBridge->RefreshInteractPrompt();
	const bool bPromptSuppressedWhileOpen = !RuntimeInputBridge->IsInteractPromptVisible();
	RuntimeInputBridge->CloseRuntimeMapTable();

	const bool bThird = !RuntimeFpsCharacter->ToggleCameraView();
	const bool bBackToFirst = RuntimeFpsCharacter->ToggleCameraView();

	UE_LOG(
		LogTemp,
		Display,
		TEXT("ArchonFactoryCanary: InteractProofCompleted farHidden=%s nearShown=%s opened=%s suppressedWhileOpen=%s thirdPerson=%s backToFirst=%s"),
		bFarHidden ? TEXT("true") : TEXT("false"),
		bNearShown ? TEXT("true") : TEXT("false"),
		bOpened ? TEXT("true") : TEXT("false"),
		bPromptSuppressedWhileOpen ? TEXT("true") : TEXT("false"),
		bThird ? TEXT("true") : TEXT("false"),
		bBackToFirst ? TEXT("true") : TEXT("false"));
}

void UArchonCanaryWorldSubsystem::StartPlaytestRecorderIfHumanSession()
{
	UWorld* World = GetWorld();
	if (!World || !World->IsGameWorld())
	{
		return;
	}

	// Human sessions only: headless proof runs pass -unattended and
	// -NullRHI; recording them would be noise (and a screenshot stall).
	if (FApp::IsUnattended() || !FApp::CanEverRender())
	{
		return;
	}

	// Long-lived hosted games learn from the log/replay metadata stream by
	// default. Rendered screenshot capture is opt-in because Slate screenshot
	// requests proved fragile during forever match travel.
	if (!UArchonLaunchFlagLibrary::IsLaunchFlagActive(World, TEXT("ArchonPlaytestRecorder")))
	{
		UE_LOG(LogTemp, Display, TEXT("ArchonFactoryCanary: PlaytestRecorderSkipped reason=opt_in_flag_missing"));
		return;
	}

	const FString SessionId = FString::Printf(
		TEXT("%s-%s"),
		*FDateTime::Now().ToString(TEXT("%Y%m%d-%H%M%S")),
		*World->GetMapName());
	PlaytestRecorderDir = FPaths::ProjectSavedDir() / TEXT("PlaytestCaptures") / SessionId;
	FPlatformFileManager::Get().GetPlatformFile().CreateDirectoryTree(*PlaytestRecorderDir);
	PlaytestRecorderShotIndex = 0;

	// 5s baseline; the event shots below carry the actual action beats
	// (a human can run a whole build-and-change-body sequence in 10s).
	World->GetTimerManager().SetTimer(
		PlaytestRecorderTimer,
		this,
		&UArchonCanaryWorldSubsystem::TakePlaytestRecorderShot,
		5.0f,
		true);
	UE_LOG(
		LogTemp,
		Display,
		TEXT("ArchonFactoryCanary: PlaytestRecorderStarted session=%s dir=%s intervalSeconds=5 eventShots=true"),
		*SessionId,
		*PlaytestRecorderDir);
}

void UArchonCanaryWorldSubsystem::NotifyPlaytestEvent(const TCHAR* EventName)
{
	UWorld* World = GetWorld();
	if (!World || PlaytestRecorderDir.IsEmpty() || PlaytestRecorderShotIndex >= 720)
	{
		return;
	}

	// Rate limit PER EVENT NAME: arrow spam shares one shot per beat, but
	// a weapon_fire must never swallow a pause_open right behind it —
	// distinct beats are exactly what the recorder exists to catch.
	const FName EventKey(EventName);
	const double NowSeconds = World->GetTimeSeconds();
	const double* LastSeconds = LastPlaytestEventShotSeconds.Find(EventKey);
	if (LastSeconds && NowSeconds - *LastSeconds < 0.75)
	{
		return;
	}
	LastPlaytestEventShotSeconds.Add(EventKey, NowSeconds);

	++PlaytestRecorderShotIndex;
	const FString FileName = PlaytestRecorderDir / FString::Printf(
		TEXT("shot_%04d_%s.png"),
		PlaytestRecorderShotIndex,
		EventName);
	FScreenshotRequest::RequestScreenshot(FileName, /*bInShowUI=*/ true, /*bAddFilenameSuffix=*/ false);
	UE_LOG(
		LogTemp,
		Display,
		TEXT("ArchonFactoryCanary: PlaytestRecorderEventShot index=%d event=%s"),
		PlaytestRecorderShotIndex,
		EventName);
}

void UArchonCanaryWorldSubsystem::TakePlaytestRecorderShot()
{
	// 2 hours at one shot per 10s; a runaway idle session should not fill
	// the disk with identical frames.
	if (PlaytestRecorderShotIndex >= 720)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(PlaytestRecorderTimer);
		}
		UE_LOG(LogTemp, Display, TEXT("ArchonFactoryCanary: PlaytestRecorderStopped reason=shot_cap"));
		return;
	}

	++PlaytestRecorderShotIndex;
	const FString FileName = PlaytestRecorderDir / FString::Printf(TEXT("shot_%04d.png"), PlaytestRecorderShotIndex);
	FScreenshotRequest::RequestScreenshot(FileName, /*bInShowUI=*/ true, /*bAddFilenameSuffix=*/ false);

	// Display-level so the session log correlates shots with the action
	// markers around them (the shot index IS the timeline).
	UE_LOG(
		LogTemp,
		Display,
		TEXT("ArchonFactoryCanary: PlaytestRecorderShot index=%d file=%s"),
		PlaytestRecorderShotIndex,
		*FileName);
}

void UArchonCanaryWorldSubsystem::RunPauseMenuProofIfRequested()
{
	UWorld* World = GetWorld();
	if (bPauseMenuProofScheduled || !World || !RuntimeInputBridge)
	{
		return;
	}

	if (!FParse::Param(FCommandLine::Get(), TEXT("ArchonPauseMenuProof")))
	{
		return;
	}

	bPauseMenuProofScheduled = true;
	// Synchronous like the other runtime proofs: the map smoke quits via
	// -ExecCmds=quit, so a delayed timer would never fire.
	ExecutePauseMenuProof();
}

void UArchonCanaryWorldSubsystem::ExecutePauseMenuProof()
{
	if (!RuntimeInputBridge)
	{
		return;
	}

	// Same handlers the Escape key and the slider drive. Restore the
	// user's saved look scale afterward — the proof must not overwrite
	// a real setting in GameUserSettings.ini.
	const float OriginalScale = RuntimeInputBridge->GetMouseLookScale();
	const bool bOpened = RuntimeInputBridge->TogglePauseMenu();
	RuntimeInputBridge->SetMouseLookScale(0.12f);
	const float ProofScale = RuntimeInputBridge->GetMouseLookScale();
	const bool bClosed = RuntimeInputBridge->TogglePauseMenu();
	RuntimeInputBridge->SetMouseLookScale(OriginalScale);
	UE_LOG(
		LogTemp,
		Display,
		TEXT("ArchonFactoryCanary: PauseMenuProofCompleted opened=%s closed=%s scale=%.3f"),
		bOpened ? TEXT("true") : TEXT("false"),
		bClosed ? TEXT("true") : TEXT("false"),
		ProofScale);
}

void UArchonCanaryWorldSubsystem::RunClientFireProofIfRequested()
{
	UWorld* World = GetWorld();
	if (bClientFireProofScheduled || !World || !RuntimeInputBridge || !RuntimeFpsCharacter)
	{
		return;
	}

	if (World->GetNetMode() != NM_Client ||
		!FParse::Param(FCommandLine::Get(), TEXT("ArchonClientFireProof")))
	{
		return;
	}

	bClientFireProofScheduled = true;

	// Two staggered shots (the bow's fire cycle is 1.2s): first through the
	// input bridge to prove the trigger path routes to the server RPC, then
	// a point-blank shot at the enemy core so the damage assertion does not
	// depend on terrain between the spawn fan and the Lenswright base.
	World->GetTimerManager().SetTimer(
		ClientFireProofBridgeShotTimer,
		this,
		&UArchonCanaryWorldSubsystem::ExecuteClientFireProofBridgeShot,
		2.0f,
		false);
	World->GetTimerManager().SetTimer(
		ClientFireProofCoreShotTimer,
		this,
		&UArchonCanaryWorldSubsystem::ExecuteClientFireProofCoreShot,
		4.0f,
		false);
	UE_LOG(LogTemp, Display, TEXT("ArchonFactoryCanary: ClientFireProofScheduled bridgeShotDelay=2.0 coreShotDelay=4.0"));
}

void UArchonCanaryWorldSubsystem::ExecuteClientFireProofBridgeShot()
{
	if (!RuntimeInputBridge || !RuntimeFpsCharacter)
	{
		return;
	}

	const bool bRouted = RuntimeInputBridge->ApplyRuntimeWeaponInput(RuntimeFpsCharacter, true, false);
	UE_LOG(
		LogTemp,
		Display,
		TEXT("ArchonFactoryCanary: ClientFireProofBridgeShot routed=%s"),
		bRouted ? TEXT("true") : TEXT("false"));
}

void UArchonCanaryWorldSubsystem::ExecuteClientFireProofCoreShot()
{
	if (!RuntimeFpsCharacter)
	{
		return;
	}

	const TArray<FArchonValleyPlacement> Cores = UArchonSplitrootValleyLayoutLibrary::FilterPlacementsByPiece(
		UArchonSplitrootValleyLayoutLibrary::BuildSplitrootValleyLayout(),
		EArchonValleyPiece::BaseCore);
	const FArchonValleyPlacement* EnemyCore = Cores.FindByPredicate(
		[](const FArchonValleyPlacement& Placement) { return Placement.TeamId == 1; });
	if (!EnemyCore)
	{
		UE_LOG(LogTemp, Error, TEXT("ArchonFactoryCanary: ClientFireProofCoreShot sent=false reason=missing_enemy_core_placement"));
		return;
	}

	// Approach from +X: the Lenswright defense tower stands southwest of
	// the core and eats arrows on the old -X line (towers gate cores now).
	const FVector Target = EnemyCore->Location;
	const FVector Origin = Target + FVector(900.0f, 0.0f, 0.0f);
	const FVector Direction = (Target - Origin).GetSafeNormal();
	RuntimeFpsCharacter->ServerFireBow(Origin, Direction);
	UE_LOG(
		LogTemp,
		Display,
		TEXT("ArchonFactoryCanary: ClientFireProofCoreShot sent=true target=%s origin=%s"),
		*Target.ToCompactString(),
		*Origin.ToCompactString());
}

void UArchonCanaryWorldSubsystem::SchedulePlayTestScreenshotsIfRequested()
{
	if (bPlayTestScreenshotScheduled)
	{
		return;
	}
	if (!FParse::Param(FCommandLine::Get(), TEXT("ArchonPlayTestScreenshots")))
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	bPlayTestScreenshotScheduled = true;

	// Wait for the renderer to warm up + actors to settle before capturing.
	World->GetTimerManager().SetTimer(
		PlayTestScreenshotTimer,
		this,
		&UArchonCanaryWorldSubsystem::TakePlayTestScreenshot,
		2.5f,
		false);

	UE_LOG(LogTemp, Display, TEXT("ArchonFactoryCanary: PlayTestScreenshotScheduled delaySeconds=2.5"));
}

void UArchonCanaryWorldSubsystem::FramePlayTestCameraForScreenshot()
{
	if (!RuntimeFpsCharacter)
	{
		UE_LOG(LogTemp, Warning, TEXT("ArchonFactoryCanary: PlayTestCameraFramed ready=false reason=missing_runtime_fps_character"));
		return;
	}

	// Valley hero shot: Verdant corner in the foreground, the Central
	// Splitroot owning the skyline. Template framing otherwise.
	const FVector CameraLocation = bValleyActive
		? FVector(-16800.0f, -6200.0f, 2300.0f)
		: FVector(-700.0f, -2500.0f, 1400.0f);
	const FVector LookTarget = bValleyActive
		? FVector(0.0f, 0.0f, 1300.0f)
		: (BlockoutLenswrightGhost
			? FVector(950.0f, 240.0f, 120.0f)
			: FVector(950.0f, 0.0f, 120.0f));
	const FRotator CameraRotation = (LookTarget - CameraLocation).Rotation();
	const FRotator ActorRotation(0.0f, CameraRotation.Yaw, 0.0f);

	RuntimeFpsCharacter->SetActorLocationAndRotation(
		CameraLocation,
		ActorRotation,
		false,
		nullptr,
		ETeleportType::TeleportPhysics);

	if (UWorld* World = GetWorld())
	{
		if (APlayerController* PlayerController = World->GetFirstPlayerController())
		{
			PlayerController->SetControlRotation(CameraRotation);
		}
	}

	UE_LOG(
		LogTemp,
		Display,
		TEXT("ArchonFactoryCanary: PlayTestCameraFramed ready=true location=%.0f,%.0f,%.0f target=%.0f,%.0f,%.0f hasBlockout=%s"),
		CameraLocation.X,
		CameraLocation.Y,
		CameraLocation.Z,
		LookTarget.X,
		LookTarget.Y,
		LookTarget.Z,
		(BlockoutVerdantOutpost && BlockoutSplitrootTree && BlockoutLenswrightGhost && BlockoutCoverStones.Num() == 12) ? TEXT("true") : TEXT("false"));
}

void UArchonCanaryWorldSubsystem::TakePlayTestScreenshot()
{
	UWorld* World = GetWorld();
	if (!World || !GEngine)
	{
		return;
	}

	GEngine->Exec(World, TEXT("r.MotionBlurQuality 0"));
	GEngine->Exec(World, TEXT("r.DefaultFeature.MotionBlur 0"));
	FramePlayTestCameraForScreenshot();

	World->GetTimerManager().SetTimer(
		PlayTestCaptureTimer,
		this,
		&UArchonCanaryWorldSubsystem::CapturePlayTestScreenshot,
		0.35f,
		false);
}

void UArchonCanaryWorldSubsystem::CapturePlayTestScreenshot()
{
	UWorld* World = GetWorld();
	if (!World || !GEngine)
	{
		return;
	}

	// Use FScreenshotRequest directly; it works in -game / -RenderOffScreen
	// where the HighResShot console exec sometimes silently no-ops.
	++PlayTestScreenshotsTaken;
	const FString FileName = FPaths::ProjectSavedDir() / TEXT("Screenshots") / TEXT("Windows") /
		FString::Printf(TEXT("Playtest_%04d.png"), PlayTestScreenshotsTaken);
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	PlatformFile.CreateDirectoryTree(*FPaths::GetPath(FileName));

	FScreenshotRequest::RequestScreenshot(FileName, /*bInShowUI=*/ false, /*bAddFilenameSuffix=*/ false);

	UE_LOG(
		LogTemp,
		Display,
		TEXT("ArchonFactoryCanary: PlayTestScreenshotTaken index=%d path=%s"),
		PlayTestScreenshotsTaken,
		*FileName);

	// Schedule the quit after the screenshot has flushed to disk.
	World->GetTimerManager().SetTimer(
		PlayTestQuitTimer,
		this,
		&UArchonCanaryWorldSubsystem::IssuePlayTestQuit,
		2.5f,
		false);
}

void UArchonCanaryWorldSubsystem::IssuePlayTestQuit()
{
	UWorld* World = GetWorld();
	if (!World || !GEngine)
	{
		return;
	}

	UE_LOG(
		LogTemp,
		Display,
		TEXT("ArchonFactoryCanary: PlayTestQuitIssued screenshotsTaken=%d"),
		PlayTestScreenshotsTaken);
	GEngine->Exec(World, TEXT("quit"));
}
