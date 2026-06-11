#if WITH_DEV_AUTOMATION_TESTS

#include "ArchonCanaryRtsSquadActor.h"
#include "ArchonMapTableActor.h"
#include "ArchonTeamRtsStateComponent.h"
#include "Misc/AutomationTest.h"

namespace
{
	FArchonRtsCommandIntent MakeSquadIntent(EArchonRtsOrderKind OrderKind)
	{
		FArchonRtsCommandIntent Intent;
		Intent.TeamId = 0;
		Intent.IssuingPlayerId = 7;
		Intent.OrderKind = OrderKind;
		Intent.SubjectId = TEXT("canary_squad");
		Intent.TargetId = OrderKind == EArchonRtsOrderKind::AttackTarget ? TEXT("canary_target") : TEXT("canary_rally_point");
		return Intent;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonRuntimeRtsSquadMoveOrderSetsVisibleMovementStateTest,
	"ArchonFactory.RuntimeRtsSquad.MoveOrderSetsVisibleMovementState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonRuntimeRtsSquadMoveOrderSetsVisibleMovementStateTest::RunTest(const FString& Parameters)
{
	UArchonTeamRtsStateComponent* TeamState = NewObject<UArchonTeamRtsStateComponent>();
	TeamState->ConfigureTeamState(0);

	AArchonCanaryRtsSquadActor* Squad = NewObject<AArchonCanaryRtsSquadActor>();
	Squad->ConfigureSquad(0, TEXT("canary_squad"));
	Squad->AttachTeamState(TeamState);

	AArchonMapTableActor* MapTable = NewObject<AArchonMapTableActor>();
	MapTable->ConfigureMapTable(0, EArchonSessionRoute::PrivateHost, false);
	MapTable->AttachTeamState(TeamState);

	FArchonRtsAuthorityDecision Decision;
	const bool bAccepted = MapTable->SubmitCommand(
		EArchonMapTableCommandKind::ExecuteOrder,
		FArchonMapTableCommandContext(),
		MakeSquadIntent(EArchonRtsOrderKind::MoveSquad),
		Decision);

	TestTrue(TEXT("Map-table move order is accepted"), bAccepted);
	TestEqual(TEXT("Team command sequence increments"), TeamState->GetCurrentCommandSequence(), 1);
	TestEqual(TEXT("Squad consumes accepted command sequence"), Squad->GetLastAppliedCommandSequence(), 1);
	TestEqual(TEXT("Squad enters visible moving state"), Squad->GetOrderState(), EArchonCanaryRtsSquadOrderState::Moving);
	TestTrue(TEXT("Squad status exposes movement to runtime proof"), Squad->GetRuntimeStatusText().Contains(TEXT("MOVING")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonRuntimeRtsSquadAttackOrderSetsVisibleAttackStateTest,
	"ArchonFactory.RuntimeRtsSquad.AttackOrderSetsVisibleAttackState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonRuntimeRtsSquadAttackOrderSetsVisibleAttackStateTest::RunTest(const FString& Parameters)
{
	AArchonCanaryRtsSquadActor* Squad = NewObject<AArchonCanaryRtsSquadActor>();
	Squad->ConfigureSquad(0, TEXT("canary_squad"));

	FArchonRtsAuthorityDecision Decision;
	Decision.bAccepted = true;
	Decision.bMutatesTeamState = true;
	Decision.FinalSequence = 3;
	Decision.Reason = TEXT("accepted_standard_archon_order");

	const bool bApplied = Squad->ApplyAcceptedCommand(
		MakeSquadIntent(EArchonRtsOrderKind::AttackTarget),
		Decision);

	TestTrue(TEXT("Squad applies matching attack order"), bApplied);
	TestEqual(TEXT("Squad records attack command sequence"), Squad->GetLastAppliedCommandSequence(), 3);
	TestEqual(TEXT("Squad enters visible attack state"), Squad->GetOrderState(), EArchonCanaryRtsSquadOrderState::Attacking);
	TestTrue(TEXT("Squad status exposes attack to runtime proof"), Squad->GetRuntimeStatusText().Contains(TEXT("ATTACKING")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonRuntimeRtsSquadMoveOrderWithLocationUsesIntentLocationTest,
	"ArchonFactory.RuntimeRtsSquad.MoveOrderWithLocationUsesIntentLocation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonRuntimeRtsSquadMoveOrderWithLocationUsesIntentLocationTest::RunTest(const FString& Parameters)
{
	AArchonCanaryRtsSquadActor* Squad = NewObject<AArchonCanaryRtsSquadActor>();
	Squad->ConfigureSquad(0, TEXT("canary_squad"));

	FArchonRtsCommandIntent Intent = MakeSquadIntent(EArchonRtsOrderKind::MoveSquad);
	Intent.bTargetLocationValid = true;
	Intent.TargetLocation = FVector(1234.0f, 5678.0f, 100.0f);

	FArchonRtsAuthorityDecision Decision;
	Decision.bAccepted = true;
	Decision.bMutatesTeamState = true;
	Decision.FinalSequence = 1;
	Decision.Reason = TEXT("accepted_standard_archon_order");

	const bool bApplied = Squad->ApplyAcceptedCommand(Intent, Decision);

	TestTrue(TEXT("Squad applies move order with valid target location"), bApplied);
	TestEqual(TEXT("Squad uses intent target location instead of fallback"), Squad->GetActiveDestination(), FVector(1234.0f, 5678.0f, 100.0f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonRuntimeRtsSquadIgnoresTeamRallyPointOrderTest,
	"ArchonFactory.RuntimeRtsSquad.IgnoresTeamRallyPointOrder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonRuntimeRtsSquadIgnoresTeamRallyPointOrderTest::RunTest(const FString& Parameters)
{
	AArchonCanaryRtsSquadActor* Squad = NewObject<AArchonCanaryRtsSquadActor>();
	Squad->ConfigureSquad(0, TEXT("canary_squad"));

	FArchonRtsCommandIntent Intent = MakeSquadIntent(EArchonRtsOrderKind::SetRallyPoint);
	Intent.SubjectId = TEXT("team_rally");
	Intent.TargetId = TEXT("forward_rally");
	Intent.bTargetLocationValid = true;
	Intent.TargetLocation = FVector(1234.0f, 5678.0f, 100.0f);

	FArchonRtsAuthorityDecision Decision;
	Decision.bAccepted = true;
	Decision.bMutatesTeamState = true;
	Decision.FinalSequence = 1;
	Decision.Reason = TEXT("accepted_standard_archon_order");

	const bool bApplied = Squad->ApplyAcceptedCommand(Intent, Decision);

	TestFalse(TEXT("Squad does not consume team rally order"), bApplied);
	TestEqual(TEXT("Squad command sequence remains unchanged"), Squad->GetLastAppliedCommandSequence(), 0);
	TestEqual(TEXT("Squad remains idle"), Squad->GetOrderState(), EArchonCanaryRtsSquadOrderState::Idle);
	return true;
}

#endif
