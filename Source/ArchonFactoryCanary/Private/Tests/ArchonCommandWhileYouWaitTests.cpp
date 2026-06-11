#if WITH_DEV_AUTOMATION_TESTS

#include "ArchonCanaryRtsSquadActor.h"
#include "ArchonMapTableActor.h"
#include "ArchonPlayerInputBridgeComponent.h"
#include "ArchonRespawnStateComponent.h"
#include "ArchonTeamRtsStateComponent.h"
#include "Misc/AutomationTest.h"

namespace
{
	FArchonMapTableInteractorConfig MakeCommandWaitInteractorConfig()
	{
		FArchonMapTableInteractorConfig Config;
		Config.PlayerId = 0;
		Config.TeamId = 0;
		return Config;
	}

	FArchonRespawnSpawnPoint MakeCommandWaitSpawnPoint()
	{
		FArchonRespawnSpawnPoint SpawnPoint;
		SpawnPoint.SpawnPointId = TEXT("base_spawn");
		SpawnPoint.TeamId = 0;
		SpawnPoint.Location = FVector::ZeroVector;
		SpawnPoint.Rotation = FRotator::ZeroRotator;
		SpawnPoint.bIsForwardSpawn = false;
		SpawnPoint.bIsAvailable = true;
		return SpawnPoint;
	}

	struct FCommandWaitHarness
	{
		AArchonMapTableActor* MapTable = nullptr;
		AArchonCanaryRtsSquadActor* Squad = nullptr;
		UArchonPlayerInputBridgeComponent* Bridge = nullptr;
		UArchonRespawnStateComponent* Respawn = nullptr;
	};

	FCommandWaitHarness MakeCommandWaitHarness()
	{
		FCommandWaitHarness Harness;
		Harness.MapTable = NewObject<AArchonMapTableActor>();
		Harness.MapTable->ConfigureMapTable(0, EArchonSessionRoute::PrivateHost, false);

		Harness.Squad = NewObject<AArchonCanaryRtsSquadActor>();
		Harness.Squad->ConfigureSquad(0, TEXT("canary_squad"));
		Harness.Squad->AttachTeamState(Harness.MapTable->GetTeamState());

		Harness.Respawn = NewObject<UArchonRespawnStateComponent>();
		FArchonRespawnTuning Tuning;
		Harness.Respawn->ConfigureRespawn(0, 0, Tuning);
		Harness.Respawn->RegisterSpawnPoint(MakeCommandWaitSpawnPoint());

		Harness.Bridge = NewObject<UArchonPlayerInputBridgeComponent>();
		Harness.Bridge->ConfigureBridge(nullptr, Harness.MapTable, MakeCommandWaitInteractorConfig());
		Harness.Bridge->SetRespawnStateComponent(Harness.Respawn);
		return Harness;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonCommandWhileYouWaitAliveStateAllowsTableInputTest,
	"ArchonFactory.CommandWhileYouWait.AliveStateAllowsTableInput",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonCommandWhileYouWaitAliveStateAllowsTableInputTest::RunTest(const FString& Parameters)
{
	const FCommandWaitHarness Harness = MakeCommandWaitHarness();

	TestTrue(TEXT("Alive state allows table input"), Harness.Bridge->IsDeadStateCommandAllowed());
	TestTrue(TEXT("Alive state can open the runtime map table"), Harness.Bridge->PreviewRuntimeMapTable());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonCommandWhileYouWaitDyingStateBlocksTableInputTest,
	"ArchonFactory.CommandWhileYouWait.DyingStateBlocksTableInput",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonCommandWhileYouWaitDyingStateBlocksTableInputTest::RunTest(const FString& Parameters)
{
	const FCommandWaitHarness Harness = MakeCommandWaitHarness();
	Harness.Respawn->SetLifeStateForProof(EArchonLifeState::Dying);

	TestFalse(TEXT("Dying state blocks table input"), Harness.Bridge->IsDeadStateCommandAllowed());
	TestFalse(TEXT("Dying state cannot open the runtime map table"), Harness.Bridge->PreviewRuntimeMapTable());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonCommandWhileYouWaitDeadStateAllowsTableInputTest,
	"ArchonFactory.CommandWhileYouWait.DeadStateAllowsTableInput",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonCommandWhileYouWaitDeadStateAllowsTableInputTest::RunTest(const FString& Parameters)
{
	const FCommandWaitHarness Harness = MakeCommandWaitHarness();
	Harness.Respawn->HandlePlayerDeath(1, 9);

	TestTrue(TEXT("Dead state allows table input"), Harness.Bridge->IsDeadStateCommandAllowed());
	TestTrue(TEXT("Dead state can open the runtime map table"), Harness.Bridge->PreviewRuntimeMapTable());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonCommandWhileYouWaitRespawningStateBlocksTableInputTest,
	"ArchonFactory.CommandWhileYouWait.RespawningStateBlocksTableInput",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonCommandWhileYouWaitRespawningStateBlocksTableInputTest::RunTest(const FString& Parameters)
{
	const FCommandWaitHarness Harness = MakeCommandWaitHarness();
	Harness.Respawn->HandlePlayerDeath(1, 9);
	Harness.Respawn->TickComponent(5.05f, LEVELTICK_All, nullptr);

	TestFalse(TEXT("Respawning state blocks table input"), Harness.Bridge->IsDeadStateCommandAllowed());
	TestFalse(TEXT("Respawning state cannot open the runtime map table"), Harness.Bridge->PreviewRuntimeMapTable());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonCommandWhileYouWaitOrderSubmittedDuringDeathExecutesTest,
	"ArchonFactory.CommandWhileYouWait.OrderSubmittedDuringDeathExecutes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonCommandWhileYouWaitOrderSubmittedDuringDeathExecutesTest::RunTest(const FString& Parameters)
{
	const FCommandWaitHarness Harness = MakeCommandWaitHarness();
	Harness.Respawn->HandlePlayerDeath(1, 9);
	Harness.Bridge->PreviewRuntimeMapTable();

	const bool bSubmitted = Harness.Bridge->SubmitRuntimeMapTableWidgetMoveOrderAt(
		FVector2D(0.80f, 0.50f),
		TEXT("dead_state_rally"));
	const FArchonRtsCommandIntent LastIntent = Harness.MapTable->GetTeamState()->GetLastAcceptedCommandIntent();

	TestTrue(TEXT("Dead-state widget order submits"), bSubmitted);
	TestEqual(TEXT("Death command counter increments"), Harness.Bridge->GetCommandsIssuedDuringDeath(), 1);
	TestEqual(TEXT("Team command sequence increments"), Harness.MapTable->GetCurrentCommandSequence(), 1);
	TestEqual(TEXT("Squad consumes dead-state command"), Harness.Squad->GetLastAppliedCommandSequence(), 1);
	TestEqual(TEXT("Squad enters moving state"), Harness.Squad->GetOrderState(), EArchonCanaryRtsSquadOrderState::Moving);
	TestEqual(TEXT("Order target is preserved"), LastIntent.TargetId, FName(TEXT("dead_state_rally")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonCommandWhileYouWaitSubmittedOrderSurvivesRespawnTest,
	"ArchonFactory.CommandWhileYouWait.SubmittedOrderSurvivesRespawn",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonCommandWhileYouWaitSubmittedOrderSurvivesRespawnTest::RunTest(const FString& Parameters)
{
	const FCommandWaitHarness Harness = MakeCommandWaitHarness();
	Harness.Respawn->HandlePlayerDeath(1, 9);
	Harness.Bridge->PreviewRuntimeMapTable();
	Harness.Bridge->SubmitRuntimeMapTableWidgetMoveOrderAt(FVector2D(0.80f, 0.50f), TEXT("dead_state_rally"));

	Harness.Respawn->TickComponent(5.05f, LEVELTICK_All, nullptr);
	Harness.Respawn->MarkRespawnComplete();

	TestEqual(TEXT("Respawn returns player alive"), Harness.Respawn->GetLifeState(), EArchonLifeState::Alive);
	TestEqual(TEXT("Dead-state command counter is retained"), Harness.Bridge->GetCommandsIssuedDuringDeath(), 1);
	TestEqual(TEXT("Submitted team command sequence survives respawn"), Harness.MapTable->GetCurrentCommandSequence(), 1);
	TestEqual(TEXT("Squad still carries the command issued while dead"), Harness.Squad->GetLastAppliedCommandSequence(), 1);
	TestEqual(TEXT("Squad remains on move order after respawn"), Harness.Squad->GetOrderState(), EArchonCanaryRtsSquadOrderState::Moving);
	return true;
}

#endif
