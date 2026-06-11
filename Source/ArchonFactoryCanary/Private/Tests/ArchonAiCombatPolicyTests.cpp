#if WITH_DEV_AUTOMATION_TESTS

#include "ArchonAiCombatPolicyLibrary.h"
#include "Misc/AutomationTest.h"

namespace
{
	FArchonAiCombatTuning MakeAiCombatTuning()
	{
		FArchonAiCombatTuning Tuning;
		Tuning.EngageRangeUnits = 4000.0f;
		Tuning.MaxEffectiveFireRangeUnits = 5500.0f;
		Tuning.SeekCoverHealthFraction = 0.40f;
		Tuning.RetreatHealthFraction = 0.15f;
		return Tuning;
	}

	FArchonAiTargetCandidate MakeCandidate(FName TargetId, int32 TeamId, const FVector& Location, bool bIsAlive = true)
	{
		FArchonAiTargetCandidate Candidate;
		Candidate.TargetId = TargetId;
		Candidate.TargetTeamId = TeamId;
		Candidate.Location = Location;
		Candidate.bIsAlive = bIsAlive;
		return Candidate;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonAiCombatSelectTargetPicksNearestEnemyTest,
	"ArchonFactory.AiCombat.SelectTargetPicksNearestEnemy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonAiCombatSelectTargetPicksNearestEnemyTest::RunTest(const FString& Parameters)
{
	TArray<FArchonAiTargetCandidate> Candidates;
	Candidates.Add(MakeCandidate(TEXT("far_enemy"), 0, FVector(3000.0f, 0.0f, 0.0f)));
	Candidates.Add(MakeCandidate(TEXT("near_enemy"), 0, FVector(1200.0f, 0.0f, 0.0f)));
	Candidates.Add(MakeCandidate(TEXT("mid_enemy"), 0, FVector(2000.0f, 0.0f, 0.0f)));

	FArchonAiTargetCandidate Target;
	const bool bSelected = UArchonAiCombatPolicyLibrary::SelectPrimaryTarget(
		1,
		FVector::ZeroVector,
		Candidates,
		MakeAiCombatTuning(),
		Target);

	TestTrue(TEXT("A valid enemy target should be selected"), bSelected);
	TestEqual(TEXT("Nearest valid enemy is selected"), Target.TargetId, FName(TEXT("near_enemy")));
	TestEqual(TEXT("Distance squared is filled by policy"), Target.DistanceSquared, 1200.0f * 1200.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonAiCombatSelectTargetRejectsInvalidTargetsTest,
	"ArchonFactory.AiCombat.SelectTargetRejectsFriendlyDeadAndUnknown",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonAiCombatSelectTargetRejectsInvalidTargetsTest::RunTest(const FString& Parameters)
{
	TArray<FArchonAiTargetCandidate> Candidates;
	Candidates.Add(MakeCandidate(TEXT("friendly"), 1, FVector(800.0f, 0.0f, 0.0f)));
	Candidates.Add(MakeCandidate(TEXT("dead_enemy"), 0, FVector(900.0f, 0.0f, 0.0f), false));
	Candidates.Add(MakeCandidate(TEXT("unknown_team"), INDEX_NONE, FVector(1000.0f, 0.0f, 0.0f)));

	FArchonAiTargetCandidate Target;
	const bool bSelected = UArchonAiCombatPolicyLibrary::SelectPrimaryTarget(
		1,
		FVector::ZeroVector,
		Candidates,
		MakeAiCombatTuning(),
		Target);

	TestFalse(TEXT("Friendly, dead, and unknown targets are rejected"), bSelected);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonAiCombatSelectTargetRejectsOutOfRangeTest,
	"ArchonFactory.AiCombat.SelectTargetRejectsOutOfRange",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonAiCombatSelectTargetRejectsOutOfRangeTest::RunTest(const FString& Parameters)
{
	TArray<FArchonAiTargetCandidate> Candidates;
	Candidates.Add(MakeCandidate(TEXT("too_far"), 0, FVector(6000.0f, 0.0f, 0.0f)));

	FArchonAiTargetCandidate Target;
	const bool bSelected = UArchonAiCombatPolicyLibrary::SelectPrimaryTarget(
		1,
		FVector::ZeroVector,
		Candidates,
		MakeAiCombatTuning(),
		Target);

	TestFalse(TEXT("Targets beyond MaxEffectiveFireRange are rejected"), bSelected);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonAiCombatSelectTargetStableTieBreaksByTargetIdTest,
	"ArchonFactory.AiCombat.SelectTargetStableTieBreaksByTargetId",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonAiCombatSelectTargetStableTieBreaksByTargetIdTest::RunTest(const FString& Parameters)
{
	TArray<FArchonAiTargetCandidate> Candidates;
	Candidates.Add(MakeCandidate(TEXT("zeta"), 0, FVector(2000.0f, 0.0f, 0.0f)));
	Candidates.Add(MakeCandidate(TEXT("alpha"), 0, FVector(0.0f, 2000.0f, 0.0f)));

	FArchonAiTargetCandidate Target;
	const bool bSelected = UArchonAiCombatPolicyLibrary::SelectPrimaryTarget(
		1,
		FVector::ZeroVector,
		Candidates,
		MakeAiCombatTuning(),
		Target);

	TestTrue(TEXT("Tie still selects a target"), bSelected);
	TestEqual(TEXT("Tie breaks lexicographically by target id"), Target.TargetId, FName(TEXT("alpha")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonAiCombatEvaluateDefensiveRangedEngagesTargetInRangeTest,
	"ArchonFactory.AiCombat.EvaluateDefensiveRangedEngagesTargetInRange",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonAiCombatEvaluateDefensiveRangedEngagesTargetInRangeTest::RunTest(const FString& Parameters)
{
	FArchonAiTargetCandidate Target = MakeCandidate(TEXT("player"), 0, FVector(2500.0f, 0.0f, 0.0f));
	Target.DistanceSquared = 2500.0f * 2500.0f;

	const FArchonAiCombatDecision Decision = UArchonAiCombatPolicyLibrary::EvaluateBehavior(
		EArchonAiCombatRole::DefensiveRanged,
		EArchonAiCombatStance::Idle,
		true,
		Target,
		1.0f,
		MakeAiCombatTuning());

	TestEqual(TEXT("Defensive ranged unit engages in-range target"), Decision.NewStance, EArchonAiCombatStance::Engaging);
	TestTrue(TEXT("Defensive ranged unit fires in engage range"), Decision.bShouldFire);
	TestEqual(TEXT("Selected target id is preserved"), Decision.SelectedTargetId, FName(TEXT("player")));
	TestEqual(TEXT("Reason is stable"), Decision.Reason, FName(TEXT("defensive_engage_target")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonAiCombatEvaluateDefensiveRangedHoldsBeyondEngageRangeTest,
	"ArchonFactory.AiCombat.EvaluateDefensiveRangedHoldsBeyondEngageRange",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonAiCombatEvaluateDefensiveRangedHoldsBeyondEngageRangeTest::RunTest(const FString& Parameters)
{
	FArchonAiTargetCandidate Target = MakeCandidate(TEXT("player"), 0, FVector(5000.0f, 0.0f, 0.0f));
	Target.DistanceSquared = 5000.0f * 5000.0f;

	const FArchonAiCombatDecision Decision = UArchonAiCombatPolicyLibrary::EvaluateBehavior(
		EArchonAiCombatRole::DefensiveRanged,
		EArchonAiCombatStance::Idle,
		true,
		Target,
		1.0f,
		MakeAiCombatTuning());

	TestEqual(TEXT("Defensive ranged unit holds when target is outside engage range"), Decision.NewStance, EArchonAiCombatStance::Idle);
	TestFalse(TEXT("Defensive ranged unit does not fire outside engage range"), Decision.bShouldFire);
	TestEqual(TEXT("Reason is stable"), Decision.Reason, FName(TEXT("defensive_hold_range")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonAiCombatEvaluateDefensiveRangedSeeksCoverAtLowHealthTest,
	"ArchonFactory.AiCombat.EvaluateDefensiveRangedSeeksCoverAtLowHealth",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonAiCombatEvaluateDefensiveRangedSeeksCoverAtLowHealthTest::RunTest(const FString& Parameters)
{
	FArchonAiTargetCandidate Target = MakeCandidate(TEXT("player"), 0, FVector(2500.0f, 0.0f, 0.0f));
	Target.DistanceSquared = 2500.0f * 2500.0f;

	const FArchonAiCombatDecision Decision = UArchonAiCombatPolicyLibrary::EvaluateBehavior(
		EArchonAiCombatRole::DefensiveRanged,
		EArchonAiCombatStance::Engaging,
		true,
		Target,
		0.30f,
		MakeAiCombatTuning());

	TestEqual(TEXT("Defensive ranged unit seeks cover below cover threshold"), Decision.NewStance, EArchonAiCombatStance::SeekingCover);
	TestTrue(TEXT("Cover-seeking Bracewright keeps suppressing valid target"), Decision.bShouldFire);
	TestTrue(TEXT("Cover destination is populated"), Decision.bHasMoveDestination);
	TestEqual(TEXT("Reason is stable"), Decision.Reason, FName(TEXT("defensive_seek_cover")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonAiCombatEvaluateDefensiveRangedRetreatsAtCriticalHealthTest,
	"ArchonFactory.AiCombat.EvaluateDefensiveRangedRetreatsAtCriticalHealth",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonAiCombatEvaluateDefensiveRangedRetreatsAtCriticalHealthTest::RunTest(const FString& Parameters)
{
	FArchonAiTargetCandidate Target = MakeCandidate(TEXT("player"), 0, FVector(2500.0f, 0.0f, 0.0f));
	Target.DistanceSquared = 2500.0f * 2500.0f;

	const FArchonAiCombatDecision Decision = UArchonAiCombatPolicyLibrary::EvaluateBehavior(
		EArchonAiCombatRole::DefensiveRanged,
		EArchonAiCombatStance::Engaging,
		true,
		Target,
		0.10f,
		MakeAiCombatTuning());

	TestEqual(TEXT("Defensive ranged unit retreats at critical health"), Decision.NewStance, EArchonAiCombatStance::Retreating);
	TestFalse(TEXT("Critical retreat stops firing"), Decision.bShouldFire);
	TestTrue(TEXT("Retreat destination is populated"), Decision.bHasMoveDestination);
	TestEqual(TEXT("Reason is stable"), Decision.Reason, FName(TEXT("defensive_retreat_critical")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonAiCombatEvaluateScoutNeverFiresAndRetreatsWhenCriticalTest,
	"ArchonFactory.AiCombat.EvaluateScoutNeverFiresAndRetreatsWhenCritical",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonAiCombatEvaluateScoutNeverFiresAndRetreatsWhenCriticalTest::RunTest(const FString& Parameters)
{
	const FArchonAiTargetCandidate Target = MakeCandidate(TEXT("player"), 0, FVector(2500.0f, 0.0f, 0.0f));

	const FArchonAiCombatDecision Observing = UArchonAiCombatPolicyLibrary::EvaluateBehavior(
		EArchonAiCombatRole::ScoutNoWeapon,
		EArchonAiCombatStance::Idle,
		true,
		Target,
		1.0f,
		MakeAiCombatTuning());

	const FArchonAiCombatDecision Retreating = UArchonAiCombatPolicyLibrary::EvaluateBehavior(
		EArchonAiCombatRole::ScoutNoWeapon,
		EArchonAiCombatStance::Scouting,
		true,
		Target,
		0.10f,
		MakeAiCombatTuning());

	TestEqual(TEXT("Healthy scout observes"), Observing.NewStance, EArchonAiCombatStance::Scouting);
	TestFalse(TEXT("Scout never fires while observing"), Observing.bShouldFire);
	TestEqual(TEXT("Critical scout retreats"), Retreating.NewStance, EArchonAiCombatStance::Retreating);
	TestFalse(TEXT("Scout never fires while retreating"), Retreating.bShouldFire);
	return true;
}

#endif
