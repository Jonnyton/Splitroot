#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ArchonValleyLayoutTypes.h"
#include "ArchonSplitrootValleyLayoutLibrary.generated.h"

/**
 * Pure layout authority for Splitroot Valley. Returns deterministic
 * placements; spawning is the world subsystem's job. A different game on
 * this factory swaps this library (or a JSON source) and keeps the spawner.
 *
 * Valley shape (v0.5, top-down, +X east, +Y north):
 *
 *        N ridge ───────────────────────────
 *                          Lenswright (NE)
 *   Verdant (W)   CENTRAL SPLITROOT (0,0)
 *                          Kinwild (SE)
 *        S ridge ───────────────────────────
 *
 * Cover-stone breadcrumb lanes run from each base toward the tree, spaced
 * for the faction movement verbs (root-vault hop ≈ 15m).
 */
UCLASS()
class ARCHONFACTORYCANARY_API UArchonSplitrootValleyLayoutLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Archon|Valley")
	static TArray<FArchonValleyPlacement> BuildSplitrootValleyLayout();

	UFUNCTION(BlueprintPure, Category = "Archon|Valley")
	static TArray<FArchonValleyPlacement> FilterPlacementsByPiece(
		const TArray<FArchonValleyPlacement>& Layout,
		EArchonValleyPiece Piece);

	UFUNCTION(BlueprintPure, Category = "Archon|Valley")
	static bool FindPlacementById(
		const TArray<FArchonValleyPlacement>& Layout,
		FName PieceId,
		FArchonValleyPlacement& OutPlacement);

	// Valley constants exposed for tests and the spawner.
	static constexpr double ValleyFloorHalfLengthX = 20000.0;  // 400m end to end
	static constexpr double ValleyFloorHalfWidthY = 12000.0;   // 240m ridge to ridge
	static constexpr double CentralTreeTrunkHeight = 4500.0;   // 45m landmark
	static constexpr double CoverStoneLaneSpacing = 1500.0;    // one root-vault hop
};
