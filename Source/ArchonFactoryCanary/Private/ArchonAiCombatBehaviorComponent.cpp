#include "ArchonAiCombatBehaviorComponent.h"
#include "ArchonAiCombatPolicyLibrary.h"

UArchonAiCombatBehaviorComponent::UArchonAiCombatBehaviorComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UArchonAiCombatBehaviorComponent::ConfigureBehavior(int32 InTeamId, EArchonAiCombatRole InRole)
{
	TeamId = InTeamId;
	Role = InRole;
	CurrentStance = InRole == EArchonAiCombatRole::ScoutNoWeapon
		? EArchonAiCombatStance::Scouting
		: EArchonAiCombatStance::Idle;
	LastDecision = FArchonAiCombatDecision();
	LastDecision.NewStance = CurrentStance;
}

FArchonAiCombatDecision UArchonAiCombatBehaviorComponent::EvaluateCombat(
	const FVector& SelfLocation,
	const TArray<FArchonAiTargetCandidate>& Candidates,
	float HealthFraction)
{
	FArchonAiTargetCandidate Target;
	const bool bHasTarget = UArchonAiCombatPolicyLibrary::SelectPrimaryTarget(
		TeamId,
		SelfLocation,
		Candidates,
		Tuning,
		Target);

	LastDecision = UArchonAiCombatPolicyLibrary::EvaluateBehavior(
		Role,
		CurrentStance,
		bHasTarget,
		Target,
		HealthFraction,
		Tuning);
	CurrentStance = LastDecision.NewStance;
	return LastDecision;
}

void UArchonAiCombatBehaviorComponent::SetTuning(const FArchonAiCombatTuning& InTuning)
{
	Tuning = InTuning;
	Tuning.EngageRangeUnits = FMath::Max(0.0f, Tuning.EngageRangeUnits);
	Tuning.MaxEffectiveFireRangeUnits = FMath::Max(0.0f, Tuning.MaxEffectiveFireRangeUnits);
	Tuning.SeekCoverHealthFraction = FMath::Clamp(Tuning.SeekCoverHealthFraction, 0.0f, 1.0f);
	Tuning.RetreatHealthFraction = FMath::Clamp(Tuning.RetreatHealthFraction, 0.0f, Tuning.SeekCoverHealthFraction);
}
