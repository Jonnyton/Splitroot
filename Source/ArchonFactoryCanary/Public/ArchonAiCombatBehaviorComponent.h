#pragma once

#include "CoreMinimal.h"
#include "ArchonAiCombatTypes.h"
#include "Components/ActorComponent.h"
#include "ArchonAiCombatBehaviorComponent.generated.h"

UCLASS(ClassGroup=(Archon), Blueprintable, meta=(BlueprintSpawnableComponent))
class ARCHONFACTORYCANARY_API UArchonAiCombatBehaviorComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UArchonAiCombatBehaviorComponent();

	UFUNCTION(BlueprintCallable, Category = "Archon|AiCombat")
	void ConfigureBehavior(int32 InTeamId, EArchonAiCombatRole InRole);

	UFUNCTION(BlueprintCallable, Category = "Archon|AiCombat")
	FArchonAiCombatDecision EvaluateCombat(
		const FVector& SelfLocation,
		const TArray<FArchonAiTargetCandidate>& Candidates,
		float HealthFraction);

	UFUNCTION(BlueprintPure, Category = "Archon|AiCombat")
	int32 GetTeamId() const { return TeamId; }

	UFUNCTION(BlueprintPure, Category = "Archon|AiCombat")
	EArchonAiCombatRole GetRole() const { return Role; }

	UFUNCTION(BlueprintPure, Category = "Archon|AiCombat")
	EArchonAiCombatStance GetCurrentStance() const { return CurrentStance; }

	UFUNCTION(BlueprintPure, Category = "Archon|AiCombat")
	FArchonAiCombatTuning GetTuning() const { return Tuning; }

	UFUNCTION(BlueprintCallable, Category = "Archon|AiCombat")
	void SetTuning(const FArchonAiCombatTuning& InTuning);

	UFUNCTION(BlueprintPure, Category = "Archon|AiCombat")
	FArchonAiCombatDecision GetLastDecision() const { return LastDecision; }

private:
	UPROPERTY(EditAnywhere, Category = "Archon|AiCombat")
	int32 TeamId = INDEX_NONE;

	UPROPERTY(EditAnywhere, Category = "Archon|AiCombat")
	EArchonAiCombatRole Role = EArchonAiCombatRole::None;

	UPROPERTY(VisibleAnywhere, Category = "Archon|AiCombat")
	EArchonAiCombatStance CurrentStance = EArchonAiCombatStance::Idle;

	UPROPERTY(EditAnywhere, Category = "Archon|AiCombat")
	FArchonAiCombatTuning Tuning;

	UPROPERTY(VisibleAnywhere, Category = "Archon|AiCombat")
	FArchonAiCombatDecision LastDecision;
};
