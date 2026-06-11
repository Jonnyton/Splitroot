#include "ArchonMatchPolicyLibrary.h"
#include "ArchonMatchTypes.h"
#include "Misc/AutomationTest.h"

namespace
{
	FArchonMatchConfig TestConfig()
	{
		FArchonMatchConfig Config;
		Config.WarmupSeconds = 10.0f;
		Config.MatchTimeLimitSeconds = 600.0f;
		Config.ScoreboardSeconds = 12.0f;
		Config.SupplyTickIntervalSeconds = 10.0f;
		Config.SupplyPerSitePerTick = 25;
		Config.BaseSupplyPerTick = 5;
		Config.SiteCaptureSeconds = 8.0f;
		Config.SiteCaptureRadius = 900.0f;
		return Config;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonMatchClockPhaseFlowTest,
	"ArchonFactory.Match.ClockWalksWarmupLiveEndTravel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FArchonMatchClockPhaseFlowTest::RunTest(const FString& Parameters)
{
	const FArchonMatchConfig Config = TestConfig();

	FArchonMatchClock Clock;
	TestEqual(TEXT("Starts in warmup"), Clock.Phase, EArchonMatchPhase::Warmup);

	Clock = UArchonMatchPolicyLibrary::TickMatchClock(Clock, 4.0f, Config, EArchonMatchWinner::Undecided);
	TestEqual(TEXT("Still warming up"), Clock.Phase, EArchonMatchPhase::Warmup);
	TestTrue(TEXT("Warmup time accumulates"), FMath::IsNearlyEqual(Clock.PhaseSeconds, 4.0f));

	Clock = UArchonMatchPolicyLibrary::TickMatchClock(Clock, 6.0f, Config, EArchonMatchWinner::Undecided);
	TestEqual(TEXT("Warmup ends into live"), Clock.Phase, EArchonMatchPhase::Live);
	TestTrue(TEXT("Phase clock resets on transition"), FMath::IsNearlyEqual(Clock.PhaseSeconds, 0.0f));

	Clock = UArchonMatchPolicyLibrary::TickMatchClock(Clock, 300.0f, Config, EArchonMatchWinner::Undecided);
	TestEqual(TEXT("Live holds while undecided"), Clock.Phase, EArchonMatchPhase::Live);

	Clock = UArchonMatchPolicyLibrary::TickMatchClock(Clock, 1.0f, Config, EArchonMatchWinner::TeamA);
	TestEqual(TEXT("Winner ends the match"), Clock.Phase, EArchonMatchPhase::MatchEnd);

	Clock = UArchonMatchPolicyLibrary::TickMatchClock(Clock, 5.0f, Config, EArchonMatchWinner::TeamA);
	TestEqual(TEXT("Scoreboard holds"), Clock.Phase, EArchonMatchPhase::MatchEnd);

	Clock = UArchonMatchPolicyLibrary::TickMatchClock(Clock, 7.5f, Config, EArchonMatchWinner::TeamA);
	TestEqual(TEXT("Scoreboard expires into travel"), Clock.Phase, EArchonMatchPhase::Traveling);

	Clock = UArchonMatchPolicyLibrary::TickMatchClock(Clock, 100.0f, Config, EArchonMatchWinner::TeamA);
	TestEqual(TEXT("Traveling is absorbing"), Clock.Phase, EArchonMatchPhase::Traveling);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonMatchDefaultCadenceTest,
	"ArchonFactory.Match.DefaultCadenceIsFifteenMinuteLiveCycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FArchonMatchDefaultCadenceTest::RunTest(const FString& Parameters)
{
	const FArchonMatchConfig Config;
	TestTrue(TEXT("Default warmup is 15 seconds"), FMath::IsNearlyEqual(Config.WarmupSeconds, 15.0f));
	TestTrue(TEXT("Default live time limit is 900 seconds"), FMath::IsNearlyEqual(Config.MatchTimeLimitSeconds, 900.0f));
	TestTrue(TEXT("Default scoreboard is 15 seconds"), FMath::IsNearlyEqual(Config.ScoreboardSeconds, 15.0f));
	TestTrue(
		TEXT("Default natural cycle is 930 seconds"),
		FMath::IsNearlyEqual(Config.WarmupSeconds + Config.MatchTimeLimitSeconds + Config.ScoreboardSeconds, 930.0f));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonMatchWinnerEvaluationTest,
	"ArchonFactory.Match.CoreDestructionAndTimeLimitDecideWinner",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FArchonMatchWinnerEvaluationTest::RunTest(const FString& Parameters)
{
	TestEqual(
		TEXT("Healthy cores, clock running: undecided"),
		UArchonMatchPolicyLibrary::EvaluateWinner(1000.0f, 1000.0f, false, 0, 0),
		EArchonMatchWinner::Undecided);
	TestEqual(
		TEXT("Core A destroyed: Team B wins"),
		UArchonMatchPolicyLibrary::EvaluateWinner(0.0f, 500.0f, false, 9999, 0),
		EArchonMatchWinner::TeamB);
	TestEqual(
		TEXT("Core B destroyed: Team A wins"),
		UArchonMatchPolicyLibrary::EvaluateWinner(500.0f, 0.0f, false, 0, 9999),
		EArchonMatchWinner::TeamA);
	TestEqual(
		TEXT("Mutual destruction: draw"),
		UArchonMatchPolicyLibrary::EvaluateWinner(0.0f, 0.0f, false, 10, 20),
		EArchonMatchWinner::Draw);
	TestEqual(
		TEXT("Time limit: higher points win"),
		UArchonMatchPolicyLibrary::EvaluateWinner(800.0f, 900.0f, true, 150, 90),
		EArchonMatchWinner::TeamA);
	TestEqual(
		TEXT("Time limit: points tie is a draw"),
		UArchonMatchPolicyLibrary::EvaluateWinner(800.0f, 900.0f, true, 100, 100),
		EArchonMatchWinner::Draw);
	TestEqual(
		TEXT("Core destruction outranks points at the bell"),
		UArchonMatchPolicyLibrary::EvaluateWinner(0.0f, 100.0f, true, 9999, 1),
		EArchonMatchWinner::TeamB);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonMatchSitesTiebreakTest,
	"ArchonFactory.Match.TimeLimitTiebreaksBySitesThenDraw",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FArchonMatchSitesTiebreakTest::RunTest(const FString& Parameters)
{
	// The all-AI stalemate shape: towers gated the cores, 0-0 points at
	// the bell. Map control must decide before a Draw is conceded.
	TestEqual(
		TEXT("Points tied: more sites wins (A)"),
		UArchonMatchPolicyLibrary::EvaluateWinner(800.0f, 900.0f, true, 0, 0, 2, 1),
		EArchonMatchWinner::TeamA);
	TestEqual(
		TEXT("Points tied: more sites wins (B)"),
		UArchonMatchPolicyLibrary::EvaluateWinner(800.0f, 900.0f, true, 100, 100, 0, 3),
		EArchonMatchWinner::TeamB);
	TestEqual(
		TEXT("Points AND sites tied: honest draw"),
		UArchonMatchPolicyLibrary::EvaluateWinner(800.0f, 900.0f, true, 100, 100, 1, 1),
		EArchonMatchWinner::Draw);
	TestEqual(
		TEXT("Points outrank sites"),
		UArchonMatchPolicyLibrary::EvaluateWinner(800.0f, 900.0f, true, 10, 5, 0, 3),
		EArchonMatchWinner::TeamA);
	TestEqual(
		TEXT("Sites never decide before the bell"),
		UArchonMatchPolicyLibrary::EvaluateWinner(800.0f, 900.0f, false, 0, 0, 3, 0),
		EArchonMatchWinner::Undecided);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonMatchTowerDamagePointsTest,
	"ArchonFactory.Match.TowerDamageFeedsPointsAtDiscount",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FArchonMatchTowerDamagePointsTest::RunTest(const FString& Parameters)
{
	FArchonMatchConfig Config = TestConfig();
	Config.TowerDamagePointScale = 0.2f;

	TestEqual(
		TEXT("5 dmg arrow at 0.2 scale = 1 point"),
		UArchonMatchPolicyLibrary::ComputeTowerDamagePoints(5.0f, Config), 1);
	TestEqual(
		TEXT("Full 600hp tower razed = 120 points"),
		UArchonMatchPolicyLibrary::ComputeTowerDamagePoints(600.0f, Config), 120);
	TestEqual(
		TEXT("Negative damage pays nothing"),
		UArchonMatchPolicyLibrary::ComputeTowerDamagePoints(-50.0f, Config), 0);

	Config.TowerDamagePointScale = 0.0f;
	TestEqual(
		TEXT("Zero scale disables siege scoring"),
		UArchonMatchPolicyLibrary::ComputeTowerDamagePoints(600.0f, Config), 0);
	Config.TowerDamagePointScale = -1.0f;
	TestEqual(
		TEXT("Negative scale clamps to zero"),
		UArchonMatchPolicyLibrary::ComputeTowerDamagePoints(600.0f, Config), 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonMatchReinforcementAffordTest,
	"ArchonFactory.Match.ReinforcementAffordabilityGate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FArchonMatchReinforcementAffordTest::RunTest(const FString& Parameters)
{
	FArchonMatchConfig Config = TestConfig();
	Config.ReinforcementSquadSupplyCost = 150;

	TestTrue(TEXT("Bank above cost affords"),
		UArchonMatchPolicyLibrary::CanAffordReinforcement(200, Config));
	TestTrue(TEXT("Exact cost affords"),
		UArchonMatchPolicyLibrary::CanAffordReinforcement(150, Config));
	TestFalse(TEXT("Below cost cannot afford"),
		UArchonMatchPolicyLibrary::CanAffordReinforcement(149, Config));

	Config.ReinforcementSquadSupplyCost = 0;
	TestFalse(TEXT("Zero cost disables purchasing"),
		UArchonMatchPolicyLibrary::CanAffordReinforcement(99999, Config));
	Config.ReinforcementSquadSupplyCost = -50;
	TestFalse(TEXT("Negative cost disables purchasing"),
		UArchonMatchPolicyLibrary::CanAffordReinforcement(99999, Config));

	// Lazy-banker auto-spend threshold.
	Config.ReinforcementSquadSupplyCost = 150;
	Config.AutoReinforceBankMultiple = 2.0f;
	TestFalse(TEXT("Auto spender waits below the banking threshold"),
		UArchonMatchPolicyLibrary::ShouldAutoReinforce(299, Config));
	TestTrue(TEXT("Auto spender buys at the banking threshold"),
		UArchonMatchPolicyLibrary::ShouldAutoReinforce(300, Config));
	Config.AutoReinforceBankMultiple = 0.0f;
	TestFalse(TEXT("Multiple clamps to at least 1x cost (below)"),
		UArchonMatchPolicyLibrary::ShouldAutoReinforce(149, Config));
	TestTrue(TEXT("Multiple clamps to at least 1x cost (at)"),
		UArchonMatchPolicyLibrary::ShouldAutoReinforce(150, Config));
	Config.ReinforcementSquadSupplyCost = 0;
	TestFalse(TEXT("Zero cost disables auto-spend too"),
		UArchonMatchPolicyLibrary::ShouldAutoReinforce(99999, Config));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonMatchSiteCaptureTest,
	"ArchonFactory.Match.SiteCaptureByPresence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FArchonMatchSiteCaptureTest::RunTest(const FString& Parameters)
{
	const FArchonMatchConfig Config = TestConfig();

	// Sole presence captures after SiteCaptureSeconds.
	FArchonResourceSiteState Site;
	Site = UArchonMatchPolicyLibrary::TickSiteCapture(Site, 2, 0, 5.0f, Config);
	TestEqual(TEXT("Capture in progress for team A"), Site.CapturingTeam, 0);
	TestEqual(TEXT("Still neutral mid-capture"), Site.OwningTeam, static_cast<int32>(INDEX_NONE));
	Site = UArchonMatchPolicyLibrary::TickSiteCapture(Site, 2, 0, 3.0f, Config);
	TestEqual(TEXT("Team A flips the site"), Site.OwningTeam, 0);
	TestEqual(TEXT("Progress clears after flip"), Site.CapturingTeam, static_cast<int32>(INDEX_NONE));

	// Contested presence freezes progress.
	FArchonResourceSiteState Contested;
	Contested = UArchonMatchPolicyLibrary::TickSiteCapture(Contested, 1, 0, 4.0f, Config);
	const float FrozenProgress = Contested.CaptureProgressSeconds;
	Contested = UArchonMatchPolicyLibrary::TickSiteCapture(Contested, 1, 3, 60.0f, Config);
	TestTrue(
		TEXT("Contested site freezes capture progress"),
		FMath::IsNearlyEqual(Contested.CaptureProgressSeconds, FrozenProgress));
	TestEqual(TEXT("Contested site stays neutral"), Contested.OwningTeam, static_cast<int32>(INDEX_NONE));

	// Abandoning decays progress; ownership persists while empty.
	FArchonResourceSiteState Abandoned = Site; // owned by team A
	Abandoned = UArchonMatchPolicyLibrary::TickSiteCapture(Abandoned, 0, 1, 6.0f, Config);
	TestEqual(TEXT("Team B starts the steal"), Abandoned.CapturingTeam, 1);
	Abandoned = UArchonMatchPolicyLibrary::TickSiteCapture(Abandoned, 0, 0, 2.0f, Config);
	TestTrue(
		TEXT("Empty site decays steal progress"),
		FMath::IsNearlyEqual(Abandoned.CaptureProgressSeconds, 4.0f));
	TestEqual(TEXT("Owner keeps the empty site"), Abandoned.OwningTeam, 0);
	Abandoned = UArchonMatchPolicyLibrary::TickSiteCapture(Abandoned, 0, 0, 10.0f, Config);
	TestEqual(TEXT("Full decay clears the capturing team"), Abandoned.CapturingTeam, static_cast<int32>(INDEX_NONE));

	// Owner presence clears rival progress instantly.
	FArchonResourceSiteState Defended = Site; // owned by team A
	Defended = UArchonMatchPolicyLibrary::TickSiteCapture(Defended, 0, 1, 6.0f, Config);
	Defended = UArchonMatchPolicyLibrary::TickSiteCapture(Defended, 1, 0, 0.1f, Config);
	TestTrue(
		TEXT("Owner presence clears steal progress"),
		FMath::IsNearlyEqual(Defended.CaptureProgressSeconds, 0.0f));
	TestEqual(TEXT("Owner keeps the defended site"), Defended.OwningTeam, 0);

	// Switching capturer resets progress.
	FArchonResourceSiteState Swapped;
	Swapped = UArchonMatchPolicyLibrary::TickSiteCapture(Swapped, 1, 0, 6.0f, Config);
	Swapped = UArchonMatchPolicyLibrary::TickSiteCapture(Swapped, 0, 1, 1.0f, Config);
	TestEqual(TEXT("New capturer takes over"), Swapped.CapturingTeam, 1);
	TestTrue(
		TEXT("Capturer switch resets progress"),
		FMath::IsNearlyEqual(Swapped.CaptureProgressSeconds, 1.0f));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonMatchSupplyIncomeTest,
	"ArchonFactory.Match.SupplyIncomeScalesWithSitesHeld",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FArchonMatchSupplyIncomeTest::RunTest(const FString& Parameters)
{
	const FArchonMatchConfig Config = TestConfig();

	TestEqual(
		TEXT("No sites: passive trickle only"),
		UArchonMatchPolicyLibrary::ComputeSupplyPerTick(0, Config),
		5);
	TestEqual(
		TEXT("One site"),
		UArchonMatchPolicyLibrary::ComputeSupplyPerTick(1, Config),
		30);
	TestEqual(
		TEXT("All three sites"),
		UArchonMatchPolicyLibrary::ComputeSupplyPerTick(3, Config),
		80);
	TestEqual(
		TEXT("Negative input clamps to trickle"),
		UArchonMatchPolicyLibrary::ComputeSupplyPerTick(-2, Config),
		5);

	// Presence radius is 2D: height above the pad must not break capture.
	TestTrue(
		TEXT("Inside radius counts"),
		UArchonMatchPolicyLibrary::IsLocationInsideSite(
			FVector(500.0, 0.0, 400.0), FVector::ZeroVector, Config));
	TestTrue(
		TEXT("Outside radius does not"),
		!UArchonMatchPolicyLibrary::IsLocationInsideSite(
			FVector(1200.0, 0.0, 0.0), FVector::ZeroVector, Config));

	return true;
}
