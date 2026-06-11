#if WITH_DEV_AUTOMATION_TESTS

#include "ArchonCanaryFpsCharacter.h"
#include "ArchonRangedWeaponComponent.h"
#include "ArchonVerdantThornsproutBow.h"
#include "Misc/AutomationTest.h"

namespace
{
	FArchonWeaponStats MakeComponentWeaponStats()
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
	FArchonRangedWeaponConfigureSetsStateTest,
	"ArchonFactory.RangedWeapon.ConfigureSetsStatsAndState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonRangedWeaponConfigureSetsStateTest::RunTest(const FString& Parameters)
{
	UArchonRangedWeaponComponent* Weapon = NewObject<UArchonRangedWeaponComponent>();
	Weapon->ConfigureWeapon(4, 11, MakeComponentWeaponStats());

	TestEqual(TEXT("Configured class is stored"), Weapon->GetStats().WeaponClass, EArchonWeaponClass::VerdantThornsproutBow);
	TestEqual(TEXT("Initial ammo uses quiver capacity"), Weapon->GetState().CurrentAmmo, 3);
	TestEqual(TEXT("Initial state is ready"), Weapon->GetState().FireState, EArchonWeaponFireState::Ready);
	TestTrue(TEXT("Weapon reports ready"), Weapon->IsReady());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonRangedWeaponTryFireSpendsAmmoTest,
	"ArchonFactory.RangedWeapon.TryFireSpendsAmmoAndBroadcasts",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonRangedWeaponTryFireSpendsAmmoTest::RunTest(const FString& Parameters)
{
	UArchonRangedWeaponComponent* Weapon = NewObject<UArchonRangedWeaponComponent>();
	Weapon->ConfigureWeapon(2, 9, MakeComponentWeaponStats());

	int32 FiredEvents = 0;
	FArchonShotPayload LastShot;
	Weapon->OnWeaponFired.AddLambda([&FiredEvents, &LastShot](const FArchonShotPayload& Shot)
	{
		++FiredEvents;
		LastShot = Shot;
	});

	const bool bFired = Weapon->TryFire(FVector::ZeroVector, FVector::ForwardVector);

	TestTrue(TEXT("TryFire should accept a ready weapon"), bFired);
	TestEqual(TEXT("One shot fired"), Weapon->GetShotsFired(), 1);
	TestEqual(TEXT("Ammo decremented"), Weapon->GetState().CurrentAmmo, 2);
	TestEqual(TEXT("Weapon is on cycle"), Weapon->GetState().FireState, EArchonWeaponFireState::OnFireCycle);
	TestEqual(TEXT("Fired delegate broadcast once"), FiredEvents, 1);
	TestEqual(TEXT("Shot carries owner team"), LastShot.InstigatorTeamId, 2);
	TestEqual(TEXT("Shot carries owner player"), LastShot.InstigatorPlayerId, 9);
	TestEqual(TEXT("Shot carries weapon class"), LastShot.WeaponClass, EArchonWeaponClass::VerdantThornsproutBow);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonRangedWeaponTryFireBlocksOnCycleTest,
	"ArchonFactory.RangedWeapon.TryFireBlocksOnCycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonRangedWeaponTryFireBlocksOnCycleTest::RunTest(const FString& Parameters)
{
	UArchonRangedWeaponComponent* Weapon = NewObject<UArchonRangedWeaponComponent>();
	Weapon->ConfigureWeapon(2, 9, MakeComponentWeaponStats());

	TestTrue(TEXT("First fire should succeed"), Weapon->TryFire(FVector::ZeroVector, FVector::ForwardVector));
	TestFalse(TEXT("Second fire during cycle should fail"), Weapon->TryFire(FVector::ZeroVector, FVector::ForwardVector));
	TestEqual(TEXT("Only one shot counted"), Weapon->GetShotsFired(), 1);
	TestEqual(TEXT("Ammo only decremented once"), Weapon->GetState().CurrentAmmo, 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonRangedWeaponTickReturnsReadyTest,
	"ArchonFactory.RangedWeapon.TickReturnsToReadyAfterFireCycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonRangedWeaponTickReturnsReadyTest::RunTest(const FString& Parameters)
{
	UArchonRangedWeaponComponent* Weapon = NewObject<UArchonRangedWeaponComponent>();
	Weapon->ConfigureWeapon(2, 9, MakeComponentWeaponStats());
	Weapon->TryFire(FVector::ZeroVector, FVector::ForwardVector);

	Weapon->TickComponent(1.21f, LEVELTICK_All, nullptr);

	TestEqual(TEXT("Weapon returns ready after fire cycle"), Weapon->GetState().FireState, EArchonWeaponFireState::Ready);
	TestTrue(TEXT("Weapon reports ready"), Weapon->IsReady());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonRangedWeaponReloadCompletesTest,
	"ArchonFactory.RangedWeapon.TryReloadStartsAndCompletes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonRangedWeaponReloadCompletesTest::RunTest(const FString& Parameters)
{
	UArchonRangedWeaponComponent* Weapon = NewObject<UArchonRangedWeaponComponent>();
	Weapon->ConfigureWeapon(2, 9, MakeComponentWeaponStats());
	Weapon->TryFire(FVector::ZeroVector, FVector::ForwardVector);
	Weapon->TickComponent(1.21f, LEVELTICK_All, nullptr);

	int32 ReloadStartedEvents = 0;
	int32 ReloadCompletedEvents = 0;
	Weapon->OnReloadStarted.AddLambda([&ReloadStartedEvents]()
	{
		++ReloadStartedEvents;
	});
	Weapon->OnReloadCompleted.AddLambda([&ReloadCompletedEvents]()
	{
		++ReloadCompletedEvents;
	});

	TestTrue(TEXT("Partial quiver accepts reload"), Weapon->TryReload());
	TestEqual(TEXT("Reload started delegate broadcast once"), ReloadStartedEvents, 1);
	TestEqual(TEXT("Weapon is reloading"), Weapon->GetState().FireState, EArchonWeaponFireState::Reloading);

	Weapon->TickComponent(1.81f, LEVELTICK_All, nullptr);

	TestEqual(TEXT("Reload completes to ready"), Weapon->GetState().FireState, EArchonWeaponFireState::Ready);
	TestEqual(TEXT("Reload refills ammo"), Weapon->GetState().CurrentAmmo, 3);
	TestEqual(TEXT("Reload count increments"), Weapon->GetReloadCount(), 1);
	TestEqual(TEXT("Reload completed delegate broadcast once"), ReloadCompletedEvents, 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonCanaryFpsCharacterOwnsVerdantBowTest,
	"ArchonFactory.RangedWeapon.CanaryFpsCharacterOwnsVerdantBow",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonCanaryFpsCharacterOwnsVerdantBowTest::RunTest(const FString& Parameters)
{
	AArchonCanaryFpsCharacter* Character = NewObject<AArchonCanaryFpsCharacter>();

	TestNotNull(TEXT("Canary FPS character owns the Verdant bow component"), Character->GetVerdantBow());
	TestEqual(
		TEXT("Character bow uses Verdant weapon class"),
		Character->GetVerdantBow()->GetStats().WeaponClass,
		EArchonWeaponClass::VerdantThornsproutBow);
	TestEqual(TEXT("Character bow starts with a full quiver"), Character->GetVerdantBow()->GetState().CurrentAmmo, 999);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonVerdantBowDefaultsMatchContractTest,
	"ArchonFactory.RangedWeapon.VerdantBowDefaultsMatchContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonVerdantBowDefaultsMatchContractTest::RunTest(const FString& Parameters)
{
	UArchonVerdantThornsproutBow* Bow = NewObject<UArchonVerdantThornsproutBow>();
	const FArchonWeaponStats Stats = Bow->GetStats();

	TestEqual(TEXT("Verdant bow weapon class"), Stats.WeaponClass, EArchonWeaponClass::VerdantThornsproutBow);
	// 999 = effectively unlimited, Jonathan-ratified 2026-06-10.
	TestEqual(TEXT("Verdant bow quiver capacity"), Stats.QuiverCapacity, 999);
	TestEqual(TEXT("Verdant bow fire cycle"), Stats.FireCycleSeconds, 1.2f);
	TestEqual(TEXT("Verdant bow reload time"), Stats.ReloadSeconds, 1.8f);
	// 5600 = -30% flight speed, Jonathan-ratified 2026-06-10 (visibility).
	TestEqual(TEXT("Verdant bow projectile speed"), Stats.ProjectileSpeed, 5600.0f);
	TestEqual(TEXT("Verdant bow max range"), Stats.MaxRangeUnits, 8000.0f);
	TestEqual(TEXT("Verdant bow starts full"), Bow->GetState().CurrentAmmo, 999);
	return true;
}

#endif
