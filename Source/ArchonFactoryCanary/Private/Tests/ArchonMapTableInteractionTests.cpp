#if WITH_DEV_AUTOMATION_TESTS

#include "ArchonMapTableActor.h"
#include "ArchonMapTableInteractorComponent.h"
#include "ArchonTeamRtsStateComponent.h"
#include "Misc/AutomationTest.h"

namespace
{
	UArchonMapTableInteractorComponent* MakeInteractor(int32 PlayerId, int32 TeamId)
	{
		UArchonMapTableInteractorComponent* Interactor = NewObject<UArchonMapTableInteractorComponent>();
		FArchonMapTableInteractorConfig Config;
		Config.PlayerId = PlayerId;
		Config.TeamId = TeamId;
		Interactor->ConfigureInteractor(Config);
		return Interactor;
	}

	FArchonRtsCommandIntent MakeInteractionIntent()
	{
		FArchonRtsCommandIntent Intent;
		Intent.SubjectId = TEXT("interaction_squad");
		Intent.TargetId = TEXT("interaction_target");
		return Intent;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonInteractorPreviewOpensMapTableSurfaceTest,
	"ArchonFactory.Interaction.PreviewOpensMapTableSurface",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonInteractorPreviewOpensMapTableSurfaceTest::RunTest(const FString& Parameters)
{
	AArchonMapTableActor* MapTable = NewObject<AArchonMapTableActor>();
	MapTable->ConfigureMapTable(8, EArchonSessionRoute::PrivateHost, false);

	UArchonMapTableInteractorComponent* Interactor = MakeInteractor(101, 8);
	FArchonMapTableInteractionResult Result;
	const bool bOpened = Interactor->PreviewMapTable(MapTable, Result);

	TestTrue(TEXT("Interactor opens the map-table surface for same-team preview"), bOpened);
	TestTrue(TEXT("Preview reports opened surface"), Result.bOpenedTableSurface);
	TestTrue(TEXT("Preview remains preview-only"), Result.bPreviewOnly);
	TestFalse(TEXT("Preview does not submit an order"), Result.bSubmittedOrder);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonInteractorSubmitLocalOrderMutatesTeamStateTest,
	"ArchonFactory.Interaction.SubmitLocalOrderMutatesTeamState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonInteractorSubmitLocalOrderMutatesTeamStateTest::RunTest(const FString& Parameters)
{
	UArchonTeamRtsStateComponent* TeamState = NewObject<UArchonTeamRtsStateComponent>();
	TeamState->ConfigureTeamState(9);

	AArchonMapTableActor* MapTable = NewObject<AArchonMapTableActor>();
	MapTable->ConfigureMapTable(9, EArchonSessionRoute::LANHosted, false);
	MapTable->AttachTeamState(TeamState);

	UArchonMapTableInteractorComponent* Interactor = MakeInteractor(202, 9);
	FArchonMapTableInteractionResult Result;
	const bool bSubmitted = Interactor->SubmitRtsOrder(MapTable, MakeInteractionIntent(), Result);

	TestTrue(TEXT("Local/LAN interactor can submit order"), bSubmitted);
	TestTrue(TEXT("Submitted order mutates team state"), Result.AuthorityDecision.bMutatesTeamState);
	TestEqual(TEXT("Team stream increments once"), TeamState->GetCurrentCommandSequence(), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonInteractorSteamOnlineFreeSubmitIsNoOpTest,
	"ArchonFactory.Interaction.SteamOnlineFreeSubmitIsNoOp",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonInteractorSteamOnlineFreeSubmitIsNoOpTest::RunTest(const FString& Parameters)
{
	UArchonTeamRtsStateComponent* TeamState = NewObject<UArchonTeamRtsStateComponent>();
	TeamState->ConfigureTeamState(10);

	AArchonMapTableActor* MapTable = NewObject<AArchonMapTableActor>();
	MapTable->ConfigureMapTable(10, EArchonSessionRoute::SteamOnline, false);
	MapTable->AttachTeamState(TeamState);

	UArchonMapTableInteractorComponent* Interactor = MakeInteractor(303, 10);
	FArchonMapTableInteractionResult Result;
	const bool bSubmitted = Interactor->SubmitRtsOrder(MapTable, MakeInteractionIntent(), Result);

	TestFalse(TEXT("SteamOnline free interactor cannot execute live RTS"), bSubmitted);
	TestTrue(TEXT("SteamOnline free interaction still opens preview surface"), Result.bOpenedTableSurface);
	TestTrue(TEXT("SteamOnline free interaction is preview-only"), Result.bPreviewOnly);
	TestEqual(TEXT("No-op leaves team stream unchanged"), TeamState->GetCurrentCommandSequence(), 0);
	TestEqual(TEXT("No-op reason is stable"), Result.AuthorityDecision.Reason, FName(TEXT("map_table_no_op")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonInteractorWrongTeamCannotOpenTableTest,
	"ArchonFactory.Interaction.WrongTeamCannotOpenTable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonInteractorWrongTeamCannotOpenTableTest::RunTest(const FString& Parameters)
{
	AArchonMapTableActor* MapTable = NewObject<AArchonMapTableActor>();
	MapTable->ConfigureMapTable(11, EArchonSessionRoute::PrivateHost, false);

	UArchonMapTableInteractorComponent* Interactor = MakeInteractor(404, 12);
	FArchonMapTableInteractionResult Result;
	const bool bOpened = Interactor->PreviewMapTable(MapTable, Result);

	TestFalse(TEXT("Wrong-team interactor cannot open table"), bOpened);
	TestFalse(TEXT("Wrong-team interaction does not open surface"), Result.bOpenedTableSurface);
	TestEqual(TEXT("Wrong-team reason is stable"), Result.Reason, FName(TEXT("wrong_team")));
	return true;
}

#endif
