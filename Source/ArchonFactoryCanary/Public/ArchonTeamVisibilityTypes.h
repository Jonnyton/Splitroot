#pragma once

#include "CoreMinimal.h"
#include "ArchonTeamVisibilityTypes.generated.h"

UENUM(BlueprintType)
enum class EArchonTeamVisibilityState : uint8
{
	Black UMETA(DisplayName = "Black Shroud"),
	Fog UMETA(DisplayName = "Fog"),
	Lit UMETA(DisplayName = "Lit")
};

USTRUCT(BlueprintType)
struct FArchonVisibilitySource
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Archon|Visibility")
	int32 TeamId = INDEX_NONE;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Archon|Visibility")
	FVector Location = FVector::ZeroVector;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Archon|Visibility")
	float Radius = 0.0f;
};

USTRUCT(BlueprintType)
struct FArchonVisibilityGridCell
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Archon|Visibility")
	FIntPoint Cell = FIntPoint::ZeroValue;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Archon|Visibility")
	FVector WorldCenter = FVector::ZeroVector;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Archon|Visibility")
	EArchonTeamVisibilityState State = EArchonTeamVisibilityState::Black;
};

USTRUCT(BlueprintType)
struct FArchonBuildingVisionReport
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Archon|Visibility")
	FName BuildingId;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Archon|Visibility")
	int32 ObservedTeamId = INDEX_NONE;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Archon|Visibility")
	FVector Location = FVector::ZeroVector;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Archon|Visibility")
	FName ObservedState;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Archon|Visibility")
	int32 ObservationSequence = 0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Archon|Visibility")
	bool bCurrentlyVisible = false;
};

USTRUCT(BlueprintType)
struct FArchonBuildingSnapshot
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Archon|Visibility")
	FName BuildingId;

	UPROPERTY(BlueprintReadOnly, Category = "Archon|Visibility")
	int32 ObservedTeamId = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "Archon|Visibility")
	FVector LastKnownLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Archon|Visibility")
	FName LastKnownState;

	UPROPERTY(BlueprintReadOnly, Category = "Archon|Visibility")
	int32 LastObservedSequence = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Archon|Visibility")
	bool bHasSnapshot = false;
};
