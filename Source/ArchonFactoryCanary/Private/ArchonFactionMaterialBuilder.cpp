#include "ArchonFactionMaterialBuilder.h"
#include "ArchonFactionPaletteLibrary.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

UMaterialInstanceDynamic* UArchonFactionMaterialBuilder::CreateFactionMaterial(
	UObject* Outer,
	UMaterialInterface* BaseMaterial,
	EArchonFaction Faction,
	EArchonFactionPaletteSlot Slot)
{
	if (BaseMaterial == nullptr || Outer == nullptr)
	{
		return nullptr;
	}

	UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(BaseMaterial, Outer);
	if (MID == nullptr)
	{
		return nullptr;
	}

	const FLinearColor BaseColor = UArchonFactionPaletteLibrary::GetFactionColor(Faction, Slot);
	const float Metallic = UArchonFactionPaletteLibrary::GetFactionMetallic(Faction);
	const float Roughness = UArchonFactionPaletteLibrary::GetFactionRoughness(Faction);

	MID->SetVectorParameterValue(GetBaseColorParameterName(), BaseColor);
	MID->SetScalarParameterValue(GetMetallicParameterName(), Metallic);
	MID->SetScalarParameterValue(GetRoughnessParameterName(), Roughness);

	return MID;
}
