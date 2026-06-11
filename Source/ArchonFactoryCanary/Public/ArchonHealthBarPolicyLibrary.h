#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ArchonHealthBarPolicyLibrary.generated.h"

/**
 * Pure policy for world-space structure health bars (WC3 readability:
 * a glance at any building answers "how is the siege going"). Render
 * components feed in a health fraction; these functions decide fill
 * and color. No actor code makes those decisions.
 */
UCLASS()
class ARCHONFACTORYCANARY_API UArchonHealthBarPolicyLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// Fraction of the bar's full width the fill occupies, clamped [0,1].
	UFUNCTION(BlueprintPure, Category = "Archon|HealthBar")
	static float ComputeHealthBarFillScale(float HealthFraction);

	// Green above 2/3, yellow above 1/3, red below (banded, not lerped:
	// bands read at distance better than a smooth ramp).
	UFUNCTION(BlueprintPure, Category = "Archon|HealthBar")
	static FLinearColor ComputeHealthBarColor(float HealthFraction);
};
