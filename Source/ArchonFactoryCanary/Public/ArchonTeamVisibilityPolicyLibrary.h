#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ArchonTeamVisibilityTypes.h"
#include "ArchonTeamVisibilityPolicyLibrary.generated.h"

UCLASS()
class ARCHONFACTORYCANARY_API UArchonTeamVisibilityPolicyLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Archon|Visibility")
	static bool IsPointLitBySource(const FVector& Point, const FArchonVisibilitySource& Source);

	UFUNCTION(BlueprintPure, Category = "Archon|Visibility")
	static EArchonTeamVisibilityState ResolveCellVisibility(
		const FVector& CellWorldCenter,
		EArchonTeamVisibilityState PreviousState,
		const TArray<FArchonVisibilitySource>& FriendlySources);

	UFUNCTION(BlueprintCallable, Category = "Archon|Visibility")
	static void RecomputeVisibilityGrid(
		const TArray<FArchonVisibilityGridCell>& PreviousCells,
		const TArray<FArchonVisibilitySource>& FriendlySources,
		TArray<FArchonVisibilityGridCell>& OutCells,
		TArray<FIntPoint>& OutNewlyLitCells);

	UFUNCTION(BlueprintPure, Category = "Archon|Visibility")
	static FArchonBuildingSnapshot ResolveBuildingSnapshot(
		const FArchonBuildingSnapshot& PreviousSnapshot,
		const FArchonBuildingVisionReport& CurrentReport);

	// Replication policy: an actor owned by ActorTeamId, located on a
	// cell currently in ActorCellState for the viewing team, replicates
	// to ViewingTeamId iff own-team OR cell is Lit. Standard WC3/SC1
	// fog model — enemies in fog or black do not replicate.
	// Pure two-line predicate; lives in the policy library so the rule
	// is named, tested, and Blueprint-callable.
	// Added by Rook (Claude Opus 4.7) as an additive extension to Hex's
	// S1 — does not modify Hex's existing surface.
	UFUNCTION(BlueprintPure, Category = "Archon|Visibility")
	static bool ShouldReplicateActorBasedOnVisibility(
		int32 ViewingTeamId,
		int32 ActorTeamId,
		EArchonTeamVisibilityState ActorCellState);
};
