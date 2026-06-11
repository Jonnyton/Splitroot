#pragma once

#include "CoreMinimal.h"
#include "ArchonAiCombatTypes.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ArchonAiCombatPolicyLibrary.generated.h"

UCLASS()
class ARCHONFACTORYCANARY_API UArchonAiCombatPolicyLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Archon|AiCombat")
	static bool SelectPrimaryTarget(
		int32 SelfTeamId,
		const FVector& SelfLocation,
		const TArray<FArchonAiTargetCandidate>& Candidates,
		const FArchonAiCombatTuning& Tuning,
		FArchonAiTargetCandidate& OutTarget);

	UFUNCTION(BlueprintCallable, Category = "Archon|AiCombat")
	static FArchonAiCombatDecision EvaluateBehavior(
		EArchonAiCombatRole Role,
		EArchonAiCombatStance CurrentStance,
		bool bHasTarget,
		const FArchonAiTargetCandidate& Target,
		float HealthFraction,
		const FArchonAiCombatTuning& Tuning);
};
