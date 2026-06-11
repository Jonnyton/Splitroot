#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ArchonTeamVisibilityTypes.h"
#include "ArchonMapTableMiniatureLibrary.generated.h"

/**
 * Pure policy for the in-world miniature board on the Archon map table
 * (the WC3 hill: the table must read as a live diorama of the match,
 * not a menu). Render code feeds world facts in; these functions
 * decide where a marker sits on the table and whether fog hides it.
 */
UCLASS()
class ARCHONFACTORYCANARY_API UArchonMapTableMiniatureLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// Maps a world point into table-local space: world (origin +/-
	// extents) -> (+/- TableHalfX, +/- TableHalfY), clamped to the rim
	// so off-map actors pin to the board edge instead of floating off
	// the furniture. Z is always 0 (markers sit on the felt).
	UFUNCTION(BlueprintPure, Category = "Archon|MapTable")
	static FVector WorldToTableLocal(
		const FVector& WorldPoint,
		const FVector& WorldOrigin,
		const FVector& WorldExtents,
		float TableHalfX,
		float TableHalfY);

	// Standard WC3/SC fog read for board markers: a world point is
	// visible iff the NEAREST visibility cell is currently Lit. Fog
	// (explored-but-stale) and Black both hide live enemy markers.
	UFUNCTION(BlueprintPure, Category = "Archon|MapTable")
	static bool IsWorldPointLit(
		const TArray<FArchonVisibilityGridCell>& Cells,
		const FVector& WorldPoint);
};
