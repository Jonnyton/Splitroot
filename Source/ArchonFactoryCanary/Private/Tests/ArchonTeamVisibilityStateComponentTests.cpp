#if WITH_DEV_AUTOMATION_TESTS

#include "ArchonTeamVisibilityStateComponent.h"
#include "Misc/AutomationTest.h"

namespace
{
	FArchonVisibilityGridCell MakeVisibilityStateCell(
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

	FArchonVisibilitySource MakeVisibilityStateSource(
		int32 TeamId,
		const FVector& Location,
		float Radius)
	{
		FArchonVisibilitySource Source;
		Source.TeamId = TeamId;
		Source.Location = Location;
		Source.Radius = Radius;
		return Source;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonVisibilityStateConfigureStoresTeamAndCellsTest,
	"ArchonFactory.VisibilityState.ConfigureStoresTeamAndCells",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonVisibilityStateConfigureStoresTeamAndCellsTest::RunTest(const FString& Parameters)
{
	UArchonTeamVisibilityStateComponent* VisibilityState = NewObject<UArchonTeamVisibilityStateComponent>();

	TArray<FArchonVisibilityGridCell> InitialCells;
	InitialCells.Add(MakeVisibilityStateCell(FIntPoint(0, 0), FVector::ZeroVector, EArchonTeamVisibilityState::Black));
	InitialCells.Add(MakeVisibilityStateCell(FIntPoint(1, 0), FVector(500.0f, 0.0f, 0.0f), EArchonTeamVisibilityState::Fog));

	VisibilityState->ConfigureVisibilityState(2, InitialCells);

	TestEqual(TEXT("Visibility state stores configured team"), VisibilityState->GetTeamId(), 2);
	TestEqual(TEXT("Visibility state stores configured cells"), VisibilityState->GetVisibilityCells().Num(), 2);
	TestEqual(
		TEXT("Known grid cell can be queried by coordinate"),
		VisibilityState->GetCellState(FIntPoint(1, 0)),
		EArchonTeamVisibilityState::Fog);
	TestEqual(
		TEXT("Unknown grid cells default to black shroud"),
		VisibilityState->GetCellState(FIntPoint(9, 9)),
		EArchonTeamVisibilityState::Black);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonVisibilityStateRecomputeLightsFriendlySourcesTest,
	"ArchonFactory.VisibilityState.RecomputeLightsFriendlySources",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonVisibilityStateRecomputeLightsFriendlySourcesTest::RunTest(const FString& Parameters)
{
	UArchonTeamVisibilityStateComponent* VisibilityState = NewObject<UArchonTeamVisibilityStateComponent>();

	TArray<FArchonVisibilityGridCell> InitialCells;
	InitialCells.Add(MakeVisibilityStateCell(FIntPoint(0, 0), FVector::ZeroVector, EArchonTeamVisibilityState::Black));
	InitialCells.Add(MakeVisibilityStateCell(FIntPoint(1, 0), FVector(500.0f, 0.0f, 0.0f), EArchonTeamVisibilityState::Black));

	VisibilityState->ConfigureVisibilityState(3, InitialCells);
	VisibilityState->SetFriendlySources({
		MakeVisibilityStateSource(3, FVector::ZeroVector, 150.0f),
		MakeVisibilityStateSource(4, FVector(500.0f, 0.0f, 0.0f), 150.0f)
	});
	VisibilityState->RecomputeVisibility();

	TestEqual(
		TEXT("Friendly sight source lights its cell"),
		VisibilityState->GetCellState(FIntPoint(0, 0)),
		EArchonTeamVisibilityState::Lit);
	TestEqual(
		TEXT("Wrong-team sight source is ignored"),
		VisibilityState->GetCellState(FIntPoint(1, 0)),
		EArchonTeamVisibilityState::Black);
	TestEqual(TEXT("One cell became newly lit"), VisibilityState->GetLastNewlyLitCellCount(), 1);
	TestEqual(TEXT("One lit cell is counted"), VisibilityState->CountCellsInState(EArchonTeamVisibilityState::Lit), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonVisibilityStateRecomputeTurnsStaleLightToFogTest,
	"ArchonFactory.VisibilityState.RecomputeTurnsStaleLightToFog",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonVisibilityStateRecomputeTurnsStaleLightToFogTest::RunTest(const FString& Parameters)
{
	UArchonTeamVisibilityStateComponent* VisibilityState = NewObject<UArchonTeamVisibilityStateComponent>();

	TArray<FArchonVisibilityGridCell> InitialCells;
	InitialCells.Add(MakeVisibilityStateCell(FIntPoint(0, 0), FVector::ZeroVector, EArchonTeamVisibilityState::Lit));

	VisibilityState->ConfigureVisibilityState(5, InitialCells);
	VisibilityState->SetFriendlySources({ MakeVisibilityStateSource(5, FVector(1000.0f, 0.0f, 0.0f), 100.0f) });
	VisibilityState->RecomputeVisibility();

	TestEqual(
		TEXT("Previously lit cell falls back to fog when sight leaves"),
		VisibilityState->GetCellState(FIntPoint(0, 0)),
		EArchonTeamVisibilityState::Fog);
	TestEqual(TEXT("No stale cell is counted as newly lit"), VisibilityState->GetLastNewlyLitCellCount(), 0);
	TestEqual(TEXT("One fog cell is counted"), VisibilityState->CountCellsInState(EArchonTeamVisibilityState::Fog), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonVisibilityStateBuildingSnapshotsFreezeAndUpdateTest,
	"ArchonFactory.VisibilityState.BuildingSnapshotsFreezeAndUpdate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonVisibilityStateBuildingSnapshotsFreezeAndUpdateTest::RunTest(const FString& Parameters)
{
	UArchonTeamVisibilityStateComponent* VisibilityState = NewObject<UArchonTeamVisibilityStateComponent>();
	VisibilityState->ConfigureVisibilityState(0, {});

	FArchonBuildingVisionReport LitReport;
	LitReport.BuildingId = TEXT("root_vault");
	LitReport.ObservedTeamId = 1;
	LitReport.Location = FVector(1200.0f, 300.0f, 0.0f);
	LitReport.ObservedState = TEXT("intact");
	LitReport.ObservationSequence = 4;
	LitReport.bCurrentlyVisible = true;

	FArchonBuildingSnapshot Snapshot;
	const bool bCreatedSnapshot = VisibilityState->ApplyBuildingVisionReport(LitReport, Snapshot);

	FArchonBuildingVisionReport FogReport = LitReport;
	FogReport.ObservedState = TEXT("destroyed");
	FogReport.ObservationSequence = 5;
	FogReport.bCurrentlyVisible = false;

	FArchonBuildingSnapshot FrozenSnapshot;
	const bool bKeptSnapshot = VisibilityState->ApplyBuildingVisionReport(FogReport, FrozenSnapshot);

	FArchonBuildingVisionReport RelitReport = FogReport;
	RelitReport.ObservationSequence = 6;
	RelitReport.bCurrentlyVisible = true;

	FArchonBuildingSnapshot UpdatedSnapshot;
	const bool bUpdatedSnapshot = VisibilityState->ApplyBuildingVisionReport(RelitReport, UpdatedSnapshot);

	TestTrue(TEXT("Visible building creates a snapshot"), bCreatedSnapshot);
	TestEqual(TEXT("Created snapshot stores visible state"), Snapshot.LastKnownState, FName(TEXT("intact")));
	TestTrue(TEXT("Fogged building keeps a previous snapshot"), bKeptSnapshot);
	TestEqual(TEXT("Fog preserves last known state"), FrozenSnapshot.LastKnownState, FName(TEXT("intact")));
	TestEqual(TEXT("Fog preserves last observed sequence"), FrozenSnapshot.LastObservedSequence, 4);
	TestTrue(TEXT("Relit building updates snapshot"), bUpdatedSnapshot);
	TestEqual(TEXT("Relit snapshot updates state"), UpdatedSnapshot.LastKnownState, FName(TEXT("destroyed")));
	TestEqual(TEXT("Relit snapshot updates sequence"), UpdatedSnapshot.LastObservedSequence, 6);
	return true;
}

#endif
