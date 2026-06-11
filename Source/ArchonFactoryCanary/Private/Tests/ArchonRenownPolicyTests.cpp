#include "ArchonRenownPolicyLibrary.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonRenownTricklePaysPerInterval,
	"ArchonFactory.Renown.TricklePaysOncePerFullInterval",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FArchonRenownTricklePaysPerInterval::RunTest(const FString& Parameters)
{
	const FArchonRenownTickResult Result = UArchonRenownPolicyLibrary::EvaluateTrickle(
		/*AccumulatorSeconds=*/ 9.5f,
		/*DeltaSeconds=*/ 1.0f,
		/*IntervalSeconds=*/ 10.0f,
		/*TricklePerInterval=*/ 5);
	TestEqual(TEXT("one interval crossed pays once"), Result.Payout, 5);
	TestTrue(TEXT("fractional remainder carried"), FMath::IsNearlyEqual(Result.NewAccumulatorSeconds, 0.5f, 0.001f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonRenownTrickleNoDrift,
	"ArchonFactory.Renown.TrickleHasNoDriftOverManyTicks",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FArchonRenownTrickleNoDrift::RunTest(const FString& Parameters)
{
	// 600 frames of 0.1s = 60 seconds = exactly 6 intervals of 10s.
	float Accumulator = 0.0f;
	int32 TotalPaid = 0;
	for (int32 Frame = 0; Frame < 600; ++Frame)
	{
		const FArchonRenownTickResult Result = UArchonRenownPolicyLibrary::EvaluateTrickle(
			Accumulator, 0.1f, 10.0f, 5);
		Accumulator = Result.NewAccumulatorSeconds;
		TotalPaid += Result.Payout;
	}
	TestEqual(TEXT("60s at 5-per-10s pays exactly 30"), TotalPaid, 30);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonRenownTrickleLongHitch,
	"ArchonFactory.Renown.TricklePaysMultipleIntervalsAfterHitch",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FArchonRenownTrickleLongHitch::RunTest(const FString& Parameters)
{
	// A 35-second hitch (or dead-time) still pays every interval owed.
	const FArchonRenownTickResult Result = UArchonRenownPolicyLibrary::EvaluateTrickle(
		0.0f, 35.0f, 10.0f, 5);
	TestEqual(TEXT("three intervals owed"), Result.Payout, 15);
	TestTrue(TEXT("5s remainder"), FMath::IsNearlyEqual(Result.NewAccumulatorSeconds, 5.0f, 0.001f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonRenownGuardrails,
	"ArchonFactory.Renown.InvalidConfigPaysNothing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FArchonRenownGuardrails::RunTest(const FString& Parameters)
{
	const FArchonRenownTickResult Zero = UArchonRenownPolicyLibrary::EvaluateTrickle(5.0f, 1.0f, 0.0f, 5);
	TestEqual(TEXT("zero interval pays nothing"), Zero.Payout, 0);
	TestEqual(TEXT("bounty is positive"), UArchonRenownPolicyLibrary::GetKillBounty() > 0, true);
	TestEqual(TEXT("harvest payout is positive"), UArchonRenownPolicyLibrary::GetHarvestPayout() > 0, true);
	return true;
}
