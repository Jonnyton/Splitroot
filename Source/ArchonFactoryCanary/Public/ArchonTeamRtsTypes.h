#pragma once

#include "CoreMinimal.h"
#include "ArchonTeamRtsTypes.generated.h"

UENUM(BlueprintType)
enum class EArchonRtsOrderKind : uint8
{
	MoveSquad UMETA(DisplayName = "Move Squad"),
	AttackTarget UMETA(DisplayName = "Attack Target"),
	BuildStructure UMETA(DisplayName = "Build Structure"),
	TrainUnit UMETA(DisplayName = "Train Unit"),
	UseFactionPower UMETA(DisplayName = "Use Faction Power"),
	SetRallyPoint UMETA(DisplayName = "Set Rally Point")
};

USTRUCT(BlueprintType)
struct FArchonRtsCommandIntent
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Archon|RTS")
	int32 TeamId = INDEX_NONE;

	UPROPERTY(BlueprintReadWrite, Category = "Archon|RTS")
	int32 IssuingPlayerId = INDEX_NONE;

	UPROPERTY(BlueprintReadWrite, Category = "Archon|RTS")
	EArchonRtsOrderKind OrderKind = EArchonRtsOrderKind::MoveSquad;

	UPROPERTY(BlueprintReadWrite, Category = "Archon|RTS")
	FName SubjectId;

	UPROPERTY(BlueprintReadWrite, Category = "Archon|RTS")
	FName TargetId;

	UPROPERTY(BlueprintReadWrite, Category = "Archon|RTS")
	FVector TargetLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadWrite, Category = "Archon|RTS")
	bool bTargetLocationValid = false;
};

USTRUCT(BlueprintType)
struct FArchonRtsAuthorityDecision
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Archon|RTS")
	bool bAccepted = false;

	UPROPERTY(BlueprintReadOnly, Category = "Archon|RTS")
	bool bMutatesTeamState = false;

	UPROPERTY(BlueprintReadOnly, Category = "Archon|RTS")
	bool bReplicatesToTeam = false;

	UPROPERTY(BlueprintReadOnly, Category = "Archon|RTS")
	bool bRequiresCommanderToken = false;

	UPROPERTY(BlueprintReadOnly, Category = "Archon|RTS")
	int32 PreviousSequence = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Archon|RTS")
	int32 FinalSequence = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Archon|RTS")
	FName Reason;
};
