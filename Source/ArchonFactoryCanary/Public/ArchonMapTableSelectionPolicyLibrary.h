#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ArchonMapTableSelectionPolicyLibrary.generated.h"

USTRUCT(BlueprintType)
struct FArchonSelectableSquadCandidate
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Archon|MapTable")
	FName SquadId;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Archon|MapTable")
	int32 TeamId = INDEX_NONE;

	// Table-space position (normalized 0..1 over the rendered map).
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Archon|MapTable")
	FVector2D TableSpacePosition = FVector2D::ZeroVector;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Archon|MapTable")
	bool bIsAlive = true;
};

USTRUCT(BlueprintType)
struct FArchonDragBox
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Archon|MapTable")
	FVector2D Min = FVector2D::ZeroVector;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Archon|MapTable")
	FVector2D Max = FVector2D::ZeroVector;
};

UCLASS()
class ARCHONFACTORYCANARY_API UArchonMapTableSelectionPolicyLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Archon|MapTable")
	static FArchonDragBox NormalizeDragBox(const FVector2D& StartCorner, const FVector2D& EndCorner);

	UFUNCTION(BlueprintPure, Category = "Archon|MapTable")
	static bool BoxContainsPoint(const FArchonDragBox& Box, const FVector2D& Point);

	UFUNCTION(BlueprintPure, Category = "Archon|MapTable")
	static bool IsDragBoxDegenerate(const FArchonDragBox& Box);

	UFUNCTION(BlueprintCallable, Category = "Archon|MapTable")
	static void ComputeSelection(
		const FArchonDragBox& Box,
		int32 IssuingTeamId,
		const TArray<FArchonSelectableSquadCandidate>& Candidates,
		TArray<FName>& OutSelectedSquadIds);

	UFUNCTION(BlueprintCallable, Category = "Archon|MapTable")
	static void ResolveClickSelection(
		const FVector2D& ClickPoint,
		float ClickRadius,
		int32 IssuingTeamId,
		const TArray<FArchonSelectableSquadCandidate>& Candidates,
		TArray<FName>& OutSelectedSquadIds);

	UFUNCTION(BlueprintPure, Category = "Archon|MapTable")
	static FVector TableSpaceToWorld(
		const FVector2D& TableSpacePoint,
		const FVector& WorldOrigin,
		const FVector& WorldExtents);

	UFUNCTION(BlueprintPure, Category = "Archon|MapTable")
	static FVector2D WorldToTableSpace(
		const FVector& WorldPoint,
		const FVector& WorldOrigin,
		const FVector& WorldExtents);
};
