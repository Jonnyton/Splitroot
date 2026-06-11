#pragma once

#include "CoreMinimal.h"
#include "ArchonValleyLayoutTypes.generated.h"

UENUM(BlueprintType)
enum class EArchonValleyPiece : uint8
{
	None,
	GroundPlane,
	RidgeSlab,
	CentralTreeTrunk,
	CentralTreeCanopy,
	RootButtress,
	VerdantOutpost,
	LenswrightOutpost,
	KinwildCamp,
	BaseCore,
	ResourceSite,
	CoverStone,
	MapTable,
	PlayerSpawn,
	SquadSpawn,
	DefenseTower,
	DecorTree,
	DecorRock,
	DecorFoliage
};

USTRUCT(BlueprintType)
struct FArchonValleyPlacement
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Archon|Valley")
	FName PieceId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Archon|Valley")
	EArchonValleyPiece Piece = EArchonValleyPiece::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Archon|Valley")
	FVector Location = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Archon|Valley")
	FRotator Rotation = FRotator::ZeroRotator;

	// Scale applies to a 100uu engine cube, so Scale=(4,4,4) is a 4m block.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Archon|Valley")
	FVector Scale = FVector::OneVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Archon|Valley")
	FLinearColor DebugColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Archon|Valley")
	int32 TeamId = INDEX_NONE;
};
