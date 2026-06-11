#if WITH_DEV_AUTOMATION_TESTS

#include "ArchonTeamVisibilityPolicyLibrary.h"
#include "Misc/AutomationTest.h"

namespace
{
	FArchonVisibilitySource MakeSource(const FVector& Location, float Radius)
	{
		FArchonVisibilitySource Source;
		Source.TeamId = 0;
		Source.Location = Location;
		Source.Radius = Radius;
		return Source;
	}

	FArchonVisibilityGridCell MakeCell(
		const FIntPoint& Cell,
		const FVector& WorldCenter,
		EArchonTeamVisibilityState State)
	{
		FArchonVisibilityGridCell GridCell;
		GridCell.Cell = Cell;
		GridCell.WorldCenter = WorldCenter;
		GridCell.State = State;
		return GridCell;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonVisibilityBlackCellStaysBlackWithoutSightTest,
	"ArchonFactory.Visibility.BlackCellStaysBlackWithoutSight",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonVisibilityBlackCellStaysBlackWithoutSightTest::RunTest(const FString& Parameters)
{
	const EArchonTeamVisibilityState State = UArchonTeamVisibilityPolicyLibrary::ResolveCellVisibility(
		FVector(1000.0f, 0.0f, 0.0f),
		EArchonTeamVisibilityState::Black,
		{ MakeSource(FVector::ZeroVector, 250.0f) });

	TestEqual(TEXT("Unexplored cell outside sight remains black"), State, EArchonTeamVisibilityState::Black);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonVisibilitySourceLightsCellInRadiusTest,
	"ArchonFactory.Visibility.SourceLightsCellInRadius",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonVisibilitySourceLightsCellInRadiusTest::RunTest(const FString& Parameters)
{
	const EArchonTeamVisibilityState State = UArchonTeamVisibilityPolicyLibrary::ResolveCellVisibility(
		FVector(150.0f, 0.0f, 0.0f),
		EArchonTeamVisibilityState::Black,
		{ MakeSource(FVector::ZeroVector, 250.0f) });

	TestEqual(TEXT("Friendly sight source lights cells in radius"), State, EArchonTeamVisibilityState::Lit);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonVisibilityExploredCellFallsBackToFogTest,
	"ArchonFactory.Visibility.ExploredCellFallsBackToFog",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonVisibilityExploredCellFallsBackToFogTest::RunTest(const FString& Parameters)
{
	const EArchonTeamVisibilityState FromLit = UArchonTeamVisibilityPolicyLibrary::ResolveCellVisibility(
		FVector(1000.0f, 0.0f, 0.0f),
		EArchonTeamVisibilityState::Lit,
		{ MakeSource(FVector::ZeroVector, 250.0f) });
	const EArchonTeamVisibilityState FromFog = UArchonTeamVisibilityPolicyLibrary::ResolveCellVisibility(
		FVector(1000.0f, 0.0f, 0.0f),
		EArchonTeamVisibilityState::Fog,
		{ MakeSource(FVector::ZeroVector, 250.0f) });

	TestEqual(TEXT("Previously lit cells become fog when sight leaves"), FromLit, EArchonTeamVisibilityState::Fog);
	TestEqual(TEXT("Previously fogged cells remain fog without sight"), FromFog, EArchonTeamVisibilityState::Fog);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonVisibilityGridAggregatesSourcesAndReportsNewlyLitTest,
	"ArchonFactory.Visibility.GridAggregatesSourcesAndReportsNewlyLit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonVisibilityGridAggregatesSourcesAndReportsNewlyLitTest::RunTest(const FString& Parameters)
{
	TArray<FArchonVisibilityGridCell> PreviousCells;
	PreviousCells.Add(MakeCell(FIntPoint(0, 0), FVector(0.0f, 0.0f, 0.0f), EArchonTeamVisibilityState::Black));
	PreviousCells.Add(MakeCell(FIntPoint(1, 0), FVector(500.0f, 0.0f, 0.0f), EArchonTeamVisibilityState::Fog));
	PreviousCells.Add(MakeCell(FIntPoint(2, 0), FVector(1000.0f, 0.0f, 0.0f), EArchonTeamVisibilityState::Lit));

	TArray<FArchonVisibilityGridCell> NextCells;
	TArray<FIntPoint> NewlyLitCells;
	UArchonTeamVisibilityPolicyLibrary::RecomputeVisibilityGrid(
		PreviousCells,
		{ MakeSource(FVector(0.0f, 0.0f, 0.0f), 200.0f), MakeSource(FVector(500.0f, 0.0f, 0.0f), 200.0f) },
		NextCells,
		NewlyLitCells);

	TestEqual(TEXT("All grid cells are recomputed"), NextCells.Num(), 3);
	TestEqual(TEXT("First source lights first cell"), NextCells[0].State, EArchonTeamVisibilityState::Lit);
	TestEqual(TEXT("Second source lights second cell"), NextCells[1].State, EArchonTeamVisibilityState::Lit);
	TestEqual(TEXT("Old lit cell outside sight becomes fog"), NextCells[2].State, EArchonTeamVisibilityState::Fog);
	TestEqual(TEXT("Two non-lit cells became newly lit"), NewlyLitCells.Num(), 2);
	TestTrue(TEXT("Newly lit includes first cell"), NewlyLitCells.Contains(FIntPoint(0, 0)));
	TestTrue(TEXT("Newly lit includes second cell"), NewlyLitCells.Contains(FIntPoint(1, 0)));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonVisibilityBuildingSnapshotFreezesInFogAndUpdatesWhenLitTest,
	"ArchonFactory.Visibility.BuildingSnapshotFreezesInFogAndUpdatesWhenLit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonVisibilityBuildingSnapshotFreezesInFogAndUpdatesWhenLitTest::RunTest(const FString& Parameters)
{
	FArchonBuildingVisionReport LitReport;
	LitReport.BuildingId = TEXT("lenswright_outpost");
	LitReport.ObservedTeamId = 1;
	LitReport.Location = FVector(600.0f, 300.0f, 0.0f);
	LitReport.ObservedState = TEXT("intact");
	LitReport.ObservationSequence = 10;
	LitReport.bCurrentlyVisible = true;

	const FArchonBuildingSnapshot FirstSnapshot =
		UArchonTeamVisibilityPolicyLibrary::ResolveBuildingSnapshot(FArchonBuildingSnapshot(), LitReport);

	FArchonBuildingVisionReport FogReport = LitReport;
	FogReport.ObservedState = TEXT("destroyed");
	FogReport.ObservationSequence = 11;
	FogReport.bCurrentlyVisible = false;

	const FArchonBuildingSnapshot FrozenSnapshot =
		UArchonTeamVisibilityPolicyLibrary::ResolveBuildingSnapshot(FirstSnapshot, FogReport);

	FArchonBuildingVisionReport RelitReport = FogReport;
	RelitReport.bCurrentlyVisible = true;
	RelitReport.ObservationSequence = 12;

	const FArchonBuildingSnapshot UpdatedSnapshot =
		UArchonTeamVisibilityPolicyLibrary::ResolveBuildingSnapshot(FrozenSnapshot, RelitReport);

	TestTrue(TEXT("Lit building creates a ghost snapshot"), FirstSnapshot.bHasSnapshot);
	TestEqual(TEXT("Fog preserves last seen building state"), FrozenSnapshot.LastKnownState, FName(TEXT("intact")));
	TestEqual(TEXT("Fog preserves last seen sequence"), FrozenSnapshot.LastObservedSequence, 10);
	TestEqual(TEXT("Relit building updates ghost state"), UpdatedSnapshot.LastKnownState, FName(TEXT("destroyed")));
	TestEqual(TEXT("Relit building updates observed sequence"), UpdatedSnapshot.LastObservedSequence, 12);
	return true;
}

// --- Additive extension by Rook (Claude Opus 4.7) on top of Hex's S1 ---
// Replication-policy predicate tests. Documents the "own-team always,
// enemy only when Lit" rule that the state component / map-table widget
// will rely on.

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonVisibilityReplicationOwnTeamAlwaysReplicatesTest,
	"ArchonFactory.Visibility.ReplicationOwnTeamAlwaysReplicates",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonVisibilityReplicationOwnTeamAlwaysReplicatesTest::RunTest(const FString& Parameters)
{
	// Own-team actor replicates regardless of cell state — even on a
	// Black cell (e.g. a unit that just spawned outside any sight source
	// of its own team's known geometry).
	const bool bOnBlack = UArchonTeamVisibilityPolicyLibrary::ShouldReplicateActorBasedOnVisibility(
		1, 1, EArchonTeamVisibilityState::Black);
	const bool bOnFog = UArchonTeamVisibilityPolicyLibrary::ShouldReplicateActorBasedOnVisibility(
		1, 1, EArchonTeamVisibilityState::Fog);
	const bool bOnLit = UArchonTeamVisibilityPolicyLibrary::ShouldReplicateActorBasedOnVisibility(
		1, 1, EArchonTeamVisibilityState::Lit);

	TestTrue(TEXT("Own-team actor on Black still replicates"), bOnBlack);
	TestTrue(TEXT("Own-team actor on Fog still replicates"), bOnFog);
	TestTrue(TEXT("Own-team actor on Lit still replicates"), bOnLit);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonVisibilityReplicationEnemyOnlyWhenLitTest,
	"ArchonFactory.Visibility.ReplicationEnemyOnlyWhenLit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonVisibilityReplicationEnemyOnlyWhenLitTest::RunTest(const FString& Parameters)
{
	const bool bEnemyOnLit = UArchonTeamVisibilityPolicyLibrary::ShouldReplicateActorBasedOnVisibility(
		1, 2, EArchonTeamVisibilityState::Lit);
	const bool bEnemyOnFog = UArchonTeamVisibilityPolicyLibrary::ShouldReplicateActorBasedOnVisibility(
		1, 2, EArchonTeamVisibilityState::Fog);
	const bool bEnemyOnBlack = UArchonTeamVisibilityPolicyLibrary::ShouldReplicateActorBasedOnVisibility(
		1, 2, EArchonTeamVisibilityState::Black);

	TestTrue(TEXT("Enemy actor on Lit cell replicates"), bEnemyOnLit);
	TestFalse(TEXT("Enemy actor on Fog cell does not replicate"), bEnemyOnFog);
	TestFalse(TEXT("Enemy actor on Black cell does not replicate"), bEnemyOnBlack);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonVisibilityReplicationUnassignedViewerNeverGetsOwnTeamShortcutTest,
	"ArchonFactory.Visibility.ReplicationUnassignedViewerNeverGetsOwnTeamShortcut",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonVisibilityReplicationUnassignedViewerNeverGetsOwnTeamShortcutTest::RunTest(const FString& Parameters)
{
	// A viewing team of INDEX_NONE must not accidentally match an actor
	// of INDEX_NONE via the own-team shortcut. The shortcut should only
	// fire for valid team ids.
	const bool bReplicatesOnFog = UArchonTeamVisibilityPolicyLibrary::ShouldReplicateActorBasedOnVisibility(
		INDEX_NONE, INDEX_NONE, EArchonTeamVisibilityState::Fog);
	const bool bReplicatesOnLit = UArchonTeamVisibilityPolicyLibrary::ShouldReplicateActorBasedOnVisibility(
		INDEX_NONE, INDEX_NONE, EArchonTeamVisibilityState::Lit);

	TestFalse(TEXT("Unassigned viewer + unassigned actor + Fog: withhold"), bReplicatesOnFog);
	TestTrue(TEXT("Unassigned viewer + unassigned actor + Lit: passes the cell-state branch"), bReplicatesOnLit);
	return true;
}

#endif
