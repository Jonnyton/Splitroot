#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ArchonBotSteeringPolicyLibrary.generated.h"

USTRUCT(BlueprintType)
struct FArchonBotSteeringDecision
{
	GENERATED_BODY()

	// Unit-length world direction the bot should move this frame.
	UPROPERTY(BlueprintReadOnly, Category = "Archon|Bot")
	FVector MoveDirection = FVector::ForwardVector;

	UPROPERTY(BlueprintReadOnly, Category = "Archon|Bot")
	bool bAvoidingObstacle = false;

	UPROPERTY(BlueprintReadOnly, Category = "Archon|Bot")
	bool bAtObjective = false;
};

/**
 * Pure steering for v1 bot players (Jonathan 2026-06-10, decision 9):
 * run to the enemy base, path around obstacles. Deliberately simple —
 * seek the objective; when something blocks the way, slide along the
 * blocking surface until the path clears. NavMesh can replace this
 * later without touching the brain.
 */
UCLASS()
class ARCHONFACTORYCANARY_API UArchonBotSteeringPolicyLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Archon|Bot")
	static FArchonBotSteeringDecision ComputeSteering(
		const FVector& BotLocation,
		const FVector& ObjectiveLocation,
		bool bObstacleAhead,
		const FVector& ObstacleNormal,
		float AcceptanceRadius);

	UFUNCTION(BlueprintPure, Category = "Archon|Bot")
	static FVector ComputeUnstickEscapeTarget(
		const FVector& BotLocation,
		const FVector& ObjectiveLocation,
		int32 AttemptIndex,
		float LateralDistance,
		float BackstepDistance);

	UFUNCTION(BlueprintPure, Category = "Archon|Bot")
	static TArray<FVector> ComputeUnstickEscapeCandidates(
		const FVector& BotLocation,
		const FVector& ObjectiveLocation,
		int32 AttemptIndex,
		float LateralDistance,
		float BackstepDistance);

	UFUNCTION(BlueprintPure, Category = "Archon|Bot")
	static FVector ComputeObjectiveStandOffTarget(
		const FVector& BotLocation,
		const FVector& ObjectiveLocation,
		int32 BotIndex,
		float StandOffDistance,
		float LaneSpacing);

	UFUNCTION(BlueprintPure, Category = "Archon|Bot")
	static bool ShouldAbandonRouteWaypointAfterStalls(
		int32 RouteStallAttempts,
		int32 MaxRouteStallAttempts);

	UFUNCTION(BlueprintPure, Category = "Archon|Bot")
	static int32 ComputeObjectiveLaneShift(
		int32 ObjectiveStallAttempts,
		int32 AttemptsPerLaneShift,
		int32 MaxLaneShift);
};
