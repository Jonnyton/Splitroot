#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ArchonFactionTypes.h"
#include "ArchonFactionPaletteTypes.h"
#include "ArchonFactionPaletteLibrary.generated.h"

/**
 * Pure-lookup palette library. Values mirror games/<game-id>/FactoryContracts/faction_palette.json
 * (single source of truth for human review; C++ literals here are the runtime
 * mirror — per JSONify-factions plan, this will become a registry read).
 * Consumed by UArchonFactionMaterialBuilder and UArchonFactionTintedWidget.
 *
 * Lenswright Accent is alchemical cyan, not muzzle-flash orange — protects the
 * no-gunpowder hill at the palette layer.
 */
UCLASS()
class ARCHONFACTORYCANARY_API UArchonFactionPaletteLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Archon|Art")
	static FLinearColor GetFactionColor(EArchonFaction Faction, EArchonFactionPaletteSlot Slot);

	UFUNCTION(BlueprintPure, Category = "Archon|Art")
	static float GetFactionMetallic(EArchonFaction Faction);

	UFUNCTION(BlueprintPure, Category = "Archon|Art")
	static float GetFactionRoughness(EArchonFaction Faction);

	UFUNCTION(BlueprintPure, Category = "Archon|Art")
	static FLinearColor GetNeutralColor(EArchonNeutralPaletteSlot Slot);

	UFUNCTION(BlueprintPure, Category = "Archon|Art")
	static FArchonLightingAnchor GetLightingAnchor();
};
