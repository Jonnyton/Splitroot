#include "ArchonBotSteeringPolicyLibrary.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonBotSteeringSeeksObjective,
	"ArchonFactory.Bot.SteeringSeeksObjectiveWhenClear",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FArchonBotSteeringSeeksObjective::RunTest(const FString& Parameters)
{
	const FArchonBotSteeringDecision Decision = UArchonBotSteeringPolicyLibrary::ComputeSteering(
		FVector::ZeroVector,
		FVector(1000.0f, 0.0f, 0.0f),
		/*bObstacleAhead=*/ false,
		FVector::ZeroVector,
		/*AcceptanceRadius=*/ 300.0f);
	TestFalse(TEXT("not avoiding"), Decision.bAvoidingObstacle);
	TestFalse(TEXT("not at objective"), Decision.bAtObjective);
	TestTrue(TEXT("seeks +X"), Decision.MoveDirection.Equals(FVector::ForwardVector, 0.01f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonBotSteeringSlidesAlongWall,
	"ArchonFactory.Bot.SteeringSlidesAlongObstacle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FArchonBotSteeringSlidesAlongWall::RunTest(const FString& Parameters)
{
	// Objective ahead-right; wall directly ahead facing back at us.
	const FArchonBotSteeringDecision Decision = UArchonBotSteeringPolicyLibrary::ComputeSteering(
		FVector::ZeroVector,
		FVector(1000.0f, 400.0f, 0.0f),
		/*bObstacleAhead=*/ true,
		FVector(-1.0f, 0.0f, 0.0f),
		300.0f);
	TestTrue(TEXT("avoiding"), Decision.bAvoidingObstacle);
	TestTrue(TEXT("no into-wall component"), FMath::Abs(Decision.MoveDirection.X) < 0.01f);
	TestTrue(TEXT("slides toward objective side (+Y)"), Decision.MoveDirection.Y > 0.9f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonBotSteeringHeadOnPicksTangent,
	"ArchonFactory.Bot.SteeringHeadOnWallPicksATangent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FArchonBotSteeringHeadOnPicksTangent::RunTest(const FString& Parameters)
{
	// Objective dead ahead behind the wall: seek is parallel to the
	// normal, slide degenerates — policy must still produce a tangent.
	const FArchonBotSteeringDecision Decision = UArchonBotSteeringPolicyLibrary::ComputeSteering(
		FVector::ZeroVector,
		FVector(1000.0f, 0.0f, 0.0f),
		/*bObstacleAhead=*/ true,
		FVector(-1.0f, 0.0f, 0.0f),
		300.0f);
	TestTrue(TEXT("avoiding"), Decision.bAvoidingObstacle);
	TestTrue(TEXT("unit-length tangent chosen"), FMath::Abs(Decision.MoveDirection.Size2D() - 1.0f) < 0.01f);
	TestTrue(TEXT("tangent is perpendicular to the wall"), FMath::Abs(Decision.MoveDirection.X) < 0.01f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonBotSteeringStopsAtObjective,
	"ArchonFactory.Bot.SteeringStopsInsideAcceptanceRadius",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FArchonBotSteeringStopsAtObjective::RunTest(const FString& Parameters)
{
	const FArchonBotSteeringDecision Decision = UArchonBotSteeringPolicyLibrary::ComputeSteering(
		FVector(900.0f, 0.0f, 0.0f),
		FVector(1000.0f, 0.0f, 0.0f),
		false,
		FVector::ZeroVector,
		300.0f);
	TestTrue(TEXT("at objective"), Decision.bAtObjective);
	TestTrue(TEXT("no movement"), Decision.MoveDirection.IsNearlyZero());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonBotSteeringUnstickAlternatesSides,
	"ArchonFactory.Bot.SteeringUnstickAlternatesSides",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FArchonBotSteeringUnstickAlternatesSides::RunTest(const FString& Parameters)
{
	const FVector First = UArchonBotSteeringPolicyLibrary::ComputeUnstickEscapeTarget(
		FVector::ZeroVector,
		FVector(1000.0f, 0.0f, 0.0f),
		0,
		650.0f,
		250.0f);
	const FVector Second = UArchonBotSteeringPolicyLibrary::ComputeUnstickEscapeTarget(
		FVector::ZeroVector,
		FVector(1000.0f, 0.0f, 0.0f),
		1,
		650.0f,
		250.0f);

	TestTrue(TEXT("first attempt backsteps from objective"), First.X < -249.0f);
	TestTrue(TEXT("first attempt strafes left"), First.Y > 649.0f);
	TestTrue(TEXT("second attempt backsteps from objective"), Second.X < -249.0f);
	TestTrue(TEXT("second attempt strafes right"), Second.Y < -649.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonBotSteeringUnstickHandlesZeroObjectiveVector,
	"ArchonFactory.Bot.SteeringUnstickHandlesZeroObjectiveVector",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FArchonBotSteeringUnstickHandlesZeroObjectiveVector::RunTest(const FString& Parameters)
{
	const FVector Target = UArchonBotSteeringPolicyLibrary::ComputeUnstickEscapeTarget(
		FVector::ZeroVector,
		FVector::ZeroVector,
		0,
		500.0f,
		200.0f);

	TestTrue(TEXT("fallback backsteps along -X"), Target.X < -199.0f);
	TestTrue(TEXT("fallback picks a lateral lane"), Target.Y > 499.0f);
	TestTrue(TEXT("fallback stays 2D"), FMath::IsNearlyZero(Target.Z));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonBotSteeringUnstickCandidateQueryPreservesLegacyFirstChoice,
	"ArchonFactory.Bot.SteeringUnstickCandidateQueryPreservesLegacyFirstChoice",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FArchonBotSteeringUnstickCandidateQueryPreservesLegacyFirstChoice::RunTest(const FString& Parameters)
{
	const FVector BotLocation = FVector::ZeroVector;
	const FVector Objective(1000.0f, 0.0f, 0.0f);
	const TArray<FVector> Candidates = UArchonBotSteeringPolicyLibrary::ComputeUnstickEscapeCandidates(
		BotLocation,
		Objective,
		0,
		650.0f,
		250.0f);
	const FVector Legacy = UArchonBotSteeringPolicyLibrary::ComputeUnstickEscapeTarget(
		BotLocation,
		Objective,
		0,
		650.0f,
		250.0f);

	TestTrue(TEXT("candidate list has alternatives"), Candidates.Num() >= 6);
	TestTrue(TEXT("first candidate remains the previous unstick target"), Candidates[0].Equals(Legacy, 0.01f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonBotSteeringUnstickCandidateQueryExpandsAfterRepeats,
	"ArchonFactory.Bot.SteeringUnstickCandidateQueryExpandsAfterRepeats",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FArchonBotSteeringUnstickCandidateQueryExpandsAfterRepeats::RunTest(const FString& Parameters)
{
	const FVector BotLocation = FVector::ZeroVector;
	const FVector Objective(1000.0f, 0.0f, 0.0f);
	const TArray<FVector> Early = UArchonBotSteeringPolicyLibrary::ComputeUnstickEscapeCandidates(
		BotLocation,
		Objective,
		0,
		650.0f,
		250.0f);
	const TArray<FVector> Late = UArchonBotSteeringPolicyLibrary::ComputeUnstickEscapeCandidates(
		BotLocation,
		Objective,
		8,
		650.0f,
		250.0f);

	TestTrue(TEXT("late repeated attempt pushes the first option farther"), Late[0].Size2D() > Early[0].Size2D());

	bool bHasForwardDiagonal = false;
	for (const FVector& Candidate : Early)
	{
		if (Candidate.X > 0.0f && FMath::Abs(Candidate.Y) > 0.0f)
		{
			bHasForwardDiagonal = true;
			break;
		}
	}
	TestTrue(TEXT("query includes a forward diagonal to escape side/back loops"), bHasForwardDiagonal);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonBotSteeringStandOffKeepsObjectiveRange,
	"ArchonFactory.Bot.SteeringStandOffKeepsObjectiveRange",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FArchonBotSteeringStandOffKeepsObjectiveRange::RunTest(const FString& Parameters)
{
	const FVector Target = UArchonBotSteeringPolicyLibrary::ComputeObjectiveStandOffTarget(
		FVector::ZeroVector,
		FVector(1000.0f, 0.0f, 120.0f),
		0,
		300.0f,
		200.0f);

	TestTrue(TEXT("target stays on bot side of objective"), Target.X < 701.0f && Target.X > 699.0f);
	TestTrue(TEXT("center lane has no lateral offset"), FMath::IsNearlyZero(Target.Y));
	TestTrue(TEXT("target keeps objective Z"), FMath::IsNearlyEqual(Target.Z, 120.0f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonBotSteeringStandOffSpreadsAdjacentBots,
	"ArchonFactory.Bot.SteeringStandOffSpreadsAdjacentBots",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FArchonBotSteeringStandOffSpreadsAdjacentBots::RunTest(const FString& Parameters)
{
	const FVector FirstLane = UArchonBotSteeringPolicyLibrary::ComputeObjectiveStandOffTarget(
		FVector::ZeroVector,
		FVector(1000.0f, 0.0f, 0.0f),
		1,
		300.0f,
		200.0f);
	const FVector SecondLane = UArchonBotSteeringPolicyLibrary::ComputeObjectiveStandOffTarget(
		FVector::ZeroVector,
		FVector(1000.0f, 0.0f, 0.0f),
		2,
		300.0f,
		200.0f);

	TestTrue(TEXT("both lanes keep the stand-off X"), FMath::IsNearlyEqual(FirstLane.X, 700.0f, 1.0f) && FMath::IsNearlyEqual(SecondLane.X, 700.0f, 1.0f));
	TestTrue(TEXT("adjacent bots split to opposite sides"), FirstLane.Y * SecondLane.Y < 0.0f);
	TestTrue(TEXT("lane spacing is deterministic"), FMath::IsNearlyEqual(FMath::Abs(FirstLane.Y), 200.0f, 1.0f) && FMath::IsNearlyEqual(FMath::Abs(SecondLane.Y), 200.0f, 1.0f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonBotSteeringStandOffHandlesZeroObjectiveVector,
	"ArchonFactory.Bot.SteeringStandOffHandlesZeroObjectiveVector",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FArchonBotSteeringStandOffHandlesZeroObjectiveVector::RunTest(const FString& Parameters)
{
	const FVector Target = UArchonBotSteeringPolicyLibrary::ComputeObjectiveStandOffTarget(
		FVector::ZeroVector,
		FVector::ZeroVector,
		2,
		500.0f,
		100.0f);

	TestTrue(TEXT("fallback picks a nonzero lane"), Target.Size2D() > 400.0f);
	TestTrue(TEXT("fallback remains on objective plane"), FMath::IsNearlyZero(Target.Z));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonBotSteeringRouteStallAbandonsAfterThreshold,
	"ArchonFactory.Bot.SteeringRouteStallAbandonsAfterThreshold",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FArchonBotSteeringRouteStallAbandonsAfterThreshold::RunTest(const FString& Parameters)
{
	TestFalse(
		TEXT("route waypoint is not abandoned before threshold"),
		UArchonBotSteeringPolicyLibrary::ShouldAbandonRouteWaypointAfterStalls(7, 8));
	TestTrue(
		TEXT("route waypoint is abandoned at threshold"),
		UArchonBotSteeringPolicyLibrary::ShouldAbandonRouteWaypointAfterStalls(8, 8));
	TestFalse(
		TEXT("zero threshold disables route abandonment"),
		UArchonBotSteeringPolicyLibrary::ShouldAbandonRouteWaypointAfterStalls(100, 0));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonBotSteeringObjectiveLaneShiftEscalatesAndCaps,
	"ArchonFactory.Bot.SteeringObjectiveLaneShiftEscalatesAndCaps",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FArchonBotSteeringObjectiveLaneShiftEscalatesAndCaps::RunTest(const FString& Parameters)
{
	TestEqual(
		TEXT("no lane shift before threshold"),
		UArchonBotSteeringPolicyLibrary::ComputeObjectiveLaneShift(5, 6, 3),
		0);
	TestEqual(
		TEXT("first lane shift at threshold"),
		UArchonBotSteeringPolicyLibrary::ComputeObjectiveLaneShift(6, 6, 3),
		1);
	TestEqual(
		TEXT("second lane shift after another threshold band"),
		UArchonBotSteeringPolicyLibrary::ComputeObjectiveLaneShift(12, 6, 3),
		2);
	TestEqual(
		TEXT("lane shift caps"),
		UArchonBotSteeringPolicyLibrary::ComputeObjectiveLaneShift(60, 6, 3),
		3);
	return true;
}
