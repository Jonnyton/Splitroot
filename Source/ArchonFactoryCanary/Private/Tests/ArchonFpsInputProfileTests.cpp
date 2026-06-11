#if WITH_DEV_AUTOMATION_TESTS

#include "ArchonFpsInputProfile.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonFpsInputWASDMovementTest,
	"ArchonFactory.Input.WASDMovementDefaults",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonFpsInputWASDMovementTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("W moves forward"), UArchonFpsInputProfile::HasDefaultKeyboardMouseBinding(EArchonFpsInputAction::MoveForward, EKeys::W, 1.0f));
	TestTrue(TEXT("S moves backward"), UArchonFpsInputProfile::HasDefaultKeyboardMouseBinding(EArchonFpsInputAction::MoveBackward, EKeys::S, -1.0f));
	TestTrue(TEXT("A strafes left"), UArchonFpsInputProfile::HasDefaultKeyboardMouseBinding(EArchonFpsInputAction::MoveLeft, EKeys::A, -1.0f));
	TestTrue(TEXT("D strafes right"), UArchonFpsInputProfile::HasDefaultKeyboardMouseBinding(EArchonFpsInputAction::MoveRight, EKeys::D, 1.0f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonFpsInputMouseLookTest,
	"ArchonFactory.Input.MouseLookDefaults",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonFpsInputMouseLookTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("Mouse X controls yaw"), UArchonFpsInputProfile::HasDefaultKeyboardMouseBinding(EArchonFpsInputAction::LookYaw, EKeys::MouseX, 1.0f));
	TestTrue(TEXT("Mouse Y controls pitch"), UArchonFpsInputProfile::HasDefaultKeyboardMouseBinding(EArchonFpsInputAction::LookPitch, EKeys::MouseY, 1.0f));
	TestTrue(TEXT("Yaw is a mouse axis"), UArchonFpsInputProfile::UsesMouseAxisForLook(EArchonFpsInputAction::LookYaw));
	TestTrue(TEXT("Pitch is a mouse axis"), UArchonFpsInputProfile::UsesMouseAxisForLook(EArchonFpsInputAction::LookPitch));
	TestFalse(TEXT("Archon PC FPS standard does not allow mouse smoothing"), UArchonFpsInputProfile::IsMouseSmoothingAllowedByArchonStandard());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonFpsInputCombatAndUseDefaultsTest,
	"ArchonFactory.Input.CombatAndUseDefaults",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonFpsInputCombatAndUseDefaultsTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("Left mouse fires primary"), UArchonFpsInputProfile::HasDefaultKeyboardMouseBinding(EArchonFpsInputAction::PrimaryFire, EKeys::LeftMouseButton, 1.0f));
	TestTrue(TEXT("Right mouse fires secondary"), UArchonFpsInputProfile::HasDefaultKeyboardMouseBinding(EArchonFpsInputAction::SecondaryFire, EKeys::RightMouseButton, 1.0f));
	TestTrue(TEXT("R reloads"), UArchonFpsInputProfile::HasDefaultKeyboardMouseBinding(EArchonFpsInputAction::Reload, EKeys::R, 1.0f));
	TestTrue(TEXT("E interacts"), UArchonFpsInputProfile::HasDefaultKeyboardMouseBinding(EArchonFpsInputAction::Interact, EKeys::E, 1.0f));
	TestTrue(TEXT("Tab toggles the map-table surface"), UArchonFpsInputProfile::HasDefaultKeyboardMouseBinding(EArchonFpsInputAction::ToggleMapTable, EKeys::Tab, 1.0f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonFpsInputMobilityAndMenuDefaultsTest,
	"ArchonFactory.Input.MobilityAndMenuDefaults",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonFpsInputMobilityAndMenuDefaultsTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("Space jumps"), UArchonFpsInputProfile::HasDefaultKeyboardMouseBinding(EArchonFpsInputAction::Jump, EKeys::SpaceBar, 1.0f));
	TestTrue(TEXT("Left shift sprints"), UArchonFpsInputProfile::HasDefaultKeyboardMouseBinding(EArchonFpsInputAction::Sprint, EKeys::LeftShift, 1.0f));
	TestTrue(TEXT("Left control crouches"), UArchonFpsInputProfile::HasDefaultKeyboardMouseBinding(EArchonFpsInputAction::Crouch, EKeys::LeftControl, 1.0f));
	TestTrue(TEXT("C also crouches"), UArchonFpsInputProfile::HasDefaultKeyboardMouseBinding(EArchonFpsInputAction::Crouch, EKeys::C, 1.0f));
	TestTrue(TEXT("Escape pauses"), UArchonFpsInputProfile::HasDefaultKeyboardMouseBinding(EArchonFpsInputAction::Pause, EKeys::Escape, 1.0f));
	return true;
}

#endif
