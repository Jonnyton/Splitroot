#if WITH_DEV_AUTOMATION_TESTS

#include "ArchonMapTablePolicyLibrary.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonSteamOnlineFreeMapTableNoOpTest,
	"ArchonFactory.MapTable.SteamOnlineFreeNoOp",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonSteamOnlineFreeMapTableNoOpTest::RunTest(const FString& Parameters)
{
	const FArchonMapTableCommandContext Context;
	const FArchonMapTableDecision Decision = UArchonMapTablePolicyLibrary::EvaluateMapTableCommand(
		EArchonSessionRoute::SteamOnline,
		false,
		EArchonMapTableCommandKind::ExecuteOrder,
		Context);

	TestTrue(TEXT("Free online users can inspect"), Decision.bCanInspect);
	TestTrue(TEXT("Free online users can select"), Decision.bCanSelect);
	TestTrue(TEXT("Free online users can preview"), Decision.bCanPreview);
	TestFalse(TEXT("Free online users cannot execute live RTS"), Decision.bWillExecute);
	TestTrue(TEXT("Free online execute becomes explicit no-op"), Decision.bExplicitNoOp);
	TestEqual(TEXT("No-op reason is stable"), Decision.Reason, FName(TEXT("steam_online_preview_no_op")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonSteamOnlinePaidMapTableExecutesTest,
	"ArchonFactory.MapTable.SteamOnlinePaidExecutes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonSteamOnlinePaidMapTableExecutesTest::RunTest(const FString& Parameters)
{
	const FArchonMapTableCommandContext Context;
	const FArchonMapTableDecision Decision = UArchonMapTablePolicyLibrary::EvaluateMapTableCommand(
		EArchonSessionRoute::SteamOnline,
		true,
		EArchonMapTableCommandKind::ExecuteOrder,
		Context);

	TestTrue(TEXT("Paid online RTS executes"), Decision.bWillExecute);
	TestFalse(TEXT("Paid online RTS is not no-op"), Decision.bExplicitNoOp);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonLocalMapTableExecutesFreeTest,
	"ArchonFactory.MapTable.LocalExecutesFree",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonLocalMapTableExecutesFreeTest::RunTest(const FString& Parameters)
{
	const FArchonMapTableCommandContext Context;
	const FArchonMapTableDecision Decision = UArchonMapTablePolicyLibrary::EvaluateMapTableCommand(
		EArchonSessionRoute::LANHosted,
		false,
		EArchonMapTableCommandKind::ExecuteOrder,
		Context);

	TestTrue(TEXT("LANHosted map table executes free"), Decision.bWillExecute);
	TestFalse(TEXT("LANHosted map table is not no-op"), Decision.bExplicitNoOp);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonInvalidMapTableDeniedTest,
	"ArchonFactory.MapTable.InvalidTableDenied",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonInvalidMapTableDeniedTest::RunTest(const FString& Parameters)
{
	FArchonMapTableCommandContext Context;
	Context.bAtValidMapTable = false;

	const FArchonMapTableDecision Decision = UArchonMapTablePolicyLibrary::EvaluateMapTableCommand(
		EArchonSessionRoute::PrivateHost,
		false,
		EArchonMapTableCommandKind::ExecuteOrder,
		Context);

	TestFalse(TEXT("Invalid table denies inspect"), Decision.bCanInspect);
	TestFalse(TEXT("Invalid table denies execution"), Decision.bWillExecute);
	TestEqual(TEXT("Invalid table reason is stable"), Decision.Reason, FName(TEXT("invalid_map_table")));
	return true;
}

#endif
