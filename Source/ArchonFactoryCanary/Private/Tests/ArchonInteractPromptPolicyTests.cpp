#include "ArchonInteractPromptPolicyLibrary.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonInteractPromptInRangeShows,
	"ArchonFactory.Interact.InRangeShowsPrompt",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FArchonInteractPromptInRangeShows::RunTest(const FString& Parameters)
{
	const FArchonInteractPromptDecision Decision = UArchonInteractPromptPolicyLibrary::EvaluateInteractPrompt(
		FVector(100.0f, 0.0f, 0.0f),
		FVector(500.0f, 0.0f, 0.0f),
		UArchonInteractPromptPolicyLibrary::DefaultInteractRadius,
		/*bInterfaceAlreadyOpen=*/ false);
	TestTrue(TEXT("400uu away with 600uu radius is in range"), Decision.bInRange);
	TestTrue(TEXT("prompt shows when in range and interface closed"), Decision.bShowPrompt);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonInteractPromptOutOfRangeHides,
	"ArchonFactory.Interact.OutOfRangeHidesPrompt",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FArchonInteractPromptOutOfRangeHides::RunTest(const FString& Parameters)
{
	const FArchonInteractPromptDecision Decision = UArchonInteractPromptPolicyLibrary::EvaluateInteractPrompt(
		FVector::ZeroVector,
		FVector(2000.0f, 0.0f, 0.0f),
		UArchonInteractPromptPolicyLibrary::DefaultInteractRadius,
		/*bInterfaceAlreadyOpen=*/ false);
	TestFalse(TEXT("2000uu away with 600uu radius is out of range"), Decision.bInRange);
	TestFalse(TEXT("no prompt out of range"), Decision.bShowPrompt);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonInteractPromptHeightTolerant,
	"ArchonFactory.Interact.HeightDifferenceDoesNotBreakRange",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FArchonInteractPromptHeightTolerant::RunTest(const FString& Parameters)
{
	const FArchonInteractPromptDecision Decision = UArchonInteractPromptPolicyLibrary::EvaluateInteractPrompt(
		FVector(0.0f, 0.0f, 0.0f),
		FVector(300.0f, 0.0f, 400.0f),
		UArchonInteractPromptPolicyLibrary::DefaultInteractRadius,
		/*bInterfaceAlreadyOpen=*/ false);
	TestTrue(TEXT("vertical offset is ignored (2D range)"), Decision.bInRange);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonInteractPromptOpenSuppresses,
	"ArchonFactory.Interact.OpenInterfaceSuppressesPrompt",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FArchonInteractPromptOpenSuppresses::RunTest(const FString& Parameters)
{
	const FArchonInteractPromptDecision Decision = UArchonInteractPromptPolicyLibrary::EvaluateInteractPrompt(
		FVector::ZeroVector,
		FVector(100.0f, 0.0f, 0.0f),
		UArchonInteractPromptPolicyLibrary::DefaultInteractRadius,
		/*bInterfaceAlreadyOpen=*/ true);
	TestTrue(TEXT("still in range"), Decision.bInRange);
	TestFalse(TEXT("prompt hidden while its interface is open"), Decision.bShowPrompt);
	return true;
}
