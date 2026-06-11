#if WITH_DEV_AUTOMATION_TESTS

#include "ArchonFactionMovementPolicyLibrary.h"
#include "Misc/AutomationTest.h"

namespace
{
	FArchonFactionMovementTuning DefaultTuning()
	{
		return FArchonFactionMovementTuning();
	}

	FArchonFactionMovementInputState MakeInput(
		bool bSprintHeld,
		float SprintHeldSeconds,
		bool bJumpPressed,
		bool bGrounded)
	{
		FArchonFactionMovementInputState Input;
		Input.bSprintHeld = bSprintHeld;
		Input.SprintHeldSeconds = SprintHeldSeconds;
		Input.bJumpPressedThisFrame = bJumpPressed;
		Input.bGrounded = bGrounded;
		return Input;
	}

	FArchonFactionMovementCooldown ReadyCooldown()
	{
		return FArchonFactionMovementCooldown();
	}

	FArchonFactionMovementCooldown OnCooldown(float Remaining, float Total)
	{
		FArchonFactionMovementCooldown C;
		C.SecondsRemaining = Remaining;
		C.TotalSeconds = Total;
		return C;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonLocomotionVerdantVerbIsRootVaultTest,
	"ArchonFactory.Locomotion.VerdantVerbIsRootVault",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonLocomotionVerdantVerbIsRootVaultTest::RunTest(const FString& Parameters)
{
	const EArchonFactionMovementVerb Verb =
		UArchonFactionMovementPolicyLibrary::GetMovementVerbForFaction(EArchonFaction::VerdantChoir);
	TestEqual(TEXT("Verdant Choir movement verb is VerdantRootVault"), Verb, EArchonFactionMovementVerb::VerdantRootVault);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonLocomotionOtherFactionsHaveNoVerbAtV0Test,
	"ArchonFactory.Locomotion.OtherFactionsHaveNoVerbAtV0",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonLocomotionOtherFactionsHaveNoVerbAtV0Test::RunTest(const FString& Parameters)
{
	TestEqual(
		TEXT("Kinwild has no v0 movement verb"),
		UArchonFactionMovementPolicyLibrary::GetMovementVerbForFaction(EArchonFaction::KinwildCovenant),
		EArchonFactionMovementVerb::None);
	TestEqual(
		TEXT("Lenswright has no v0 movement verb"),
		UArchonFactionMovementPolicyLibrary::GetMovementVerbForFaction(EArchonFaction::LenswrightCompact),
		EArchonFactionMovementVerb::None);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonLocomotionNoLaunchWithoutSprintTest,
	"ArchonFactory.Locomotion.NoLaunchWithoutSprint",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonLocomotionNoLaunchWithoutSprintTest::RunTest(const FString& Parameters)
{
	const FArchonFactionMovementDecision Decision = UArchonFactionMovementPolicyLibrary::EvaluateLaunch(
		EArchonFaction::VerdantChoir,
		MakeInput(/*bSprintHeld*/ false, /*SprintHeldSeconds*/ 0.0f, /*bJumpPressed*/ true, /*bGrounded*/ true),
		ReadyCooldown(),
		DefaultTuning());
	TestFalse(TEXT("Jump without sprint should not vault"), Decision.bShouldLaunch);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonLocomotionNoLaunchBelowMinSprintWindowTest,
	"ArchonFactory.Locomotion.NoLaunchBelowMinSprintWindow",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonLocomotionNoLaunchBelowMinSprintWindowTest::RunTest(const FString& Parameters)
{
	const FArchonFactionMovementDecision Decision = UArchonFactionMovementPolicyLibrary::EvaluateLaunch(
		EArchonFaction::VerdantChoir,
		MakeInput(true, 0.10f, true, true),
		ReadyCooldown(),
		DefaultTuning());
	TestFalse(TEXT("Sprint window below 0.15s anti-accident threshold should not vault"), Decision.bShouldLaunch);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonLocomotionLaunchAtOrAboveMinSprintWindowTest,
	"ArchonFactory.Locomotion.LaunchAtOrAboveMinSprintWindow",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonLocomotionLaunchAtOrAboveMinSprintWindowTest::RunTest(const FString& Parameters)
{
	const FArchonFactionMovementDecision Decision = UArchonFactionMovementPolicyLibrary::EvaluateLaunch(
		EArchonFaction::VerdantChoir,
		MakeInput(true, 0.15f, true, true),
		ReadyCooldown(),
		DefaultTuning());
	TestTrue(TEXT("Sprint window at 0.15s should vault"), Decision.bShouldLaunch);
	TestEqual(TEXT("Verb triggered is Verdant root-vault"), Decision.VerbTriggered, EArchonFactionMovementVerb::VerdantRootVault);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonLocomotionNoLaunchWhileAirborneByDefaultTest,
	"ArchonFactory.Locomotion.NoLaunchWhileAirborneByDefault",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonLocomotionNoLaunchWhileAirborneByDefaultTest::RunTest(const FString& Parameters)
{
	const FArchonFactionMovementDecision Decision = UArchonFactionMovementPolicyLibrary::EvaluateLaunch(
		EArchonFaction::VerdantChoir,
		MakeInput(true, 0.2f, true, /*bGrounded*/ false),
		ReadyCooldown(),
		DefaultTuning());
	TestFalse(TEXT("Default tuning requires grounded; airborne should not vault"), Decision.bShouldLaunch);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonLocomotionLaunchAirborneWhenGroundedRequirementOffTest,
	"ArchonFactory.Locomotion.LaunchAirborneWhenGroundedRequirementOff",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonLocomotionLaunchAirborneWhenGroundedRequirementOffTest::RunTest(const FString& Parameters)
{
	FArchonFactionMovementTuning Tuning = DefaultTuning();
	Tuning.bRequireGroundedAtLaunch = false;

	const FArchonFactionMovementDecision Decision = UArchonFactionMovementPolicyLibrary::EvaluateLaunch(
		EArchonFaction::VerdantChoir,
		MakeInput(true, 0.2f, true, false),
		ReadyCooldown(),
		Tuning);
	TestTrue(TEXT("Airborne vault allowed when grounded requirement is off"), Decision.bShouldLaunch);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonLocomotionLaunchAppliesExpectedImpulseValuesTest,
	"ArchonFactory.Locomotion.LaunchAppliesExpectedImpulseValues",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonLocomotionLaunchAppliesExpectedImpulseValuesTest::RunTest(const FString& Parameters)
{
	const FArchonFactionMovementDecision Decision = UArchonFactionMovementPolicyLibrary::EvaluateLaunch(
		EArchonFaction::VerdantChoir,
		MakeInput(true, 0.2f, true, true),
		ReadyCooldown(),
		DefaultTuning());
	TestEqual(TEXT("Forward impulse magnitude matches tuning default"), Decision.ForwardImpulse, 850.0f);
	TestEqual(TEXT("Vertical impulse magnitude matches tuning default"), Decision.VerticalImpulse, 450.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonLocomotionLaunchStartsFullCooldownTest,
	"ArchonFactory.Locomotion.LaunchStartsFullCooldown",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonLocomotionLaunchStartsFullCooldownTest::RunTest(const FString& Parameters)
{
	const FArchonFactionMovementDecision Decision = UArchonFactionMovementPolicyLibrary::EvaluateLaunch(
		EArchonFaction::VerdantChoir,
		MakeInput(true, 0.2f, true, true),
		ReadyCooldown(),
		DefaultTuning());
	TestEqual(TEXT("Cooldown starts at full 3.0s remaining"), Decision.NewCooldown.SecondsRemaining, 3.0f);
	TestEqual(TEXT("Cooldown total is 3.0s"), Decision.NewCooldown.TotalSeconds, 3.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonLocomotionNoLaunchWhileOnCooldownTest,
	"ArchonFactory.Locomotion.NoLaunchWhileOnCooldown",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonLocomotionNoLaunchWhileOnCooldownTest::RunTest(const FString& Parameters)
{
	const FArchonFactionMovementDecision Decision = UArchonFactionMovementPolicyLibrary::EvaluateLaunch(
		EArchonFaction::VerdantChoir,
		MakeInput(true, 0.2f, true, true),
		OnCooldown(1.5f, 3.0f),
		DefaultTuning());
	TestFalse(TEXT("Vault on cooldown should not launch"), Decision.bShouldLaunch);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonLocomotionCooldownAdvancesAndReadyAtZeroTest,
	"ArchonFactory.Locomotion.CooldownAdvancesAndReadyAtZero",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonLocomotionCooldownAdvancesAndReadyAtZeroTest::RunTest(const FString& Parameters)
{
	const FArchonFactionMovementCooldown AfterOneSecond =
		UArchonFactionMovementPolicyLibrary::AdvanceCooldown(OnCooldown(3.0f, 3.0f), 1.0f);
	TestEqual(TEXT("Cooldown advances by delta"), AfterOneSecond.SecondsRemaining, 2.0f);
	TestEqual(TEXT("Cooldown total unchanged"), AfterOneSecond.TotalSeconds, 3.0f);

	const FArchonFactionMovementCooldown AtZero =
		UArchonFactionMovementPolicyLibrary::AdvanceCooldown(OnCooldown(0.5f, 3.0f), 0.5f);
	TestTrue(TEXT("Cooldown is ready when remaining hits zero"), UArchonFactionMovementPolicyLibrary::IsCooldownReady(AtZero));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonLocomotionCooldownClampsAtZeroTest,
	"ArchonFactory.Locomotion.CooldownClampsAtZero",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonLocomotionCooldownClampsAtZeroTest::RunTest(const FString& Parameters)
{
	const FArchonFactionMovementCooldown Clamped =
		UArchonFactionMovementPolicyLibrary::AdvanceCooldown(OnCooldown(0.2f, 3.0f), 5.0f);
	TestEqual(TEXT("Cooldown does not go negative"), Clamped.SecondsRemaining, 0.0f);
	TestEqual(TEXT("Cooldown total unchanged on clamp"), Clamped.TotalSeconds, 3.0f);
	return true;
}

#endif
