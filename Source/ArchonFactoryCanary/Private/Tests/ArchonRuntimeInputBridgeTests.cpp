#if WITH_DEV_AUTOMATION_TESTS

#include "ArchonCanaryFpsCharacter.h"
#include "ArchonFactionMovementComponent.h"
#include "ArchonMapTableActor.h"
#include "ArchonMapTableWidget.h"
#include "ArchonPlayerInputBridgeComponent.h"
#include "ArchonTeamRtsStateComponent.h"
#include "ArchonVerdantThornsproutBow.h"
#include "Camera/CameraComponent.h"
#include "Misc/AutomationTest.h"

namespace
{
	FArchonMapTableInteractorConfig MakeRuntimeInteractorConfig()
	{
		FArchonMapTableInteractorConfig Config;
		Config.PlayerId = 0;
		Config.TeamId = 0;
		return Config;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonRuntimeInputBridgePreviewUpdatesVisibleMapTableTest,
	"ArchonFactory.RuntimeInputBridge.PreviewUpdatesVisibleMapTable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonRuntimeInputBridgePreviewUpdatesVisibleMapTableTest::RunTest(const FString& Parameters)
{
	AArchonMapTableActor* MapTable = NewObject<AArchonMapTableActor>();
	MapTable->ConfigureMapTable(0, EArchonSessionRoute::PrivateHost, false);

	UArchonPlayerInputBridgeComponent* Bridge = NewObject<UArchonPlayerInputBridgeComponent>();
	Bridge->ConfigureBridge(nullptr, MapTable, MakeRuntimeInteractorConfig());

	const bool bOpened = Bridge->PreviewRuntimeMapTable();

	TestTrue(TEXT("Runtime bridge opens map-table preview"), bOpened);
	TestTrue(TEXT("Runtime bridge tracks open map-table surface"), Bridge->IsMapTableSurfaceOpen());
	TestTrue(TEXT("Visible status reports RTS surface"), MapTable->GetRuntimeStatusText().Contains(TEXT("RTS SURFACE OPEN")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonRuntimeInputBridgeSubmitOrderMutatesVisibleSequenceTest,
	"ArchonFactory.RuntimeInputBridge.SubmitOrderMutatesVisibleSequence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonRuntimeInputBridgeSubmitOrderMutatesVisibleSequenceTest::RunTest(const FString& Parameters)
{
	AArchonMapTableActor* MapTable = NewObject<AArchonMapTableActor>();
	MapTable->ConfigureMapTable(0, EArchonSessionRoute::LANHosted, false);

	UArchonPlayerInputBridgeComponent* Bridge = NewObject<UArchonPlayerInputBridgeComponent>();
	Bridge->ConfigureBridge(nullptr, MapTable, MakeRuntimeInteractorConfig());

	const bool bSubmitted = Bridge->SubmitRuntimeRtsOrder(EArchonRtsOrderKind::MoveSquad);

	TestTrue(TEXT("Runtime bridge submits a standard RTS order"), bSubmitted);
	TestEqual(TEXT("Submitted order count increments"), Bridge->GetSubmittedOrderCount(), 1);
	TestEqual(TEXT("Map-table sequence increments"), MapTable->GetCurrentCommandSequence(), 1);
	TestTrue(TEXT("Visible status reports accepted order"), MapTable->GetRuntimeStatusText().Contains(TEXT("ORDER ACCEPTED")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonRuntimeInputBridgeBuildStructureUsesArmoryIntentTest,
	"ArchonFactory.RuntimeInputBridge.BuildStructureUsesArmoryIntent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonRuntimeInputBridgeBuildStructureUsesArmoryIntentTest::RunTest(const FString& Parameters)
{
	AArchonMapTableActor* MapTable = NewObject<AArchonMapTableActor>();
	MapTable->ConfigureMapTable(0, EArchonSessionRoute::PrivateHost, false);

	UArchonPlayerInputBridgeComponent* Bridge = NewObject<UArchonPlayerInputBridgeComponent>();
	Bridge->ConfigureBridge(nullptr, MapTable, MakeRuntimeInteractorConfig());

	const bool bSubmitted = Bridge->SubmitRuntimeRtsOrder(EArchonRtsOrderKind::BuildStructure);
	const FArchonRtsCommandIntent Intent = MapTable->GetTeamState()->GetLastAcceptedCommandIntent();

	TestTrue(TEXT("Runtime bridge submits a build-structure order"), bSubmitted);
	TestEqual(TEXT("Build order increments the map-table sequence"), MapTable->GetCurrentCommandSequence(), 1);
	TestEqual(TEXT("Build order uses the armory tech id as subject"), Intent.SubjectId, FName(TEXT("armory")));
	TestEqual(TEXT("Build order targets the v0 armory slot"), Intent.TargetId, FName(TEXT("verdant_armory_slot")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonRuntimeInputBridgeTechSelectionCyclesThroughUnitTechTest,
	"ArchonFactory.RuntimeInputBridge.TechSelectionCyclesThroughUnitTech",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonRuntimeInputBridgeTechSelectionCyclesThroughUnitTechTest::RunTest(const FString& Parameters)
{
	UArchonPlayerInputBridgeComponent* Bridge = NewObject<UArchonPlayerInputBridgeComponent>();

	TestEqual(TEXT("Armory is the default build tech"), Bridge->GetSelectedTechBuildingId(), FName(TEXT("armory")));
	TestEqual(TEXT("Cycling once selects Beast Den"), Bridge->CycleSelectedTechBuilding(1), FName(TEXT("beast_den")));
	TestEqual(TEXT("Cycling twice selects Grove"), Bridge->CycleSelectedTechBuilding(1), FName(TEXT("grove")));
	TestEqual(TEXT("Cycling wraps back to Armory"), Bridge->CycleSelectedTechBuilding(1), FName(TEXT("armory")));
	TestEqual(TEXT("Reverse cycling wraps to Grove"), Bridge->CycleSelectedTechBuilding(-1), FName(TEXT("grove")));
	TestFalse(TEXT("Unknown tech id is rejected"), Bridge->SelectTechBuilding(FName(TEXT("stable"))));
	TestEqual(TEXT("Rejected selection keeps the prior tech"), Bridge->GetSelectedTechBuildingId(), FName(TEXT("grove")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonRuntimeInputBridgeBuildStructureUsesSelectedTechIntentTest,
	"ArchonFactory.RuntimeInputBridge.BuildStructureUsesSelectedTechIntent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonRuntimeInputBridgeBuildStructureUsesSelectedTechIntentTest::RunTest(const FString& Parameters)
{
	AArchonMapTableActor* MapTable = NewObject<AArchonMapTableActor>();
	MapTable->ConfigureMapTable(0, EArchonSessionRoute::PrivateHost, false);

	UArchonPlayerInputBridgeComponent* Bridge = NewObject<UArchonPlayerInputBridgeComponent>();
	Bridge->ConfigureBridge(nullptr, MapTable, MakeRuntimeInteractorConfig());

	TestTrue(TEXT("Beast Den can be selected as build tech"), Bridge->SelectTechBuilding(FName(TEXT("beast_den"))));
	const bool bBeastDenSubmitted = Bridge->SubmitRuntimeRtsOrder(EArchonRtsOrderKind::BuildStructure);
	FArchonRtsCommandIntent Intent = MapTable->GetTeamState()->GetLastAcceptedCommandIntent();

	TestTrue(TEXT("Runtime bridge submits Beast Den build order"), bBeastDenSubmitted);
	TestEqual(TEXT("Beast Den build order uses selected tech id"), Intent.SubjectId, FName(TEXT("beast_den")));
	TestEqual(TEXT("Beast Den build order targets its slot"), Intent.TargetId, FName(TEXT("kinwild_beast_den_slot")));

	TestTrue(TEXT("Grove can be selected as build tech"), Bridge->SelectTechBuilding(FName(TEXT("grove"))));
	const bool bGroveSubmitted = Bridge->SubmitRuntimeRtsOrder(EArchonRtsOrderKind::BuildStructure);
	Intent = MapTable->GetTeamState()->GetLastAcceptedCommandIntent();

	TestTrue(TEXT("Runtime bridge submits Grove build order"), bGroveSubmitted);
	TestEqual(TEXT("Grove build order uses selected tech id"), Intent.SubjectId, FName(TEXT("grove")));
	TestEqual(TEXT("Grove build order targets its slot"), Intent.TargetId, FName(TEXT("verdant_grove_slot")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonRuntimeInputBridgeSteamOnlineFreeRemainsPreviewOnlyTest,
	"ArchonFactory.RuntimeInputBridge.SteamOnlineFreeRemainsPreviewOnly",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonRuntimeInputBridgeSteamOnlineFreeRemainsPreviewOnlyTest::RunTest(const FString& Parameters)
{
	AArchonMapTableActor* MapTable = NewObject<AArchonMapTableActor>();
	MapTable->ConfigureMapTable(0, EArchonSessionRoute::SteamOnline, false);

	UArchonPlayerInputBridgeComponent* Bridge = NewObject<UArchonPlayerInputBridgeComponent>();
	Bridge->ConfigureBridge(nullptr, MapTable, MakeRuntimeInteractorConfig());

	const bool bOpened = Bridge->PreviewRuntimeMapTable();
	const bool bSubmitted = Bridge->SubmitRuntimeRtsOrder(EArchonRtsOrderKind::MoveSquad);

	TestTrue(TEXT("SteamOnline free can still preview the map table"), bOpened);
	TestFalse(TEXT("SteamOnline free cannot execute live RTS order"), bSubmitted);
	TestEqual(TEXT("No-op order leaves sequence unchanged"), MapTable->GetCurrentCommandSequence(), 0);
	TestTrue(TEXT("Visible status reports preview-only order"), MapTable->GetRuntimeStatusText().Contains(TEXT("ORDER PREVIEW ONLY")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonRuntimeInputBridgeProofSequenceOpensSubmitsAndClosesTest,
	"ArchonFactory.RuntimeInputBridge.ProofSequenceOpensSubmitsAndCloses",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonRuntimeInputBridgeProofSequenceOpensSubmitsAndClosesTest::RunTest(const FString& Parameters)
{
	AArchonMapTableActor* MapTable = NewObject<AArchonMapTableActor>();
	MapTable->ConfigureMapTable(0, EArchonSessionRoute::PrivateHost, false);

	UArchonPlayerInputBridgeComponent* Bridge = NewObject<UArchonPlayerInputBridgeComponent>();
	Bridge->ConfigureBridge(nullptr, MapTable, MakeRuntimeInteractorConfig());

	const bool bProofRan = Bridge->RunRuntimeRtsProofSequence();

	TestTrue(TEXT("Runtime proof sequence completes"), bProofRan);
	TestTrue(TEXT("Runtime proof sequence records completion"), Bridge->HasRuntimeRtsProofSequenceRun());
	TestFalse(TEXT("Runtime proof sequence closes the map-table surface"), Bridge->IsMapTableSurfaceOpen());
	TestEqual(TEXT("Move and attack orders are submitted"), Bridge->GetSubmittedOrderCount(), 2);
	TestEqual(TEXT("Map-table sequence records both orders"), MapTable->GetCurrentCommandSequence(), 2);
	TestTrue(TEXT("Visible status returns to FPS control"), MapTable->GetRuntimeStatusText().Contains(TEXT("FPS CONTROL ACTIVE")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonRuntimeInputBridgePreviewMountsMapTableWidgetTest,
	"ArchonFactory.RuntimeInputBridge.PreviewMountsMapTableWidget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonRuntimeInputBridgePreviewMountsMapTableWidgetTest::RunTest(const FString& Parameters)
{
	AArchonMapTableActor* MapTable = NewObject<AArchonMapTableActor>();
	MapTable->ConfigureMapTable(0, EArchonSessionRoute::PrivateHost, false);

	UArchonPlayerInputBridgeComponent* Bridge = NewObject<UArchonPlayerInputBridgeComponent>();
	Bridge->ConfigureBridge(nullptr, MapTable, MakeRuntimeInteractorConfig());

	const bool bOpened = Bridge->PreviewRuntimeMapTable();
	const UArchonMapTableWidget* Widget = Bridge->GetRuntimeMapTableWidget();

	TestTrue(TEXT("Runtime bridge opens map-table preview"), bOpened);
	TestNotNull(TEXT("Runtime bridge creates a map-table widget instance"), Widget);
	TestTrue(TEXT("Runtime bridge marks the map-table widget mounted"), Bridge->IsRuntimeMapTableWidgetMounted());
	if (Widget)
	{
		TestEqual(TEXT("Runtime bridge selects one squad"), Widget->GetSelectedSquadIds().Num(), 1);
		if (Widget->GetSelectedSquadIds().Num() > 0)
		{
			TestEqual(TEXT("Runtime bridge seeds and selects the canary squad"), Widget->GetSelectedSquadIds()[0], FName(TEXT("canary_squad")));
		}
		TestTrue(TEXT("Runtime widget can read visibility summary"), Widget->FormatVisibilitySummary().Contains(TEXT("VISION")));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonRuntimeInputBridgeWidgetProofSequenceSelectsAndOrdersTest,
	"ArchonFactory.RuntimeInputBridge.WidgetProofSequenceSelectsAndOrders",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonRuntimeInputBridgeWidgetProofSequenceSelectsAndOrdersTest::RunTest(const FString& Parameters)
{
	AArchonMapTableActor* MapTable = NewObject<AArchonMapTableActor>();
	MapTable->ConfigureMapTable(0, EArchonSessionRoute::PrivateHost, false);

	UArchonPlayerInputBridgeComponent* Bridge = NewObject<UArchonPlayerInputBridgeComponent>();
	Bridge->ConfigureBridge(nullptr, MapTable, MakeRuntimeInteractorConfig());

	const bool bProofRan = Bridge->RunRuntimeMapTableWidgetProofSequence();
	const FArchonRtsCommandIntent LastIntent = MapTable->GetTeamState()->GetLastAcceptedCommandIntent();

	TestTrue(TEXT("Runtime map-table widget proof sequence completes"), bProofRan);
	TestTrue(TEXT("Runtime map-table widget proof records completion"), Bridge->HasRuntimeMapTableWidgetProofSequenceRun());
	TestFalse(TEXT("Runtime map-table widget proof closes the surface"), Bridge->IsMapTableSurfaceOpen());
	TestFalse(TEXT("Runtime map-table widget proof unmounts the widget"), Bridge->IsRuntimeMapTableWidgetMounted());
	TestEqual(TEXT("Widget proof submits one order through the widget"), Bridge->GetRuntimeWidgetSubmittedOrderCount(), 1);
	TestEqual(TEXT("Widget order increments the map-table sequence"), MapTable->GetCurrentCommandSequence(), 1);
	TestEqual(TEXT("Widget-selected squad becomes order subject"), LastIntent.SubjectId, FName(TEXT("canary_squad")));
	TestEqual(TEXT("Widget order uses the proof target id"), LastIntent.TargetId, FName(TEXT("canary_widget_rally")));
	TestTrue(TEXT("Widget order carries a target location"), LastIntent.bTargetLocationValid);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonRuntimeInputBridgeWidgetOrderAtUsesProvidedTargetTest,
	"ArchonFactory.RuntimeInputBridge.WidgetOrderAtUsesProvidedTarget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonRuntimeInputBridgeWidgetOrderAtUsesProvidedTargetTest::RunTest(const FString& Parameters)
{
	AArchonMapTableActor* MapTable = NewObject<AArchonMapTableActor>();
	MapTable->ConfigureMapTable(0, EArchonSessionRoute::PrivateHost, false);

	UArchonPlayerInputBridgeComponent* Bridge = NewObject<UArchonPlayerInputBridgeComponent>();
	Bridge->ConfigureBridge(nullptr, MapTable, MakeRuntimeInteractorConfig());
	Bridge->PreviewRuntimeMapTable();

	const bool bSubmitted = Bridge->SubmitRuntimeMapTableWidgetMoveOrderAt(
		FVector2D(2.0f / 3.0f, 0.50f),
		TEXT("central_splitroot"));
	const FArchonRtsCommandIntent LastIntent = MapTable->GetTeamState()->GetLastAcceptedCommandIntent();

	TestTrue(TEXT("Runtime widget submits the provided target order"), bSubmitted);
	TestEqual(TEXT("Runtime widget order count increments"), Bridge->GetRuntimeWidgetSubmittedOrderCount(), 1);
	TestEqual(TEXT("Provided target id reaches team state"), LastIntent.TargetId, FName(TEXT("central_splitroot")));
	TestTrue(
		TEXT("Runtime widget maps provided table point to world target"),
		LastIntent.TargetLocation.Equals(FVector(2000.0f, 0.0f, 0.0f), 0.1f));
	TestTrue(TEXT("Provided widget order carries a target location"), LastIntent.bTargetLocationValid);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonRuntimeInputBridgeRallyPointUpdatesTeamStateTest,
	"ArchonFactory.RuntimeInputBridge.RallyPointUpdatesTeamState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonRuntimeInputBridgeRallyPointUpdatesTeamStateTest::RunTest(const FString& Parameters)
{
	AArchonMapTableActor* MapTable = NewObject<AArchonMapTableActor>();
	MapTable->ConfigureMapTable(0, EArchonSessionRoute::PrivateHost, false);

	UArchonPlayerInputBridgeComponent* Bridge = NewObject<UArchonPlayerInputBridgeComponent>();
	Bridge->ConfigureBridge(nullptr, MapTable, MakeRuntimeInteractorConfig());
	Bridge->PreviewRuntimeMapTable();

	const bool bSubmitted = Bridge->SubmitRuntimeMapTableWidgetRallyPointAt(
		FVector2D(2.0f / 3.0f, 0.50f),
		TEXT("central_rally"));
	const FArchonRtsCommandIntent LastIntent = MapTable->GetTeamState()->GetLastAcceptedCommandIntent();

	TestTrue(TEXT("Runtime bridge submits a rally point order"), bSubmitted);
	TestEqual(TEXT("Runtime widget order count increments"), Bridge->GetRuntimeWidgetSubmittedOrderCount(), 1);
	TestEqual(TEXT("Submitted order count increments"), Bridge->GetSubmittedOrderCount(), 1);
	TestEqual(TEXT("Rally order reaches team state"), LastIntent.OrderKind, EArchonRtsOrderKind::SetRallyPoint);
	TestEqual(TEXT("Rally order uses team rally subject"), LastIntent.SubjectId, FName(TEXT("team_rally")));
	TestTrue(TEXT("Team state now has a rally point"), MapTable->GetTeamState()->HasTeamRallyPoint());
	TestTrue(
		TEXT("Runtime rally maps table point to world target"),
		MapTable->GetTeamState()->GetTeamRallyPoint().Equals(FVector(2000.0f, 0.0f, 0.0f), 0.1f));
	TestTrue(TEXT("Visible status reports rally update"), MapTable->GetRuntimeStatusText().Contains(TEXT("RALLY POINT SET")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonRuntimeInputBridgeWeaponInputFiresWhenMapTableClosedTest,
	"ArchonFactory.RuntimeInputBridge.WeaponInputFiresWhenMapTableClosed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonRuntimeInputBridgeWeaponInputFiresWhenMapTableClosedTest::RunTest(const FString& Parameters)
{
	AArchonCanaryFpsCharacter* Character = NewObject<AArchonCanaryFpsCharacter>();
	UArchonPlayerInputBridgeComponent* Bridge = NewObject<UArchonPlayerInputBridgeComponent>();

	const bool bRouted = Bridge->ApplyRuntimeWeaponInput(Character, true, false);

	TestTrue(TEXT("Closed map-table surface routes LMB into the Verdant bow"), bRouted);
	TestEqual(TEXT("Bow fires exactly one shot"), Character->GetVerdantBow()->GetShotsFired(), 1);
	TestEqual(TEXT("Bow spends one arrow"), Character->GetVerdantBow()->GetState().CurrentAmmo, 998);
	TestEqual(TEXT("Bow enters fire cycle"), Character->GetVerdantBow()->GetState().FireState, EArchonWeaponFireState::OnFireCycle);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonRuntimeInputBridgeWeaponInputDoesNotFireWhenMapTableOpenTest,
	"ArchonFactory.RuntimeInputBridge.WeaponInputDoesNotFireWhenMapTableOpen",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonRuntimeInputBridgeWeaponInputDoesNotFireWhenMapTableOpenTest::RunTest(const FString& Parameters)
{
	AArchonCanaryFpsCharacter* Character = NewObject<AArchonCanaryFpsCharacter>();
	AArchonMapTableActor* MapTable = NewObject<AArchonMapTableActor>();
	MapTable->ConfigureMapTable(0, EArchonSessionRoute::PrivateHost, false);

	UArchonPlayerInputBridgeComponent* Bridge = NewObject<UArchonPlayerInputBridgeComponent>();
	Bridge->ConfigureBridge(nullptr, MapTable, MakeRuntimeInteractorConfig());
	Bridge->PreviewRuntimeMapTable();

	const bool bRouted = Bridge->ApplyRuntimeWeaponInput(Character, true, false);

	TestFalse(TEXT("Open map-table surface keeps LMB reserved for RTS attack order"), bRouted);
	TestEqual(TEXT("Bow does not fire while map table is open"), Character->GetVerdantBow()->GetShotsFired(), 0);
	TestEqual(TEXT("Bow ammo stays full"), Character->GetVerdantBow()->GetState().CurrentAmmo, 999);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonRuntimeInputBridgeWeaponInputReloadsAfterFireCycleTest,
	"ArchonFactory.RuntimeInputBridge.WeaponInputReloadsAfterFireCycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonRuntimeInputBridgeWeaponInputReloadsAfterFireCycleTest::RunTest(const FString& Parameters)
{
	AArchonCanaryFpsCharacter* Character = NewObject<AArchonCanaryFpsCharacter>();
	UArchonPlayerInputBridgeComponent* Bridge = NewObject<UArchonPlayerInputBridgeComponent>();
	UArchonVerdantThornsproutBow* Bow = Character->GetVerdantBow();

	Bridge->ApplyRuntimeWeaponInput(Character, true, false);
	Bow->TickComponent(Bow->GetStats().FireCycleSeconds + 0.01f, LEVELTICK_All, nullptr);

	const bool bReloadStarted = Bridge->ApplyRuntimeWeaponInput(Character, false, true);
	Bow->TickComponent(Bow->GetStats().ReloadSeconds + 0.01f, LEVELTICK_All, nullptr);

	TestTrue(TEXT("R starts reload after a fired arrow"), bReloadStarted);
	TestEqual(TEXT("Reload completes to full quiver"), Bow->GetState().CurrentAmmo, 999);
	TestEqual(TEXT("Reload count increments"), Bow->GetReloadCount(), 1);
	TestEqual(TEXT("Bow returns ready"), Bow->GetState().FireState, EArchonWeaponFireState::Ready);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonRuntimeInputBridgeWeaponProofSequenceFiresAndReloadsTest,
	"ArchonFactory.RuntimeInputBridge.WeaponProofSequenceFiresAndReloads",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonRuntimeInputBridgeWeaponProofSequenceFiresAndReloadsTest::RunTest(const FString& Parameters)
{
	AArchonCanaryFpsCharacter* Character = NewObject<AArchonCanaryFpsCharacter>();
	UArchonPlayerInputBridgeComponent* Bridge = NewObject<UArchonPlayerInputBridgeComponent>();

	const bool bProofRan = Bridge->RunRuntimeWeaponProofSequence(Character);

	TestTrue(TEXT("Runtime weapon proof sequence completes"), bProofRan);
	TestTrue(TEXT("Runtime weapon proof sequence records completion"), Bridge->HasRuntimeWeaponProofSequenceRun());
	TestEqual(TEXT("Weapon proof fires once"), Character->GetVerdantBow()->GetShotsFired(), 1);
	TestEqual(TEXT("Weapon proof reloads once"), Character->GetVerdantBow()->GetReloadCount(), 1);
	TestEqual(TEXT("Weapon proof ends full"), Character->GetVerdantBow()->GetState().CurrentAmmo, 999);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonRuntimeInputBridgeRootVaultInputSuppressesStandardJumpWhenReadyTest,
	"ArchonFactory.RuntimeInputBridge.RootVaultInputSuppressesStandardJumpWhenReady",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonRuntimeInputBridgeRootVaultInputSuppressesStandardJumpWhenReadyTest::RunTest(const FString& Parameters)
{
	AArchonCanaryFpsCharacter* Character = NewObject<AArchonCanaryFpsCharacter>();
	UArchonFactionMovementComponent* FactionMovement = Character->GetFactionMovement();
	FArchonFactionMovementTuning Tuning = FactionMovement->GetTuning();
	Tuning.bRequireGroundedAtLaunch = false;
	Tuning.MinSprintHeldSeconds = 0.0f;
	FactionMovement->SetTuning(Tuning);

	UArchonPlayerInputBridgeComponent* Bridge = NewObject<UArchonPlayerInputBridgeComponent>();

	const bool bSuppressedStandardJump = Bridge->ApplyRuntimeFactionMovementInput(Character, true, true, false);

	TestTrue(TEXT("Vault-ready sprint+jump suppresses standard character jump"), bSuppressedStandardJump);
	TestFalse(TEXT("Suppressed vault input does not set standard jump flag"), Character->bPressedJump);
	TestTrue(TEXT("Faction movement predicts a launch for the next registered tick"), FactionMovement->WillVaultThisFrame());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonRuntimeInputBridgeJumpBelowRootVaultWindowStaysStandardTest,
	"ArchonFactory.RuntimeInputBridge.JumpBelowRootVaultWindowStaysStandard",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonRuntimeInputBridgeJumpBelowRootVaultWindowStaysStandardTest::RunTest(const FString& Parameters)
{
	AArchonCanaryFpsCharacter* Character = NewObject<AArchonCanaryFpsCharacter>();
	UArchonFactionMovementComponent* FactionMovement = Character->GetFactionMovement();
	FArchonFactionMovementTuning Tuning = FactionMovement->GetTuning();
	Tuning.bRequireGroundedAtLaunch = false;
	FactionMovement->SetTuning(Tuning);

	UArchonPlayerInputBridgeComponent* Bridge = NewObject<UArchonPlayerInputBridgeComponent>();

	const bool bSuppressedStandardJump = Bridge->ApplyRuntimeFactionMovementInput(Character, true, true, false);

	TestFalse(TEXT("Early sprint+jump does not suppress standard character jump"), bSuppressedStandardJump);
	TestTrue(TEXT("Early sprint+jump keeps normal FPS jump behavior"), Character->bPressedJump);
	TestFalse(TEXT("Faction movement does not predict a launch below sprint window"), FactionMovement->WillVaultThisFrame());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonCanaryFpsCharacterHasFirstPersonCameraTest,
	"ArchonFactory.RuntimeInputBridge.CanaryFpsCharacterHasCamera",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonCanaryFpsCharacterHasFirstPersonCameraTest::RunTest(const FString& Parameters)
{
	AArchonCanaryFpsCharacter* Character = NewObject<AArchonCanaryFpsCharacter>();

	TestNotNull(TEXT("Native canary FPS character owns first-person camera"), Character->GetFirstPersonCamera());
	TestTrue(TEXT("First-person camera uses pawn control rotation"), Character->GetFirstPersonCamera()->bUsePawnControlRotation);
	return true;
}

#endif
