#if WITH_DEV_AUTOMATION_TESTS

#include "ArchonWeaponPolicyLibrary.h"
#include "Misc/AutomationTest.h"

namespace
{
	FArchonWeaponStats MakeVerdantWeaponStats()
	{
		FArchonWeaponStats Stats;
		Stats.WeaponClass = EArchonWeaponClass::VerdantThornsproutBow;
		Stats.QuiverCapacity = 3;
		Stats.FireCycleSeconds = 1.2f;
		Stats.ReloadSeconds = 1.8f;
		Stats.ProjectileSpeed = 8000.0f;
		Stats.MaxRangeUnits = 8000.0f;
		return Stats;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonWeaponFireReadyAcceptedDecrementsAmmoTest,
	"ArchonFactory.Weapon.FireReadyAcceptedDecrementsAmmo",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonWeaponFireReadyAcceptedDecrementsAmmoTest::RunTest(const FString& Parameters)
{
	const FArchonWeaponStats Stats = MakeVerdantWeaponStats();
	const FArchonWeaponState State = UArchonWeaponPolicyLibrary::MakeInitialState(Stats);
	const FArchonWeaponFireDecision Decision = UArchonWeaponPolicyLibrary::EvaluateFire(State, Stats, true);

	TestTrue(TEXT("Ready weapon should accept fire input"), Decision.bShouldFire);
	TestEqual(TEXT("Ammo decrements once"), Decision.NewState.CurrentAmmo, 2);
	TestEqual(TEXT("Fire state enters cycle"), Decision.NewState.FireState, EArchonWeaponFireState::OnFireCycle);
	TestEqual(TEXT("Cycle timer uses stats"), Decision.NewState.SecondsUntilReady, 1.2f);
	TestEqual(TEXT("Reason is stable"), Decision.Reason, FName(TEXT("accepted_fire")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonWeaponFireRejectsNotPressedTest,
	"ArchonFactory.Weapon.FireRejectsWhenNotPressed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonWeaponFireRejectsNotPressedTest::RunTest(const FString& Parameters)
{
	const FArchonWeaponStats Stats = MakeVerdantWeaponStats();
	const FArchonWeaponFireDecision Decision = UArchonWeaponPolicyLibrary::EvaluateFire(
		UArchonWeaponPolicyLibrary::MakeInitialState(Stats),
		Stats,
		false);

	TestFalse(TEXT("No input should not fire"), Decision.bShouldFire);
	TestEqual(TEXT("Reason is stable"), Decision.Reason, FName(TEXT("not_pressed")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonWeaponFireRejectsOnCycleTest,
	"ArchonFactory.Weapon.FireRejectsOnCycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonWeaponFireRejectsOnCycleTest::RunTest(const FString& Parameters)
{
	const FArchonWeaponStats Stats = MakeVerdantWeaponStats();
	FArchonWeaponState State = UArchonWeaponPolicyLibrary::MakeInitialState(Stats);
	State.FireState = EArchonWeaponFireState::OnFireCycle;
	State.SecondsUntilReady = 0.4f;

	const FArchonWeaponFireDecision Decision = UArchonWeaponPolicyLibrary::EvaluateFire(State, Stats, true);

	TestFalse(TEXT("Weapon on fire cycle should not fire"), Decision.bShouldFire);
	TestEqual(TEXT("Reason is stable"), Decision.Reason, FName(TEXT("on_cycle")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonWeaponFireRejectsReloadingTest,
	"ArchonFactory.Weapon.FireRejectsReloading",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonWeaponFireRejectsReloadingTest::RunTest(const FString& Parameters)
{
	const FArchonWeaponStats Stats = MakeVerdantWeaponStats();
	FArchonWeaponState State = UArchonWeaponPolicyLibrary::MakeInitialState(Stats);
	State.FireState = EArchonWeaponFireState::Reloading;
	State.SecondsUntilReady = 0.8f;

	const FArchonWeaponFireDecision Decision = UArchonWeaponPolicyLibrary::EvaluateFire(State, Stats, true);

	TestFalse(TEXT("Reloading weapon should not fire"), Decision.bShouldFire);
	TestEqual(TEXT("Reason is stable"), Decision.Reason, FName(TEXT("reloading")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonWeaponFireRejectsEmptyTest,
	"ArchonFactory.Weapon.FireRejectsEmpty",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonWeaponFireRejectsEmptyTest::RunTest(const FString& Parameters)
{
	const FArchonWeaponStats Stats = MakeVerdantWeaponStats();
	FArchonWeaponState State;
	State.FireState = EArchonWeaponFireState::Empty;
	State.CurrentAmmo = 0;

	const FArchonWeaponFireDecision Decision = UArchonWeaponPolicyLibrary::EvaluateFire(State, Stats, true);

	TestFalse(TEXT("Empty weapon should not fire"), Decision.bShouldFire);
	TestEqual(TEXT("Reason is stable"), Decision.Reason, FName(TEXT("empty")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonWeaponReloadStartAcceptedTest,
	"ArchonFactory.Weapon.ReloadStartAcceptedFromPartialAmmo",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonWeaponReloadStartAcceptedTest::RunTest(const FString& Parameters)
{
	const FArchonWeaponStats Stats = MakeVerdantWeaponStats();
	FArchonWeaponState State = UArchonWeaponPolicyLibrary::MakeInitialState(Stats);
	State.CurrentAmmo = 1;

	const FArchonWeaponReloadDecision Decision = UArchonWeaponPolicyLibrary::EvaluateReloadStart(State, Stats, true);

	TestTrue(TEXT("Partial ammo should accept reload"), Decision.bShouldReload);
	TestEqual(TEXT("Reload state is active"), Decision.NewState.FireState, EArchonWeaponFireState::Reloading);
	TestEqual(TEXT("Reload timer uses stats"), Decision.NewState.SecondsUntilReady, 1.8f);
	TestEqual(TEXT("Reason is stable"), Decision.Reason, FName(TEXT("accepted_reload")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonWeaponAdvanceFireCycleTest,
	"ArchonFactory.Weapon.AdvanceFireCycleReturnsReadyOrAutoReload",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonWeaponAdvanceFireCycleTest::RunTest(const FString& Parameters)
{
	const FArchonWeaponStats Stats = MakeVerdantWeaponStats();
	FArchonWeaponState WithAmmo;
	WithAmmo.FireState = EArchonWeaponFireState::OnFireCycle;
	WithAmmo.CurrentAmmo = 1;
	WithAmmo.SecondsUntilReady = 0.2f;

	const FArchonWeaponState Ready = UArchonWeaponPolicyLibrary::AdvanceWeaponState(WithAmmo, Stats, 0.25f, false);
	TestEqual(TEXT("Cycle with remaining ammo returns ready"), Ready.FireState, EArchonWeaponFireState::Ready);
	TestEqual(TEXT("Cycle timer clamps to zero"), Ready.SecondsUntilReady, 0.0f);

	FArchonWeaponState EmptyAfterShot;
	EmptyAfterShot.FireState = EArchonWeaponFireState::OnFireCycle;
	EmptyAfterShot.CurrentAmmo = 0;
	EmptyAfterShot.SecondsUntilReady = 0.2f;

	const FArchonWeaponState AutoReload = UArchonWeaponPolicyLibrary::AdvanceWeaponState(EmptyAfterShot, Stats, 0.25f, true);
	TestEqual(TEXT("Empty cycle can auto reload"), AutoReload.FireState, EArchonWeaponFireState::Reloading);
	TestEqual(TEXT("Auto reload timer uses stats"), AutoReload.SecondsUntilReady, 1.8f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonWeaponAdvanceReloadCompletesTest,
	"ArchonFactory.Weapon.AdvanceReloadCompletesToFullAmmo",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonWeaponAdvanceReloadCompletesTest::RunTest(const FString& Parameters)
{
	const FArchonWeaponStats Stats = MakeVerdantWeaponStats();
	FArchonWeaponState State;
	State.FireState = EArchonWeaponFireState::Reloading;
	State.CurrentAmmo = 0;
	State.SecondsUntilReady = 0.2f;

	const FArchonWeaponState Reloaded = UArchonWeaponPolicyLibrary::AdvanceWeaponState(State, Stats, 0.25f, true);

	TestEqual(TEXT("Reload completes to ready"), Reloaded.FireState, EArchonWeaponFireState::Ready);
	TestEqual(TEXT("Reload refills quiver"), Reloaded.CurrentAmmo, 3);
	TestEqual(TEXT("Reload timer clamps to zero"), Reloaded.SecondsUntilReady, 0.0f);
	return true;
}

#endif
