#pragma once

#include "CoreMinimal.h"
#include "ArchonRespawnTypes.generated.h"

UENUM(BlueprintType)
enum class EArchonLifeState : uint8
{
	Alive UMETA(DisplayName = "Alive"),
	Dying UMETA(DisplayName = "Dying"),
	Dead UMETA(DisplayName = "Dead"),
	Respawning UMETA(DisplayName = "Respawning")
};

USTRUCT(BlueprintType)
struct ARCHONFACTORYCANARY_API FArchonRespawnSpawnPoint
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Archon|Respawn")
	FName SpawnPointId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Archon|Respawn")
	int32 TeamId = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Archon|Respawn")
	FVector Location = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Archon|Respawn")
	FRotator Rotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Archon|Respawn")
	bool bIsForwardSpawn = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Archon|Respawn")
	bool bIsAvailable = true;
};

USTRUCT(BlueprintType)
struct ARCHONFACTORYCANARY_API FArchonRespawnState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Archon|Respawn")
	EArchonLifeState LifeState = EArchonLifeState::Alive;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Archon|Respawn")
	float SecondsUntilRespawn = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Archon|Respawn")
	float TotalRespawnSeconds = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Archon|Respawn")
	FName SelectedSpawnPointId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Archon|Respawn")
	int32 LivesLostThisMatch = 0;
};

USTRUCT(BlueprintType)
struct ARCHONFACTORYCANARY_API FArchonRespawnTuning
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Archon|Respawn")
	float DefaultRespawnSeconds = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Archon|Respawn")
	float ForwardSpawnPenaltySeconds = 1.5f;
};
