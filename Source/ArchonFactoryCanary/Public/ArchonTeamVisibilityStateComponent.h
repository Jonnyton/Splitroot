#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ArchonTeamVisibilityTypes.h"
#include "ArchonTeamVisibilityStateComponent.generated.h"

UCLASS(ClassGroup=(Archon), Blueprintable, meta=(BlueprintSpawnableComponent))
class ARCHONFACTORYCANARY_API UArchonTeamVisibilityStateComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UArchonTeamVisibilityStateComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintCallable, Category = "Archon|Visibility")
	void ConfigureVisibilityState(int32 InTeamId, const TArray<FArchonVisibilityGridCell>& InitialCells);

	UFUNCTION(BlueprintCallable, Category = "Archon|Visibility")
	void SetFriendlySources(const TArray<FArchonVisibilitySource>& InSources);

	UFUNCTION(BlueprintCallable, Category = "Archon|Visibility")
	bool AddFriendlySource(const FArchonVisibilitySource& Source);

	UFUNCTION(BlueprintCallable, Category = "Archon|Visibility")
	void RecomputeVisibility();

	UFUNCTION(BlueprintCallable, Category = "Archon|Visibility")
	void ResetProofState();

	UFUNCTION(BlueprintCallable, Category = "Archon|Visibility")
	bool ApplyBuildingVisionReport(const FArchonBuildingVisionReport& Report, FArchonBuildingSnapshot& OutSnapshot);

	UFUNCTION(BlueprintPure, Category = "Archon|Visibility")
	bool GetBuildingSnapshot(FName BuildingId, FArchonBuildingSnapshot& OutSnapshot) const;

	UFUNCTION(BlueprintPure, Category = "Archon|Visibility")
	int32 GetTeamId() const { return TeamId; }

	UFUNCTION(BlueprintPure, Category = "Archon|Visibility")
	TArray<FArchonVisibilityGridCell> GetVisibilityCells() const { return VisibilityCells; }

	UFUNCTION(BlueprintPure, Category = "Archon|Visibility")
	TArray<FArchonVisibilitySource> GetFriendlySources() const { return FriendlySources; }

	UFUNCTION(BlueprintPure, Category = "Archon|Visibility")
	TArray<FIntPoint> GetLastNewlyLitCells() const { return LastNewlyLitCells; }

	UFUNCTION(BlueprintPure, Category = "Archon|Visibility")
	int32 GetLastNewlyLitCellCount() const { return LastNewlyLitCells.Num(); }

	UFUNCTION(BlueprintPure, Category = "Archon|Visibility")
	EArchonTeamVisibilityState GetCellState(FIntPoint Cell) const;

	UFUNCTION(BlueprintPure, Category = "Archon|Visibility")
	int32 CountCellsInState(EArchonTeamVisibilityState State) const;

private:
	bool IsFriendlySource(const FArchonVisibilitySource& Source) const;
	FArchonBuildingSnapshot* FindMutableBuildingSnapshot(FName BuildingId);
	const FArchonBuildingSnapshot* FindBuildingSnapshot(FName BuildingId) const;

	UPROPERTY(Replicated)
	int32 TeamId = INDEX_NONE;

	UPROPERTY(Replicated)
	TArray<FArchonVisibilityGridCell> VisibilityCells;

	UPROPERTY(Replicated)
	TArray<FArchonVisibilitySource> FriendlySources;

	UPROPERTY(Replicated)
	TArray<FIntPoint> LastNewlyLitCells;

	UPROPERTY(Replicated)
	TArray<FArchonBuildingSnapshot> BuildingSnapshots;
};
