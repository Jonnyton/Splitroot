#include "ArchonBotPerceptionPolicyLibrary.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonBotVisionConeTest,
	"ArchonFactory.BotPerception.VisionConeMatchesHumanScreen",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FArchonBotVisionConeTest::RunTest(const FString& Parameters)
{
	const FVector Viewer = FVector::ZeroVector;
	const FVector Forward(1.0f, 0.0f, 0.0f);

	TestTrue(TEXT("Dead ahead in range is seen"),
		UArchonBotPerceptionPolicyLibrary::IsInVisionCone(Viewer, Forward, FVector(1000, 0, 0), 2200.0f, 70.0f));
	TestTrue(TEXT("60 degrees off-axis is inside a 70-degree half-angle"),
		UArchonBotPerceptionPolicyLibrary::IsInVisionCone(Viewer, Forward, FVector(500, 866, 0), 2200.0f, 70.0f));
	TestFalse(TEXT("Directly behind is never seen"),
		UArchonBotPerceptionPolicyLibrary::IsInVisionCone(Viewer, Forward, FVector(-1000, 0, 0), 2200.0f, 70.0f));
	TestFalse(TEXT("In the cone but beyond range is not seen"),
		UArchonBotPerceptionPolicyLibrary::IsInVisionCone(Viewer, Forward, FVector(3000, 0, 0), 2200.0f, 70.0f));
	TestFalse(TEXT("The flanker at 100 degrees is off-screen"),
		UArchonBotPerceptionPolicyLibrary::IsInVisionCone(Viewer, Forward, FVector(-174, 985, 0), 2200.0f, 70.0f));
	TestTrue(TEXT("Height difference does not break the 2D cone"),
		UArchonBotPerceptionPolicyLibrary::IsInVisionCone(Viewer, Forward, FVector(1000, 0, 400), 2200.0f, 70.0f));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonBotHearingAndEyesOnTest,
	"ArchonFactory.BotPerception.HearingAndEyesOnRadii",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FArchonBotHearingAndEyesOnTest::RunTest(const FString& Parameters)
{
	const FVector Viewer = FVector::ZeroVector;

	TestTrue(TEXT("Footsteps behind are heard inside the radius"),
		UArchonBotPerceptionPolicyLibrary::IsWithinHearing(Viewer, FVector(-600, 0, 0), 800.0f));
	TestFalse(TEXT("Footsteps beyond the radius are silent"),
		UArchonBotPerceptionPolicyLibrary::IsWithinHearing(Viewer, FVector(-900, 0, 0), 800.0f));
	TestFalse(TEXT("Zero radius hears nothing"),
		UArchonBotPerceptionPolicyLibrary::IsWithinHearing(Viewer, FVector(1, 0, 0), 0.0f));

	TestTrue(TEXT("Standing by the building earns the arrow's origin"),
		UArchonBotPerceptionPolicyLibrary::EarnsEyesOnOrigin(FVector(2000, 0, 0), Viewer, 2500.0f));
	TestFalse(TEXT("Too far from the building learns nothing"),
		UArchonBotPerceptionPolicyLibrary::EarnsEyesOnOrigin(FVector(3000, 0, 0), Viewer, 2500.0f));

	return true;
}
