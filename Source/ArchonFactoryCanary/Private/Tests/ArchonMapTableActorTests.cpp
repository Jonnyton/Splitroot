#if WITH_DEV_AUTOMATION_TESTS

#include "ArchonMapTableActor.h"
#include "ArchonTeamRtsStateComponent.h"
#include "Misc/AutomationTest.h"

namespace
{
	FArchonRtsCommandIntent MakeActorIntent(int32 TeamId)
	{
		FArchonRtsCommandIntent Intent;
		Intent.TeamId = TeamId;
		Intent.IssuingPlayerId = 11;
		Intent.SubjectId = TEXT("squad_actor_test");
		Intent.TargetId = TEXT("objective_actor_test");
		return Intent;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonTeamRtsStateComponentAcceptsMapTableOrderTest,
	"ArchonFactory.InWorld.TeamStateAcceptsMapTableOrder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonTeamRtsStateComponentAcceptsMapTableOrderTest::RunTest(const FString& Parameters)
{
	UArchonTeamRtsStateComponent* TeamState = NewObject<UArchonTeamRtsStateComponent>();
	TeamState->ConfigureTeamState(2);

	AArchonMapTableActor* MapTable = NewObject<AArchonMapTableActor>();
	MapTable->ConfigureMapTable(2, EArchonSessionRoute::PrivateHost, false);
	MapTable->AttachTeamState(TeamState);

	FArchonRtsAuthorityDecision Decision;
	const bool bAccepted = MapTable->SubmitCommand(
		EArchonMapTableCommandKind::ExecuteOrder,
		FArchonMapTableCommandContext(),
		MakeActorIntent(2),
		Decision);

	TestTrue(TEXT("In-world seam accepts valid local command"), bAccepted);
	TestEqual(TEXT("Team state command sequence increments"), TeamState->GetCurrentCommandSequence(), 1);
	TestEqual(TEXT("Accepted reason recorded"), TeamState->GetLastAcceptedReason(), FName(TEXT("accepted_standard_archon_order")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonTeamRtsStateComponentBlocksSteamOnlineNoOpTest,
	"ArchonFactory.InWorld.TeamStateBlocksSteamOnlineNoOp",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonTeamRtsStateComponentBlocksSteamOnlineNoOpTest::RunTest(const FString& Parameters)
{
	UArchonTeamRtsStateComponent* TeamState = NewObject<UArchonTeamRtsStateComponent>();
	TeamState->ConfigureTeamState(3);

	AArchonMapTableActor* MapTable = NewObject<AArchonMapTableActor>();
	MapTable->ConfigureMapTable(3, EArchonSessionRoute::SteamOnline, false);
	MapTable->AttachTeamState(TeamState);

	FArchonRtsAuthorityDecision Decision;
	const bool bAccepted = MapTable->SubmitCommand(
		EArchonMapTableCommandKind::ExecuteOrder,
		FArchonMapTableCommandContext(),
		MakeActorIntent(3),
		Decision);

	TestFalse(TEXT("SteamOnline free command is no-op"), bAccepted);
	TestEqual(TEXT("No-op does not increment sequence"), TeamState->GetCurrentCommandSequence(), 0);
	TestEqual(TEXT("No-op rejection reason recorded"), TeamState->GetLastRejectedReason(), FName(TEXT("map_table_no_op")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonMapTableActorRejectsWrongTeamTest,
	"ArchonFactory.InWorld.MapTableRejectsWrongTeam",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonMapTableActorRejectsWrongTeamTest::RunTest(const FString& Parameters)
{
	UArchonTeamRtsStateComponent* TeamState = NewObject<UArchonTeamRtsStateComponent>();
	TeamState->ConfigureTeamState(4);

	AArchonMapTableActor* MapTable = NewObject<AArchonMapTableActor>();
	MapTable->ConfigureMapTable(4, EArchonSessionRoute::LANHosted, false);
	MapTable->AttachTeamState(TeamState);

	FArchonRtsAuthorityDecision Decision;
	const bool bAccepted = MapTable->SubmitCommand(
		EArchonMapTableCommandKind::ExecuteOrder,
		FArchonMapTableCommandContext(),
		MakeActorIntent(5),
		Decision);

	TestFalse(TEXT("Wrong-team table command rejected"), bAccepted);
	TestEqual(TEXT("Wrong-team command does not mutate state"), TeamState->GetCurrentCommandSequence(), 0);
	TestEqual(TEXT("Wrong-team reason is stable"), Decision.Reason, FName(TEXT("wrong_map_table_team")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonMapTableActorSetRallyPointUpdatesTeamStateTest,
	"ArchonFactory.InWorld.SetRallyPointUpdatesTeamState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonMapTableActorSetRallyPointUpdatesTeamStateTest::RunTest(const FString& Parameters)
{
	UArchonTeamRtsStateComponent* TeamState = NewObject<UArchonTeamRtsStateComponent>();
	TeamState->ConfigureTeamState(4);

	AArchonMapTableActor* MapTable = NewObject<AArchonMapTableActor>();
	MapTable->ConfigureMapTable(4, EArchonSessionRoute::PrivateHost, false);
	MapTable->AttachTeamState(TeamState);

	FArchonRtsCommandIntent Intent = MakeActorIntent(4);
	Intent.OrderKind = EArchonRtsOrderKind::SetRallyPoint;
	Intent.SubjectId = TEXT("team_rally");
	Intent.TargetId = TEXT("forward_rally");
	Intent.TargetLocation = FVector(1800.0f, -300.0f, 120.0f);
	Intent.bTargetLocationValid = true;

	FArchonRtsAuthorityDecision Decision;
	const bool bAccepted = MapTable->SubmitCommand(
		EArchonMapTableCommandKind::ExecuteOrder,
		FArchonMapTableCommandContext(),
		Intent,
		Decision);

	TestTrue(TEXT("Map table accepts rally point order"), bAccepted);
	TestEqual(TEXT("Rally point increments sequence"), TeamState->GetCurrentCommandSequence(), 1);
	TestTrue(TEXT("Team state now has a rally point"), TeamState->HasTeamRallyPoint());
	TestTrue(TEXT("Team state stores the rally point"), TeamState->GetTeamRallyPoint().Equals(Intent.TargetLocation, 0.01f));
	TestEqual(TEXT("Last accepted command is the rally order"), TeamState->GetLastAcceptedCommandIntent().OrderKind, EArchonRtsOrderKind::SetRallyPoint);
	return true;
}

#endif
