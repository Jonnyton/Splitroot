#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ArchonMatchGameMode.generated.h"

/**
 * Match GameMode (server-authoritative): every player who joins — host or
 * network client — spawns as an FPS character at the valley player spawn,
 * fanned out so simultaneous joins don't stack. Selected per-map via the
 * registry's ?game= URL option; worlds without it keep the engine default.
 */
UCLASS()
class ARCHONFACTORYCANARY_API AArchonMatchGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AArchonMatchGameMode();

	virtual void BeginPlay() override;
	virtual APawn* SpawnDefaultPawnFor_Implementation(AController* NewPlayer, AActor* StartSpot) override;
	virtual void PostLogin(APlayerController* NewPlayer) override;

private:
	int32 JoinedPlayerCount = 0;
};
