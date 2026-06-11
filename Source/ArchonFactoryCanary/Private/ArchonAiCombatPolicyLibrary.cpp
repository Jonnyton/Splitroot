#include "ArchonAiCombatPolicyLibrary.h"

namespace
{
	bool TargetIdSortsBefore(const FName& Candidate, const FName& CurrentBest)
	{
		return Candidate.ToString().Compare(CurrentBest.ToString(), ESearchCase::CaseSensitive) < 0;
	}

	FVector MakeRetreatDestination(const FArchonAiTargetCandidate& Target)
	{
		const FVector AwayFromTarget = (-Target.Location).GetSafeNormal();
		return Target.Location + (AwayFromTarget.IsNearlyZero() ? FVector(-1200.0f, 0.0f, 0.0f) : AwayFromTarget * 1200.0f);
	}

	FVector MakeCoverDestination(const FArchonAiTargetCandidate& Target)
	{
		return Target.Location + FVector(-600.0f, 350.0f, 0.0f);
	}

	FArchonAiCombatDecision MakeMoveDecision(
		EArchonAiCombatStance Stance,
		bool bShouldFire,
		const FArchonAiTargetCandidate& Target,
		const FVector& MoveDestination,
		FName Reason)
	{
		FArchonAiCombatDecision Decision;
		Decision.NewStance = Stance;
		Decision.bShouldFire = bShouldFire;
		Decision.SelectedTargetId = Target.TargetId;
		Decision.MoveDestination = MoveDestination;
		Decision.bHasMoveDestination = true;
		Decision.Reason = Reason;
		return Decision;
	}
}

bool UArchonAiCombatPolicyLibrary::SelectPrimaryTarget(
	int32 SelfTeamId,
	const FVector& SelfLocation,
	const TArray<FArchonAiTargetCandidate>& Candidates,
	const FArchonAiCombatTuning& Tuning,
	FArchonAiTargetCandidate& OutTarget)
{
	const float MaxRange = FMath::Max(0.0f, Tuning.MaxEffectiveFireRangeUnits);
	const float MaxRangeSquared = MaxRange * MaxRange;
	bool bHasBest = false;
	FArchonAiTargetCandidate BestTarget;

	for (const FArchonAiTargetCandidate& Candidate : Candidates)
	{
		if (!Candidate.bIsAlive || Candidate.TargetTeamId == INDEX_NONE || Candidate.TargetTeamId == SelfTeamId)
		{
			continue;
		}

		FArchonAiTargetCandidate ScoredCandidate = Candidate;
		ScoredCandidate.DistanceSquared = FVector::DistSquared(SelfLocation, Candidate.Location);
		if (ScoredCandidate.DistanceSquared > MaxRangeSquared)
		{
			continue;
		}

		const bool bBetterDistance =
			!bHasBest || ScoredCandidate.DistanceSquared < BestTarget.DistanceSquared;
		const bool bStableTieBreak =
			bHasBest &&
			FMath::IsNearlyEqual(ScoredCandidate.DistanceSquared, BestTarget.DistanceSquared) &&
			TargetIdSortsBefore(ScoredCandidate.TargetId, BestTarget.TargetId);

		if (bBetterDistance || bStableTieBreak)
		{
			BestTarget = ScoredCandidate;
			bHasBest = true;
		}
	}

	if (!bHasBest)
	{
		OutTarget = FArchonAiTargetCandidate();
		return false;
	}

	OutTarget = BestTarget;
	return true;
}

FArchonAiCombatDecision UArchonAiCombatPolicyLibrary::EvaluateBehavior(
	EArchonAiCombatRole Role,
	EArchonAiCombatStance CurrentStance,
	bool bHasTarget,
	const FArchonAiTargetCandidate& Target,
	float HealthFraction,
	const FArchonAiCombatTuning& Tuning)
{
	const float SafeHealthFraction = FMath::Clamp(HealthFraction, 0.0f, 1.0f);

	if (Role == EArchonAiCombatRole::ScoutNoWeapon)
	{
		if (SafeHealthFraction <= Tuning.RetreatHealthFraction)
		{
			return MakeMoveDecision(
				EArchonAiCombatStance::Retreating,
				false,
				Target,
				MakeRetreatDestination(Target),
				TEXT("scout_retreat_on_low_health"));
		}

		FArchonAiCombatDecision Decision;
		Decision.NewStance = EArchonAiCombatStance::Scouting;
		Decision.bShouldFire = false;
		Decision.SelectedTargetId = bHasTarget ? Target.TargetId : NAME_None;
		Decision.Reason = TEXT("scout_observing");
		return Decision;
	}

	if (Role == EArchonAiCombatRole::DefensiveRanged)
	{
		if (SafeHealthFraction <= Tuning.RetreatHealthFraction)
		{
			return MakeMoveDecision(
				EArchonAiCombatStance::Retreating,
				false,
				Target,
				MakeRetreatDestination(Target),
				TEXT("defensive_retreat_critical"));
		}

		if (SafeHealthFraction <= Tuning.SeekCoverHealthFraction)
		{
			return MakeMoveDecision(
				EArchonAiCombatStance::SeekingCover,
				bHasTarget,
				Target,
				MakeCoverDestination(Target),
				TEXT("defensive_seek_cover"));
		}

		if (bHasTarget && Target.DistanceSquared <= FMath::Square(Tuning.EngageRangeUnits))
		{
			FArchonAiCombatDecision Decision;
			Decision.NewStance = EArchonAiCombatStance::Engaging;
			Decision.bShouldFire = true;
			Decision.SelectedTargetId = Target.TargetId;
			Decision.Reason = TEXT("defensive_engage_target");
			return Decision;
		}

		FArchonAiCombatDecision Decision;
		Decision.NewStance = CurrentStance == EArchonAiCombatStance::SeekingCover ? EArchonAiCombatStance::SeekingCover : EArchonAiCombatStance::Idle;
		Decision.bShouldFire = false;
		Decision.SelectedTargetId = bHasTarget ? Target.TargetId : NAME_None;
		Decision.Reason = bHasTarget ? FName(TEXT("defensive_hold_range")) : FName(TEXT("defensive_no_target"));
		return Decision;
	}

	if (Role == EArchonAiCombatRole::AggressiveRanged)
	{
		FArchonAiCombatDecision Decision;
		Decision.NewStance = bHasTarget ? EArchonAiCombatStance::Engaging : EArchonAiCombatStance::Idle;
		Decision.bShouldFire = bHasTarget;
		Decision.SelectedTargetId = bHasTarget ? Target.TargetId : NAME_None;
		Decision.Reason = bHasTarget ? FName(TEXT("aggressive_engage_target")) : FName(TEXT("aggressive_no_target"));
		return Decision;
	}

	FArchonAiCombatDecision Decision;
	Decision.NewStance = EArchonAiCombatStance::Idle;
	Decision.bShouldFire = false;
	Decision.SelectedTargetId = bHasTarget ? Target.TargetId : NAME_None;
	Decision.Reason = TEXT("unsupported_role");
	return Decision;
}
