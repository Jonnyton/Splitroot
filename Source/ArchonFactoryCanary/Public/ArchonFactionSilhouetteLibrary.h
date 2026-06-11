#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ArchonFactionTypes.h"
#include "ArchonFactionSilhouetteTypes.h"
#include "ArchonFactionSilhouetteLibrary.generated.h"

/**
 * Pure-lookup silhouette discipline library. Values mirror
 * games/<game-id>/FactoryContracts/faction_silhouette.json. Consumed by future procedural
 * mesh generators, body-picker UMG widgets, and capsule-based debug actors.
 *
 * Discipline rule: a player at 30m must identify faction by silhouette alone,
 * before color, before label.
 */
UCLASS()
class ARCHONFACTORYCANARY_API UArchonFactionSilhouetteLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Archon|Silhouette")
	static FArchonFactionSilhouette GetFactionSilhouette(EArchonFaction Faction);

	UFUNCTION(BlueprintPure, Category = "Archon|Silhouette")
	static FArchonHeroSilhouetteOverlay GetHeroOverlay();

	UFUNCTION(BlueprintPure, Category = "Archon|Silhouette")
	static float GetBaselineCapsuleHeightUnits() { return 192.0f; }

	UFUNCTION(BlueprintPure, Category = "Archon|Silhouette")
	static float GetBaselineCapsuleRadiusUnits() { return 42.0f; }

	/** Returns the resolved capsule height for a unit of the given faction (regular unit, not hero). */
	UFUNCTION(BlueprintPure, Category = "Archon|Silhouette")
	static float GetResolvedCapsuleHeightUnits(EArchonFaction Faction);

	/** Returns the resolved capsule radius for a unit of the given faction (regular unit, not hero). */
	UFUNCTION(BlueprintPure, Category = "Archon|Silhouette")
	static float GetResolvedCapsuleRadiusUnits(EArchonFaction Faction);
};
