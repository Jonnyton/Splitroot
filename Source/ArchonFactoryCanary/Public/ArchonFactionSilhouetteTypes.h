#pragma once

#include "CoreMinimal.h"
#include "ArchonFactionSilhouetteTypes.generated.h"

UENUM(BlueprintType)
enum class EArchonMantleType : uint8
{
	None UMETA(DisplayName = "None"),
	RoundedTopCap UMETA(DisplayName = "Rounded Top Cap (Verdant grown-organic)"),
	RightShoulderProtrusion UMETA(DisplayName = "Right Shoulder Protrusion (Lenswright crossbow mount)"),
	LeftShoulderProtrusion UMETA(DisplayName = "Left Shoulder Protrusion"),
	MantleCloak UMETA(DisplayName = "Mantle Cloak (Kinwild)"),
	AnimalCompanion UMETA(DisplayName = "Animal Companion")
};

UENUM(BlueprintType)
enum class EArchonShoulderSide : uint8
{
	None UMETA(DisplayName = "None"),
	Right UMETA(DisplayName = "Right"),
	Left UMETA(DisplayName = "Left")
};

USTRUCT(BlueprintType)
struct FArchonFactionSilhouette
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Archon|Silhouette")
	float CapsuleHeightMultiplier = 1.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Archon|Silhouette")
	float CapsuleRadiusMultiplier = 1.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Archon|Silhouette")
	float LowerCapsuleRadiusMultiplier = 1.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Archon|Silhouette")
	EArchonMantleType MantleType = EArchonMantleType::None;

	UPROPERTY(BlueprintReadOnly, Category = "Archon|Silhouette")
	float ShoulderProtrusionUnits = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Archon|Silhouette")
	EArchonShoulderSide ShoulderSide = EArchonShoulderSide::None;

	UPROPERTY(BlueprintReadOnly, Category = "Archon|Silhouette")
	float HeadLensSphereRadiusUnits = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Archon|Silhouette")
	float ForwardLeanDegrees = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Archon|Silhouette")
	bool bHasAnimalCompanion = false;

	UPROPERTY(BlueprintReadOnly, Category = "Archon|Silhouette")
	float CompanionCapsuleHeightUnits = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Archon|Silhouette")
	float CompanionCapsuleRadiusUnits = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Archon|Silhouette")
	float CompanionOffsetRightUnits = 0.0f;
};

USTRUCT(BlueprintType)
struct FArchonHeroSilhouetteOverlay
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Archon|Silhouette")
	float ScaleMultiplier = 1.15f;

	UPROPERTY(BlueprintReadOnly, Category = "Archon|Silhouette")
	bool bGroundRingDecal = true;
};
