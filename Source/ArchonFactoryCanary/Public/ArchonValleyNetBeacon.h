#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ArchonValleyNetBeacon.generated.h"

/**
 * Replicated marker that tells network clients "this world is the valley."
 * The valley is deterministic code, so clients rebuild the cosmetic set
 * locally instead of replicating hundreds of blockout actors; gameplay
 * actors (base cores, match state) replicate normally from the server.
 */
UCLASS()
class ARCHONFACTORYCANARY_API AArchonValleyNetBeacon : public AActor
{
	GENERATED_BODY()

public:
	AArchonValleyNetBeacon();

protected:
	virtual void BeginPlay() override;
};
