#if WITH_DEV_AUTOMATION_TESTS

#include "ArchonMapTableWidget.h"
#include "ArchonMapTableActor.h"
#include "ArchonTeamRtsStateComponent.h"
#include "ArchonTeamVisibilityStateComponent.h"
#include "Misc/AutomationTest.h"

namespace
{
	FArchonSelectableSquadCandidate MakeWidgetCandidate(FName SquadId, int32 TeamId, const FVector2D& Pos)
	{
		FArchonSelectableSquadCandidate Candidate;
		Candidate.SquadId = SquadId;
		Candidate.TeamId = TeamId;
		Candidate.TableSpacePosition = Pos;
		Candidate.bIsAlive = true;
		return Candidate;
	}

	FArchonVisibilityGridCell MakeWidgetVisibilityCell(
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
	FArchonMapTableWidgetDragBoxSelectsFriendlySquadTest,
	"ArchonFactory.MapTableWidget.DragBoxSelectsFriendlySquad",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonMapTableWidgetDragBoxSelectsFriendlySquadTest::RunTest(const FString& Parameters)
{
	UArchonMapTableWidget* Widget = NewObject<UArchonMapTableWidget>();
	Widget->ConfigureMapTableWidget(nullptr, 0, 0, FVector::ZeroVector, FVector(1000.0f, 1000.0f, 0.0f));

	TArray<FArchonSelectableSquadCandidate> Candidates;
	Candidates.Add(MakeWidgetCandidate(TEXT("verdant_thornbound_squad_a"), 0, FVector2D(0.25f, 0.25f)));
	Candidates.Add(MakeWidgetCandidate(TEXT("lenswright_decoy"), 1, FVector2D(0.30f, 0.30f)));
	Widget->SetSelectableSquadCandidates(Candidates);

	const bool bSelected = Widget->CommitDragBox(FVector2D(0.1f, 0.1f), FVector2D(0.4f, 0.4f));

	TestTrue(TEXT("Widget drag-box selects a friendly squad"), bSelected);
	TestEqual(TEXT("Only one squad selected"), Widget->GetSelectedSquadIds().Num(), 1);
	TestEqual(TEXT("Selected squad id is the friendly candidate"), Widget->GetSelectedSquadIds()[0], FName(TEXT("verdant_thornbound_squad_a")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonMapTableWidgetRightClickOrderSubmitsTargetLocationTest,
	"ArchonFactory.MapTableWidget.RightClickOrderSubmitsTargetLocation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonMapTableWidgetRightClickOrderSubmitsTargetLocationTest::RunTest(const FString& Parameters)
{
	AArchonMapTableActor* MapTable = NewObject<AArchonMapTableActor>();
	MapTable->ConfigureMapTable(0, EArchonSessionRoute::PrivateHost, false);

	UArchonMapTableWidget* Widget = NewObject<UArchonMapTableWidget>();
	Widget->ConfigureMapTableWidget(
		MapTable,
		7,
		0,
		FVector(-1000.0f, -1000.0f, 0.0f),
		FVector(2000.0f, 2000.0f, 0.0f));

	Widget->SetSelectableSquadCandidates({
		MakeWidgetCandidate(TEXT("verdant_thornbound_squad_a"), 0, FVector2D(0.2f, 0.2f))
	});
	Widget->CommitDragBox(FVector2D(0.1f, 0.1f), FVector2D(0.3f, 0.3f));

	FArchonRtsAuthorityDecision Decision;
	const bool bSubmitted = Widget->HandleRightClickOrder(
		FVector2D(0.75f, 0.50f),
		TEXT("splitroot_central"),
		Decision);

	const FArchonRtsCommandIntent LastIntent = MapTable->GetTeamState()->GetLastAcceptedCommandIntent();

	TestTrue(TEXT("Widget right-click order submits through map table"), bSubmitted);
	TestTrue(TEXT("Widget tracks submitted order"), Widget->HasSubmittedOrder());
	TestEqual(TEXT("Map-table command sequence increments"), MapTable->GetCurrentCommandSequence(), 1);
	TestEqual(TEXT("Selected squad becomes order subject"), LastIntent.SubjectId, FName(TEXT("verdant_thornbound_squad_a")));
	TestTrue(TEXT("Intent carries a target location"), LastIntent.bTargetLocationValid);
	TestEqual(TEXT("Intent target id is stable"), LastIntent.TargetId, FName(TEXT("splitroot_central")));
	TestEqual(TEXT("World target location is table-space converted"), LastIntent.TargetLocation, FVector(500.0f, 0.0f, 0.0f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonMapTableWidgetRallyPointDoesNotRequireSquadSelectionTest,
	"ArchonFactory.MapTableWidget.RallyPointDoesNotRequireSquadSelection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonMapTableWidgetRallyPointDoesNotRequireSquadSelectionTest::RunTest(const FString& Parameters)
{
	AArchonMapTableActor* MapTable = NewObject<AArchonMapTableActor>();
	MapTable->ConfigureMapTable(0, EArchonSessionRoute::PrivateHost, false);

	UArchonMapTableWidget* Widget = NewObject<UArchonMapTableWidget>();
	Widget->ConfigureMapTableWidget(
		MapTable,
		7,
		0,
		FVector(-1000.0f, -1000.0f, 0.0f),
		FVector(2000.0f, 2000.0f, 0.0f));

	FArchonRtsAuthorityDecision Decision;
	const bool bSubmitted = Widget->HandleRallyPointOrder(
		FVector2D(0.75f, 0.50f),
		TEXT("forward_rally"),
		Decision);

	const FArchonRtsCommandIntent LastIntent = MapTable->GetTeamState()->GetLastAcceptedCommandIntent();

	TestTrue(TEXT("Widget rally point submits without selection"), bSubmitted);
	TestTrue(TEXT("Widget tracks submitted rally order"), Widget->HasSubmittedOrder());
	TestEqual(TEXT("Map-table command sequence increments"), MapTable->GetCurrentCommandSequence(), 1);
	TestEqual(TEXT("Rally order does not hijack a selected squad"), LastIntent.SubjectId, FName(TEXT("team_rally")));
	TestEqual(TEXT("Rally order kind reaches team state"), LastIntent.OrderKind, EArchonRtsOrderKind::SetRallyPoint);
	TestTrue(TEXT("Team state stores rally after widget order"), MapTable->GetTeamState()->HasTeamRallyPoint());
	TestEqual(TEXT("World rally location is table-space converted"), MapTable->GetTeamState()->GetTeamRallyPoint(), FVector(500.0f, 0.0f, 0.0f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonMapTableWidgetFormatsVisibilitySummaryTest,
	"ArchonFactory.MapTableWidget.FormatsVisibilitySummary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonMapTableWidgetFormatsVisibilitySummaryTest::RunTest(const FString& Parameters)
{
	AArchonMapTableActor* MapTable = NewObject<AArchonMapTableActor>();
	MapTable->ConfigureMapTable(0, EArchonSessionRoute::PrivateHost, false);

	TArray<FArchonVisibilityGridCell> Cells;
	Cells.Add(MakeWidgetVisibilityCell(FIntPoint(0, 0), FVector::ZeroVector, EArchonTeamVisibilityState::Lit));
	Cells.Add(MakeWidgetVisibilityCell(FIntPoint(1, 0), FVector(500.0f, 0.0f, 0.0f), EArchonTeamVisibilityState::Fog));
	Cells.Add(MakeWidgetVisibilityCell(FIntPoint(2, 0), FVector(1000.0f, 0.0f, 0.0f), EArchonTeamVisibilityState::Black));

	UArchonTeamVisibilityStateComponent* VisibilityState = MapTable->GetTeamVisibilityState();
	VisibilityState->ConfigureVisibilityState(0, Cells);

	UArchonMapTableWidget* Widget = NewObject<UArchonMapTableWidget>();
	Widget->ConfigureMapTableWidget(MapTable, 0, 0, FVector::ZeroVector, FVector(1000.0f, 1000.0f, 0.0f));

	const FString Summary = Widget->FormatVisibilitySummary();

	TestTrue(TEXT("Summary reports lit cells"), Summary.Contains(TEXT("lit=1")));
	TestTrue(TEXT("Summary reports fog cells"), Summary.Contains(TEXT("fog=1")));
	TestTrue(TEXT("Summary reports black cells"), Summary.Contains(TEXT("black=1")));
	return true;
}

#endif
