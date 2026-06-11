#include "ArchonBotPerceptionPolicyLibrary.h"

bool UArchonBotPerceptionPolicyLibrary::IsInVisionCone(
	const FVector& ViewerLocation,
	const FVector& ViewerForward,
	const FVector& TargetLocation,
	float MaxRange,
	float HalfAngleDegrees)
{
	const FVector ToTarget = TargetLocation - ViewerLocation;
	const float Distance2D = ToTarget.Size2D();
	if (Distance2D > MaxRange || MaxRange <= 0.0f)
	{
		return false;
	}
	if (Distance2D <= KINDA_SMALL_NUMBER)
	{
		return true;
	}
	const FVector ForwardFlat = ViewerForward.GetSafeNormal2D();
	const FVector ToTargetFlat = ToTarget.GetSafeNormal2D();
	if (ForwardFlat.IsNearlyZero() || ToTargetFlat.IsNearlyZero())
	{
		return false;
	}
	const float CosHalfAngle = FMath::Cos(FMath::DegreesToRadians(FMath::Clamp(HalfAngleDegrees, 0.0f, 180.0f)));
	return FVector::DotProduct(ForwardFlat, ToTargetFlat) >= CosHalfAngle;
}

bool UArchonBotPerceptionPolicyLibrary::IsWithinHearing(
	const FVector& ViewerLocation,
	const FVector& TargetLocation,
	float HearingRadius)
{
	return HearingRadius > 0.0f && FVector::Dist2D(ViewerLocation, TargetLocation) <= HearingRadius;
}

bool UArchonBotPerceptionPolicyLibrary::EarnsEyesOnOrigin(
	const FVector& ViewerLocation,
	const FVector& StructureLocation,
	float EyesOnRadius)
{
	return EyesOnRadius > 0.0f && FVector::Dist2D(ViewerLocation, StructureLocation) <= EyesOnRadius;
}
