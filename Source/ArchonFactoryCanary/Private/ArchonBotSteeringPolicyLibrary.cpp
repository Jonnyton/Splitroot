#include "ArchonBotSteeringPolicyLibrary.h"

FArchonBotSteeringDecision UArchonBotSteeringPolicyLibrary::ComputeSteering(
	const FVector& BotLocation,
	const FVector& ObjectiveLocation,
	bool bObstacleAhead,
	const FVector& ObstacleNormal,
	float AcceptanceRadius)
{
	FArchonBotSteeringDecision Decision;

	const FVector ToObjective = ObjectiveLocation - BotLocation;
	if (ToObjective.Size2D() <= FMath::Max(0.0f, AcceptanceRadius))
	{
		Decision.bAtObjective = true;
		Decision.MoveDirection = FVector::ZeroVector;
		return Decision;
	}

	FVector Seek = ToObjective.GetSafeNormal2D();
	if (Seek.IsNearlyZero())
	{
		Seek = FVector::ForwardVector;
	}

	if (!bObstacleAhead)
	{
		Decision.MoveDirection = Seek;
		return Decision;
	}

	// Slide: remove the into-the-wall component, keep the along-the-wall
	// one. If the seek direction is dead-on into the wall, pick the wall
	// tangent that points most toward the objective.
	Decision.bAvoidingObstacle = true;
	const FVector Normal2D = FVector(ObstacleNormal.X, ObstacleNormal.Y, 0.0).GetSafeNormal2D();
	FVector Slide = Seek - FVector::DotProduct(Seek, Normal2D) * Normal2D;
	if (Slide.Size2D() < 0.05f)
	{
		const FVector TangentA(-Normal2D.Y, Normal2D.X, 0.0);
		const FVector TangentB(Normal2D.Y, -Normal2D.X, 0.0);
		Slide = FVector::DotProduct(TangentA, Seek) >= FVector::DotProduct(TangentB, Seek) ? TangentA : TangentB;
	}
	Decision.MoveDirection = Slide.GetSafeNormal2D();
	return Decision;
}

FVector UArchonBotSteeringPolicyLibrary::ComputeUnstickEscapeTarget(
	const FVector& BotLocation,
	const FVector& ObjectiveLocation,
	int32 AttemptIndex,
	float LateralDistance,
	float BackstepDistance)
{
	const TArray<FVector> Candidates = ComputeUnstickEscapeCandidates(
		BotLocation,
		ObjectiveLocation,
		AttemptIndex,
		LateralDistance,
		BackstepDistance);
	return Candidates.Num() > 0 ? Candidates[0] : BotLocation;
}

TArray<FVector> UArchonBotSteeringPolicyLibrary::ComputeUnstickEscapeCandidates(
	const FVector& BotLocation,
	const FVector& ObjectiveLocation,
	int32 AttemptIndex,
	float LateralDistance,
	float BackstepDistance)
{
	FVector ToObjective = (ObjectiveLocation - BotLocation).GetSafeNormal2D();
	if (ToObjective.IsNearlyZero())
	{
		ToObjective = FVector::ForwardVector;
	}

	const FVector LateralA(-ToObjective.Y, ToObjective.X, 0.0f);
	const float Side = (FMath::Abs(AttemptIndex) % 2) == 0 ? 1.0f : -1.0f;
	const int32 AttemptBand = FMath::Clamp(FMath::Abs(AttemptIndex) / 2, 0, 5);
	const float Expansion = 1.0f + static_cast<float>(AttemptBand) * 0.25f;
	const float Lateral = FMath::Max(0.0f, LateralDistance);
	const float Backstep = FMath::Max(0.0f, BackstepDistance);

	TArray<FVector> Candidates;
	Candidates.Reserve(6);
	Candidates.Add(BotLocation + LateralA * Side * Lateral * Expansion - ToObjective * Backstep * Expansion);
	Candidates.Add(BotLocation - LateralA * Side * Lateral * Expansion - ToObjective * Backstep * Expansion);
	Candidates.Add(BotLocation + LateralA * Side * Lateral * (Expansion + 0.35f));
	Candidates.Add(BotLocation - ToObjective * Backstep * (Expansion + 0.75f));
	Candidates.Add(BotLocation + LateralA * Side * Lateral * 0.75f + ToObjective * Backstep * 0.75f);
	Candidates.Add(BotLocation - LateralA * Side * Lateral * 0.75f + ToObjective * Backstep * 0.75f);
	return Candidates;
}

FVector UArchonBotSteeringPolicyLibrary::ComputeObjectiveStandOffTarget(
	const FVector& BotLocation,
	const FVector& ObjectiveLocation,
	int32 BotIndex,
	float StandOffDistance,
	float LaneSpacing)
{
	FVector FromObjective = (BotLocation - ObjectiveLocation).GetSafeNormal2D();
	if (FromObjective.IsNearlyZero())
	{
		const float Angle = (2.0f * PI * (FMath::Abs(BotIndex) % 8)) / 8.0f;
		FromObjective = FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.0f);
	}

	const int32 Slot = FMath::Abs(BotIndex) % 5;
	const int32 Ring = (Slot + 1) / 2;
	const float Side = (Slot % 2) == 1 ? 1.0f : -1.0f;
	const float LaneOffset = Slot == 0
		? 0.0f
		: Side * Ring * FMath::Max(0.0f, LaneSpacing);
	const FVector Lateral(-FromObjective.Y, FromObjective.X, 0.0f);

	FVector Target = ObjectiveLocation
		+ FromObjective * FMath::Max(0.0f, StandOffDistance)
		+ Lateral * LaneOffset;
	Target.Z = ObjectiveLocation.Z;
	return Target;
}

bool UArchonBotSteeringPolicyLibrary::ShouldAbandonRouteWaypointAfterStalls(
	int32 RouteStallAttempts,
	int32 MaxRouteStallAttempts)
{
	if (MaxRouteStallAttempts <= 0)
	{
		return false;
	}
	return RouteStallAttempts >= MaxRouteStallAttempts;
}

int32 UArchonBotSteeringPolicyLibrary::ComputeObjectiveLaneShift(
	int32 ObjectiveStallAttempts,
	int32 AttemptsPerLaneShift,
	int32 MaxLaneShift)
{
	if (ObjectiveStallAttempts <= 0 || AttemptsPerLaneShift <= 0 || MaxLaneShift <= 0)
	{
		return 0;
	}
	return FMath::Clamp(ObjectiveStallAttempts / AttemptsPerLaneShift, 0, MaxLaneShift);
}
