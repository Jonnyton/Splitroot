#include "ArchonMatchGameMode.h"
#include "ArchonCanaryFpsCharacter.h"
#include "ArchonCanaryWorldSubsystem.h"
#include "ArchonFactionPaletteLibrary.h"
#include "ArchonFactionTypes.h"
#include "ArchonLaunchFlagLibrary.h"
#include "Engine/World.h"
#include "GameFramework/SpectatorPawn.h"

AArchonMatchGameMode::AArchonMatchGameMode()
{
	DefaultPawnClass = AArchonCanaryFpsCharacter::StaticClass();
}

void AArchonMatchGameMode::BeginPlay()
{
	Super::BeginPlay();

	// The listen host logs in during LoadMap, BEFORE the world subsystem
	// builds the valley — their pawn spawns through the engine-default
	// path at world origin. Subsystems begin play before actors, so the
	// valley exists by now: re-place every already-spawned player.
	UWorld* World = GetWorld();
	UArchonCanaryWorldSubsystem* Canary = World ? World->GetSubsystem<UArchonCanaryWorldSubsystem>() : nullptr;
	if (!Canary || !Canary->IsValleyActive())
	{
		return;
	}

	// Bot matches: the human starts WATCHING, not embodied (Jonathan
	// 2026-06-10 — he was auto-dumped into a body with no choice and no
	// view controls). Swap any already-spawned body for a free spectator.
	if (UArchonLaunchFlagLibrary::IsLaunchFlagActive(World, TEXT("ArchonBotMatch")))
	{
		for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
		{
			APlayerController* PlayerController = It->Get();
			if (!PlayerController || !PlayerController->IsLocalController())
			{
				continue;
			}
			APawn* OldPawn = PlayerController->GetPawn();
			// Shared RTS view first (Jonathan 2026-06-10): high over the
			// valley CENTER. 15km/-75° frames the central battlefield and
			// sites; the full both-bases picture needs the board UI's unit
			// blips (task #10) — raw cameras can't read 2m bodies at 40km.
			const FVector OverheadLocation(0.0, 0.0, 15000.0);
			const FRotator OverheadRotation(-75.0f, 0.0f, 0.0f);
			FActorSpawnParameters Params;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			ASpectatorPawn* Spectator = World->SpawnActor<ASpectatorPawn>(
				ASpectatorPawn::StaticClass(), OverheadLocation, OverheadRotation, Params);
			if (Spectator)
			{
				PlayerController->Possess(Spectator);
				PlayerController->SetControlRotation(OverheadRotation);
				if (OldPawn)
				{
					OldPawn->Destroy();
				}
				UE_LOG(LogTemp, Display, TEXT("ArchonFactoryCanary: SpectatorStart pawn=%s"), *Spectator->GetName());
			}
		}
		return;
	}

	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PlayerController = It->Get();
		APawn* Pawn = PlayerController ? PlayerController->GetPawn() : nullptr;
		if (!Pawn)
		{
			continue;
		}
		const int32 PlayerIndex = JoinedPlayerCount++;
		const FVector SpawnLocation =
			Canary->GetValleyPlayerSpawnLocation() + FVector(0.0, 300.0 * static_cast<double>(PlayerIndex), 0.0);
		const FRotator SpawnRotation = Canary->GetValleyPlayerSpawnRotation();
		Pawn->SetActorLocationAndRotation(SpawnLocation, SpawnRotation, false, nullptr, ETeleportType::TeleportPhysics);
		PlayerController->SetControlRotation(SpawnRotation);
		UE_LOG(
			LogTemp,
			Display,
			TEXT("ArchonFactoryCanary: MatchPlayerSpawned playerIndex=%d pawn=%s"),
			PlayerIndex,
			*Pawn->GetName());
	}
}

APawn* AArchonMatchGameMode::SpawnDefaultPawnFor_Implementation(AController* NewPlayer, AActor* StartSpot)
{
	UWorld* World = GetWorld();
	UArchonCanaryWorldSubsystem* Canary = World ? World->GetSubsystem<UArchonCanaryWorldSubsystem>() : nullptr;
	if (!Canary || !Canary->IsValleyActive())
	{
		return Super::SpawnDefaultPawnFor_Implementation(NewPlayer, StartSpot);
	}

	const int32 PlayerIndex = JoinedPlayerCount++;
	const FVector SpawnLocation =
		Canary->GetValleyPlayerSpawnLocation() + FVector(0.0, 300.0 * static_cast<double>(PlayerIndex), 0.0);
	const FRotator SpawnRotation = Canary->GetValleyPlayerSpawnRotation();

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AArchonCanaryFpsCharacter* Pawn = World->SpawnActor<AArchonCanaryFpsCharacter>(
		AArchonCanaryFpsCharacter::StaticClass(), SpawnLocation, SpawnRotation, Params);
	if (Pawn)
	{
		// Humans are team 0 (Verdant dress) until team selection ships.
		Pawn->SetBodyTeamColor(UArchonFactionPaletteLibrary::GetFactionColor(
			EArchonFaction::VerdantChoir, EArchonFactionPaletteSlot::Primary));
	}

	UE_LOG(
		LogTemp,
		Display,
		TEXT("ArchonFactoryCanary: MatchPlayerSpawned playerIndex=%d pawn=%s"),
		PlayerIndex,
		Pawn ? *Pawn->GetName() : TEXT("none"));
	return Pawn;
}

void AArchonMatchGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
	UE_LOG(
		LogTemp,
		Display,
		TEXT("ArchonFactoryCanary: MatchPlayerJoined players=%d remote=%s"),
		GetNumPlayers(),
		(NewPlayer && !NewPlayer->IsLocalController()) ? TEXT("true") : TEXT("false"));
}
