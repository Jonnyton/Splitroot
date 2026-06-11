#include "ArchonHealthBarPolicyLibrary.h"

float UArchonHealthBarPolicyLibrary::ComputeHealthBarFillScale(float HealthFraction)
{
	return FMath::Clamp(HealthFraction, 0.0f, 1.0f);
}

FLinearColor UArchonHealthBarPolicyLibrary::ComputeHealthBarColor(float HealthFraction)
{
	const float Clamped = FMath::Clamp(HealthFraction, 0.0f, 1.0f);
	if (Clamped > 2.0f / 3.0f)
	{
		return FLinearColor(0.1f, 0.85f, 0.1f);
	}
	if (Clamped > 1.0f / 3.0f)
	{
		return FLinearColor(0.9f, 0.8f, 0.1f);
	}
	return FLinearColor(0.9f, 0.12f, 0.08f);
}
