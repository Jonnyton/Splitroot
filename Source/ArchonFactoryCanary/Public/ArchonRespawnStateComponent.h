#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ArchonRespawnTypes.h"
#include "ArchonRespawnStateComponent.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FArchonRespawnLifeStateChangedNative, EArchonLifeState);
DECLARE_MULTICAST_DELEGATE_OneParam(FArchonRespawnReadyNative, const FArchonRespawnSpawnPoint&);

UCLASS(ClassGroup=(Archon), Blueprintable, meta=(BlueprintSpawnableComponent))
class ARCHONFACTORYCANARY_API UArchonRespawnStateComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UArchonRespawnStateComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category = "Archon|Respawn")
	void ConfigureRespawn(int32 InPlayerId, int32 InPlayerTeamId, const FArchonRespawnTuning& InTuning);

	UFUNCTION(BlueprintCallable, Category = "Archon|Respawn")
	void RegisterSpawnPoint(const FArchonRespawnSpawnPoint& SpawnPoint);

	UFUNCTION(BlueprintCallable, Category = "Archon|Respawn")
	bool HandlePlayerDeath(int32 KillerTeamId, int32 KillerPlayerId);

	UFUNCTION(BlueprintCallable, Category = "Archon|Respawn")
	bool RequestSpawnPointChoice(FName SpawnPointId);

	UFUNCTION(BlueprintCallable, Category = "Archon|Respawn")
	bool MarkRespawnComplete();

	UFUNCTION(BlueprintCallable, Category = "Archon|Proof")
	void SetLifeStateForProof(EArchonLifeState InLifeState);

	UFUNCTION(BlueprintPure, Category = "Archon|Respawn")
	int32 GetPlayerId() const { return PlayerId; }

	UFUNCTION(BlueprintPure, Category = "Archon|Respawn")
	int32 GetPlayerTeamId() const { return PlayerTeamId; }

	UFUNCTION(BlueprintPure, Category = "Archon|Respawn")
	FArchonRespawnState GetState() const { return State; }

	UFUNCTION(BlueprintPure, Category = "Archon|Respawn")
	EArchonLifeState GetLifeState() const { return State.LifeState; }

	UFUNCTION(BlueprintPure, Category = "Archon|Respawn")
	bool MayIssueMapTableCommand() const;

	UFUNCTION(BlueprintPure, Category = "Archon|Respawn")
	TArray<FArchonRespawnSpawnPoint> GetAvailableSpawnPoints() const;

	UFUNCTION(BlueprintPure, Category = "Archon|Respawn")
	bool GetSelectedSpawnPoint(FArchonRespawnSpawnPoint& OutSpawnPoint) const;

	FArchonRespawnLifeStateChangedNative OnLifeStateChanged;
	FArchonRespawnReadyNative OnRespawnReady;

private:
	bool HasRespawnAuthority() const;
	void SetLifeState(EArchonLifeState NewLifeState);
	float GetRespawnSecondsForSpawnPoint(const FArchonRespawnSpawnPoint& SpawnPoint) const;

	UPROPERTY(Replicated)
	int32 PlayerId = INDEX_NONE;

	UPROPERTY(Replicated)
	int32 PlayerTeamId = INDEX_NONE;

	UPROPERTY(Replicated)
	FArchonRespawnState State;

	UPROPERTY(Replicated)
	FArchonRespawnTuning Tuning;

	UPROPERTY(Replicated)
	TArray<FArchonRespawnSpawnPoint> SpawnPoints;
};
