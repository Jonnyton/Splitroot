#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ArchonRespawnTypes.h"
#include "ArchonRespawnPolicyLibrary.generated.h"

UCLASS()
class ARCHONFACTORYCANARY_API UArchonRespawnPolicyLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Archon|Respawn")
	static bool SelectDefaultSpawnPoint(
		int32 PlayerTeamId,
		const TArray<FArchonRespawnSpawnPoint>& SpawnPoints,
		FArchonRespawnSpawnPoint& OutSpawnPoint);

	UFUNCTION(BlueprintCallable, Category = "Archon|Respawn")
	static bool CanPlayerSelectSpawnPoint(
		int32 PlayerTeamId,
		FName SpawnPointId,
		const TArray<FArchonRespawnSpawnPoint>& SpawnPoints,
		FArchonRespawnSpawnPoint& OutSpawnPoint);

	UFUNCTION(BlueprintCallable, Category = "Archon|Respawn")
	static void BeginRespawn(
		FArchonRespawnState& State,
		const FArchonRespawnSpawnPoint& SelectedSpawnPoint,
		const FArchonRespawnTuning& Tuning);

	UFUNCTION(BlueprintCallable, Category = "Archon|Respawn")
	static bool AdvanceRespawnTimer(FArchonRespawnState& State, float DeltaSeconds);

	UFUNCTION(BlueprintPure, Category = "Archon|Respawn")
	static bool MayIssueMapTableCommandInLifeState(EArchonLifeState LifeState);
};
