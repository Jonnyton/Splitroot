#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ArchonFactionTypes.h"
#include "ArchonFactionPaletteTypes.h"
#include "ArchonFactionMaterialBuilder.generated.h"

class UMaterialInstanceDynamic;
class UMaterialInterface;

/**
 * Creates UMaterialInstanceDynamic instances with faction palette parameters
 * applied. Reads from UArchonFactionPaletteLibrary so the palette source of
 * truth is single. Consumed by every AArchon*Actor constructor that wants
 * faction-tinted geometry past the rung-A debug-color treatment.
 *
 * The base material is passed by caller so the substrate doesn't pin a
 * specific Content/ asset — future games swap base materials freely.
 */
UCLASS()
class ARCHONFACTORYCANARY_API UArchonFactionMaterialBuilder : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Archon|Art", meta = (DefaultToSelf = "Outer"))
	static UMaterialInstanceDynamic* CreateFactionMaterial(
		UObject* Outer,
		UMaterialInterface* BaseMaterial,
		EArchonFaction Faction,
		EArchonFactionPaletteSlot Slot = EArchonFactionPaletteSlot::Primary);

	/** Standard parameter names the MID will set; exposed so BP material can match them. */
	UFUNCTION(BlueprintPure, Category = "Archon|Art")
	static FName GetBaseColorParameterName() { return FName(TEXT("BaseColor")); }

	UFUNCTION(BlueprintPure, Category = "Archon|Art")
	static FName GetMetallicParameterName() { return FName(TEXT("Metallic")); }

	UFUNCTION(BlueprintPure, Category = "Archon|Art")
	static FName GetRoughnessParameterName() { return FName(TEXT("Roughness")); }
};
