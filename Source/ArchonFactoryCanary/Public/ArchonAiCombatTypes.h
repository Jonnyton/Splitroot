#pragma once

#include "CoreMinimal.h"
#include "ArchonAiCombatTypes.generated.h"

UENUM(BlueprintType)
enum class EArchonAiCombatRole : uint8
{
	None UMETA(DisplayName = "None"),
	DefensiveRanged UMETA(DisplayName = "Defensive Ranged"),
	AggressiveRanged UMETA(DisplayName = "Aggressive Ranged"),
	ScoutNoWeapon UMETA(DisplayName = "Scout No Weapon"),
	Melee UMETA(DisplayName = "Melee")
};

UENUM(BlueprintType)
enum class EArchonAiCombatStance : uint8
{
	Idle UMETA(DisplayName = "Idle"),
	Scouting UMETA(DisplayName = "Scouting"),
	Engaging UMETA(DisplayName = "Engaging"),
	SeekingCover UMETA(DisplayName = "Seeking Cover"),
	Retreating UMETA(DisplayName = "Retreating")
};

USTRUCT(BlueprintType)
struct FArchonAiTargetCandidate
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Archon|AiCombat")
	int32 TargetTeamId = INDEX_NONE;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Archon|AiCombat")
	FVector Location = FVector::ZeroVector;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Archon|AiCombat")
	bool bIsAlive = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Archon|AiCombat")
	float DistanceSquared = 0.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Archon|AiCombat")
	FName TargetId;
};

USTRUCT(BlueprintType)
struct FArchonAiCombatDecision
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Archon|AiCombat")
	EArchonAiCombatStance NewStance = EArchonAiCombatStance::Idle;

	UPROPERTY(BlueprintReadOnly, Category = "Archon|AiCombat")
	bool bShouldFire = false;

	UPROPERTY(BlueprintReadOnly, Category = "Archon|AiCombat")
	FName SelectedTargetId;

	UPROPERTY(BlueprintReadOnly, Category = "Archon|AiCombat")
	FVector MoveDestination = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Archon|AiCombat")
	bool bHasMoveDestination = false;

	UPROPERTY(BlueprintReadOnly, Category = "Archon|AiCombat")
	FName Reason;
};

USTRUCT(BlueprintType)
struct FArchonAiCombatTuning
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Archon|AiCombat")
	float EngageRangeUnits = 4000.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Archon|AiCombat")
	float MaxEffectiveFireRangeUnits = 5500.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Archon|AiCombat")
	float SeekCoverHealthFraction = 0.40f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Archon|AiCombat")
	float RetreatHealthFraction = 0.15f;
};
