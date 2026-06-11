#pragma once

#include "CoreMinimal.h"
#include "ArchonFactionPaletteTypes.generated.h"

UENUM(BlueprintType)
enum class EArchonFactionPaletteSlot : uint8
{
	Primary UMETA(DisplayName = "Primary"),
	Secondary UMETA(DisplayName = "Secondary"),
	Tertiary UMETA(DisplayName = "Tertiary"),
	Accent UMETA(DisplayName = "Accent")
};

UENUM(BlueprintType)
enum class EArchonNeutralPaletteSlot : uint8
{
	CoverStone UMETA(DisplayName = "Cover Stone"),
	SplitrootWood UMETA(DisplayName = "Splitroot Wood"),
	GroundEarth UMETA(DisplayName = "Ground / Earth"),
	SkyHorizon UMETA(DisplayName = "Sky Horizon")
};

USTRUCT(BlueprintType)
struct FArchonLightingAnchor
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Archon|Art")
	float DirectionalPitchDegrees = -25.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Archon|Art")
	float DirectionalYawDegrees = 30.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Archon|Art")
	FLinearColor DirectionalColor = FLinearColor(1.0f, 0.85f, 0.7f, 1.0f);

	UPROPERTY(BlueprintReadOnly, Category = "Archon|Art")
	float DirectionalIntensityLux = 6.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Archon|Art")
	float SkyLightIntensity = 0.3f;

	// Retuned 0.05 -> 0.008 from the first rendered valley playtest
	// (2026-06-09): 0.05 bleached the full 400m valley to fog color.
	UPROPERTY(BlueprintReadOnly, Category = "Archon|Art")
	float ExponentialFogDensity = 0.008f;

	UPROPERTY(BlueprintReadOnly, Category = "Archon|Art")
	FLinearColor ExponentialFogColor = FLinearColor(0.42f, 0.50f, 0.55f, 1.0f);

	UPROPERTY(BlueprintReadOnly, Category = "Archon|Art")
	float PostProcessBloom = 0.4f;
};
