#if WITH_DEV_AUTOMATION_TESTS

#include "ArchonMapTablePolicyLibrary.h"
#include "ArchonTeamRtsPolicyLibrary.h"
#include "Misc/AutomationTest.h"

namespace
{
	FArchonRtsCommandIntent MakeValidIntent()
	{
		FArchonRtsCommandIntent Intent;
		Intent.TeamId = 1;
		Intent.IssuingPlayerId = 7;
		Intent.OrderKind = EArchonRtsOrderKind::MoveSquad;
		Intent.SubjectId = TEXT("squad_alpha");
		Intent.TargetId = TEXT("rally_point");
		return Intent;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonAcceptedOrderMutatesOneTeamStreamTest,
	"ArchonFactory.TeamRts.AcceptedOrderMutatesOneTeamStream",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonAcceptedOrderMutatesOneTeamStreamTest::RunTest(const FString& Parameters)
{
	const FArchonMapTableCommandContext Context;
	const FArchonMapTableDecision MapDecision = UArchonMapTablePolicyLibrary::EvaluateMapTableCommand(
		EArchonSessionRoute::PrivateHost,
		false,
		EArchonMapTableCommandKind::ExecuteOrder,
		Context);

	const FArchonRtsAuthorityDecision Decision = UArchonTeamRtsPolicyLibrary::FinalizeTeamCommand(
		MapDecision,
		MakeValidIntent(),
		41);

	TestTrue(TEXT("Accepted order mutates team state"), Decision.bMutatesTeamState);
	TestTrue(TEXT("Accepted order replicates to team"), Decision.bReplicatesToTeam);
	TestFalse(TEXT("Standard Archon mode needs no commander token"), Decision.bRequiresCommanderToken);
	TestEqual(TEXT("Team command stream increments once"), Decision.FinalSequence, 42);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonSteamOnlineNoOpCannotMutateRtsStateTest,
	"ArchonFactory.TeamRts.SteamOnlineNoOpCannotMutateState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonSteamOnlineNoOpCannotMutateRtsStateTest::RunTest(const FString& Parameters)
{
	const FArchonMapTableCommandContext Context;
	const FArchonMapTableDecision MapDecision = UArchonMapTablePolicyLibrary::EvaluateMapTableCommand(
		EArchonSessionRoute::SteamOnline,
		false,
		EArchonMapTableCommandKind::ExecuteOrder,
		Context);

	const FArchonRtsAuthorityDecision Decision = UArchonTeamRtsPolicyLibrary::FinalizeTeamCommand(
		MapDecision,
		MakeValidIntent(),
		9);

	TestFalse(TEXT("No-op order is not accepted"), Decision.bAccepted);
	TestFalse(TEXT("No-op order cannot mutate team state"), Decision.bMutatesTeamState);
	TestEqual(TEXT("No-op order preserves command sequence"), Decision.FinalSequence, 9);
	TestEqual(TEXT("No-op reason is stable"), Decision.Reason, FName(TEXT("map_table_no_op")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonInvalidIntentCannotMutateRtsStateTest,
	"ArchonFactory.TeamRts.InvalidIntentCannotMutateState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonInvalidIntentCannotMutateRtsStateTest::RunTest(const FString& Parameters)
{
	FArchonRtsCommandIntent Intent = MakeValidIntent();
	Intent.TeamId = INDEX_NONE;

	FArchonMapTableDecision MapDecision;
	MapDecision.bWillExecute = true;

	const FArchonRtsAuthorityDecision Decision = UArchonTeamRtsPolicyLibrary::FinalizeTeamCommand(
		MapDecision,
		Intent,
		3);

	TestFalse(TEXT("Missing team order is not accepted"), Decision.bAccepted);
	TestFalse(TEXT("Missing team order cannot mutate state"), Decision.bMutatesTeamState);
	TestEqual(TEXT("Invalid intent reason is stable"), Decision.Reason, FName(TEXT("missing_team")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonSetRallyPointRequiresTargetLocationTest,
	"ArchonFactory.TeamRts.SetRallyPointRequiresTargetLocation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonSetRallyPointRequiresTargetLocationTest::RunTest(const FString& Parameters)
{
	FArchonRtsCommandIntent Intent = MakeValidIntent();
	Intent.OrderKind = EArchonRtsOrderKind::SetRallyPoint;
	Intent.TargetId = TEXT("forward_rally");
	Intent.bTargetLocationValid = false;

	FArchonMapTableDecision MapDecision;
	MapDecision.bWillExecute = true;

	const FArchonRtsAuthorityDecision Decision = UArchonTeamRtsPolicyLibrary::FinalizeTeamCommand(
		MapDecision,
		Intent,
		6);

	TestFalse(TEXT("Rally order without a point is not accepted"), Decision.bAccepted);
	TestEqual(TEXT("Rally rejection reason is stable"), Decision.Reason, FName(TEXT("rally_missing_location")));
	TestEqual(TEXT("Rejected rally preserves sequence"), Decision.FinalSequence, 6);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonSetRallyPointMutatesTeamStreamTest,
	"ArchonFactory.TeamRts.SetRallyPointMutatesTeamStream",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonSetRallyPointMutatesTeamStreamTest::RunTest(const FString& Parameters)
{
	FArchonRtsCommandIntent Intent = MakeValidIntent();
	Intent.OrderKind = EArchonRtsOrderKind::SetRallyPoint;
	Intent.TargetId = TEXT("forward_rally");
	Intent.TargetLocation = FVector(1200.0f, 500.0f, 0.0f);
	Intent.bTargetLocationValid = true;

	FArchonMapTableDecision MapDecision;
	MapDecision.bWillExecute = true;

	const FArchonRtsAuthorityDecision Decision = UArchonTeamRtsPolicyLibrary::FinalizeTeamCommand(
		MapDecision,
		Intent,
		6);

	TestTrue(TEXT("Rally order is accepted with a valid point"), Decision.bAccepted);
	TestTrue(TEXT("Rally order mutates team state"), Decision.bMutatesTeamState);
	TestEqual(TEXT("Accepted rally increments sequence"), Decision.FinalSequence, 7);
	return true;
}

#endif
