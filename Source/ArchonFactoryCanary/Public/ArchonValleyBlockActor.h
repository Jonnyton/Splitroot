#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ArchonValleyLayoutTypes.h"
#include "ArchonValleyBlockActor.generated.h"

class UStaticMeshComponent;

/**
 * Generic colored blockout piece for layout-driven worlds. One class covers
 * ground, ridges, tree parts, and stones; the layout placement supplies
 * identity, transform, and tint.
 */
UCLASS(Blueprintable)
class ARCHONFACTORYCANARY_API AArchonValleyBlockActor : public AActor
{
	GENERATED_BODY()

public:
	AArchonValleyBlockActor();

	void ConfigureBlock(const FArchonValleyPlacement& Placement);

	UFUNCTION(BlueprintPure, Category = "Archon|Valley")
	FName GetPieceId() const { return PieceId; }

	UFUNCTION(BlueprintPure, Category = "Archon|Valley")
	EArchonValleyPiece GetPiece() const { return Piece; }

	UFUNCTION(BlueprintPure, Category = "Archon|Valley")
	FLinearColor GetDebugColor() const { return DebugColor; }

private:
	UPROPERTY(VisibleAnywhere, Category = "Archon|Valley")
	TObjectPtr<UStaticMeshComponent> Block;

	UPROPERTY(VisibleAnywhere, Category = "Archon|Valley")
	FName PieceId = NAME_None;

	UPROPERTY(VisibleAnywhere, Category = "Archon|Valley")
	EArchonValleyPiece Piece = EArchonValleyPiece::None;

	UPROPERTY(VisibleAnywhere, Category = "Archon|Valley")
	FLinearColor DebugColor = FLinearColor::White;
};
