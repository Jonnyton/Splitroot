#if WITH_DEV_AUTOMATION_TESTS

#include "ArchonMapTableSelectionPolicyLibrary.h"
#include "Misc/AutomationTest.h"

namespace
{
	FArchonSelectableSquadCandidate MakeCandidate(FName SquadId, int32 TeamId, const FVector2D& Pos, bool bAlive = true)
	{
		FArchonSelectableSquadCandidate C;
		C.SquadId = SquadId;
		C.TeamId = TeamId;
		C.TableSpacePosition = Pos;
		C.bIsAlive = bAlive;
		return C;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonMapTableNormalizeDragBoxFromAnyCornerOrderTest,
	"ArchonFactory.MapTable.NormalizeDragBoxFromAnyCornerOrder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonMapTableNormalizeDragBoxFromAnyCornerOrderTest::RunTest(const FString& Parameters)
{
	const FArchonDragBox Box = UArchonMapTableSelectionPolicyLibrary::NormalizeDragBox(
		FVector2D(0.8f, 0.2f), FVector2D(0.3f, 0.7f));
	TestTrue(TEXT("Min X"), FMath::IsNearlyEqual(Box.Min.X, 0.3f));
	TestTrue(TEXT("Min Y"), FMath::IsNearlyEqual(Box.Min.Y, 0.2f));
	TestTrue(TEXT("Max X"), FMath::IsNearlyEqual(Box.Max.X, 0.8f));
	TestTrue(TEXT("Max Y"), FMath::IsNearlyEqual(Box.Max.Y, 0.7f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonMapTableBoxContainsPointInclusiveEdgesTest,
	"ArchonFactory.MapTable.BoxContainsPointInclusiveEdges",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonMapTableBoxContainsPointInclusiveEdgesTest::RunTest(const FString& Parameters)
{
	FArchonDragBox Box;
	Box.Min = FVector2D(0.0f, 0.0f);
	Box.Max = FVector2D(0.5f, 0.5f);

	TestTrue(TEXT("Top-left corner inside"), UArchonMapTableSelectionPolicyLibrary::BoxContainsPoint(Box, FVector2D(0.0f, 0.0f)));
	TestTrue(TEXT("Bottom-right corner inside"), UArchonMapTableSelectionPolicyLibrary::BoxContainsPoint(Box, FVector2D(0.5f, 0.5f)));
	TestFalse(TEXT("Just past max X outside"), UArchonMapTableSelectionPolicyLibrary::BoxContainsPoint(Box, FVector2D(0.5001f, 0.5f)));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonMapTableSelectionIncludesOnlyOwnTeamLivingInBoxTest,
	"ArchonFactory.MapTable.SelectionIncludesOnlyOwnTeamLivingInBox",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonMapTableSelectionIncludesOnlyOwnTeamLivingInBoxTest::RunTest(const FString& Parameters)
{
	const FArchonDragBox Box = UArchonMapTableSelectionPolicyLibrary::NormalizeDragBox(
		FVector2D(0.1f, 0.1f), FVector2D(0.6f, 0.6f));

	TArray<FArchonSelectableSquadCandidate> Candidates;
	Candidates.Add(MakeCandidate(TEXT("own_alive_in"), 0, FVector2D(0.3f, 0.3f)));
	Candidates.Add(MakeCandidate(TEXT("own_dead_in"), 0, FVector2D(0.4f, 0.4f), false));
	Candidates.Add(MakeCandidate(TEXT("own_alive_out"), 0, FVector2D(0.9f, 0.9f)));
	Candidates.Add(MakeCandidate(TEXT("enemy_alive_in"), 1, FVector2D(0.5f, 0.5f)));

	TArray<FName> Selected;
	UArchonMapTableSelectionPolicyLibrary::ComputeSelection(Box, 0, Candidates, Selected);

	TestEqual(TEXT("Only one own-team alive-in-box squad selected"), Selected.Num(), 1);
	TestTrue(TEXT("Selected is own_alive_in"), Selected.Contains(FName(TEXT("own_alive_in"))));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonMapTableSelectionDegenerateBoxReturnsEmptyTest,
	"ArchonFactory.MapTable.SelectionDegenerateBoxReturnsEmpty",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonMapTableSelectionDegenerateBoxReturnsEmptyTest::RunTest(const FString& Parameters)
{
	FArchonDragBox DegenerateBox;
	DegenerateBox.Min = FVector2D(0.5f, 0.5f);
	DegenerateBox.Max = FVector2D(0.5f, 0.5f);

	TArray<FArchonSelectableSquadCandidate> Candidates;
	Candidates.Add(MakeCandidate(TEXT("own_at_point"), 0, FVector2D(0.5f, 0.5f)));

	TArray<FName> Selected;
	UArchonMapTableSelectionPolicyLibrary::ComputeSelection(DegenerateBox, 0, Candidates, Selected);

	TestEqual(TEXT("Degenerate box selects nothing (click pathway is separate)"), Selected.Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonMapTableClickSelectionPicksNearestWithinRadiusTest,
	"ArchonFactory.MapTable.ClickSelectionPicksNearestWithinRadius",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonMapTableClickSelectionPicksNearestWithinRadiusTest::RunTest(const FString& Parameters)
{
	TArray<FArchonSelectableSquadCandidate> Candidates;
	Candidates.Add(MakeCandidate(TEXT("far"), 0, FVector2D(0.55f, 0.55f)));
	Candidates.Add(MakeCandidate(TEXT("near"), 0, FVector2D(0.51f, 0.51f)));
	Candidates.Add(MakeCandidate(TEXT("medium"), 0, FVector2D(0.53f, 0.53f)));

	TArray<FName> Selected;
	UArchonMapTableSelectionPolicyLibrary::ResolveClickSelection(
		FVector2D(0.5f, 0.5f), /*ClickRadius*/ 0.1f, /*IssuingTeamId*/ 0, Candidates, Selected);

	TestEqual(TEXT("Exactly one squad selected"), Selected.Num(), 1);
	TestEqual(TEXT("Closest squad wins"), Selected[0], FName(TEXT("near")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonMapTableClickSelectionOutsideRadiusReturnsEmptyTest,
	"ArchonFactory.MapTable.ClickSelectionOutsideRadiusReturnsEmpty",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonMapTableClickSelectionOutsideRadiusReturnsEmptyTest::RunTest(const FString& Parameters)
{
	TArray<FArchonSelectableSquadCandidate> Candidates;
	Candidates.Add(MakeCandidate(TEXT("far_friendly"), 0, FVector2D(0.9f, 0.9f)));

	TArray<FName> Selected;
	UArchonMapTableSelectionPolicyLibrary::ResolveClickSelection(
		FVector2D(0.1f, 0.1f), /*ClickRadius*/ 0.05f, 0, Candidates, Selected);

	TestEqual(TEXT("No friendly within radius returns empty"), Selected.Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonMapTableClickSelectionStableTieBreakTest,
	"ArchonFactory.MapTable.ClickSelectionStableTieBreak",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonMapTableClickSelectionStableTieBreakTest::RunTest(const FString& Parameters)
{
	// Two candidates equidistant from click point; smaller lexicographic id should win.
	TArray<FArchonSelectableSquadCandidate> Candidates;
	Candidates.Add(MakeCandidate(TEXT("squad_b"), 0, FVector2D(0.45f, 0.5f)));
	Candidates.Add(MakeCandidate(TEXT("squad_a"), 0, FVector2D(0.55f, 0.5f)));

	TArray<FName> Selected;
	UArchonMapTableSelectionPolicyLibrary::ResolveClickSelection(
		FVector2D(0.5f, 0.5f), /*ClickRadius*/ 0.1f, 0, Candidates, Selected);

	TestEqual(TEXT("Exactly one squad selected"), Selected.Num(), 1);
	TestEqual(TEXT("Lexicographically smaller id wins tie"), Selected[0], FName(TEXT("squad_a")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonMapTableTableSpaceToWorldRoundTripTest,
	"ArchonFactory.MapTable.TableSpaceToWorldRoundTrip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonMapTableTableSpaceToWorldRoundTripTest::RunTest(const FString& Parameters)
{
	const FVector Origin(-2000.0f, -2000.0f, 0.0f);
	const FVector Extents(24000.0f, 16000.0f, 0.0f);

	const FVector2D TablePoint(0.37f, 0.62f);
	const FVector World = UArchonMapTableSelectionPolicyLibrary::TableSpaceToWorld(TablePoint, Origin, Extents);
	const FVector2D RoundTrip = UArchonMapTableSelectionPolicyLibrary::WorldToTableSpace(World, Origin, Extents);

	TestTrue(TEXT("Round trip X within 1e-5"), FMath::IsNearlyEqual(RoundTrip.X, TablePoint.X, 1e-5f));
	TestTrue(TEXT("Round trip Y within 1e-5"), FMath::IsNearlyEqual(RoundTrip.Y, TablePoint.Y, 1e-5f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonMapTableTableSpaceCornersMapToWorldCornersTest,
	"ArchonFactory.MapTable.TableSpaceCornersMapToWorldCorners",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonMapTableTableSpaceCornersMapToWorldCornersTest::RunTest(const FString& Parameters)
{
	const FVector Origin(100.0f, 200.0f, 0.0f);
	const FVector Extents(1000.0f, 500.0f, 0.0f);

	const FVector TopLeft = UArchonMapTableSelectionPolicyLibrary::TableSpaceToWorld(FVector2D(0.0f, 0.0f), Origin, Extents);
	const FVector BottomRight = UArchonMapTableSelectionPolicyLibrary::TableSpaceToWorld(FVector2D(1.0f, 1.0f), Origin, Extents);

	TestEqual(TEXT("(0,0) maps to WorldOrigin"), TopLeft, Origin);
	TestEqual(TEXT("(1,1) maps to WorldOrigin + Extents"), BottomRight, FVector(1100.0f, 700.0f, 0.0f));
	return true;
}

#endif
