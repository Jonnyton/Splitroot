#include "ArchonValleyNetBeacon.h"
#include "ArchonCanaryWorldSubsystem.h"
#include "Engine/World.h"

AArchonValleyNetBeacon::AArchonValleyNetBeacon()
{
	bReplicates = true;
	bAlwaysRelevant = true;
	SetReplicatingMovement(false);
}

void AArchonValleyNetBeacon::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		return;
	}

	UWorld* World = GetWorld();
	UArchonCanaryWorldSubsystem* Canary = World ? World->GetSubsystem<UArchonCanaryWorldSubsystem>() : nullptr;
	if (Canary)
	{
		UE_LOG(LogTemp, Display, TEXT("ArchonFactoryCanary: ValleyNetBeaconClientSpawn"));
		Canary->SpawnSplitrootValleyForClient();
	}
}
