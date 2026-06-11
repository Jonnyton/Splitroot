#if WITH_DEV_AUTOMATION_TESTS

#include "ArchonAiCombatBehaviorComponent.h"
#include "ArchonArrowProjectile.h"
#include "ArchonCombatHealthComponent.h"
#include "ArchonCombatPolicyLibrary.h"
#include "ArchonLenswrightBracewrightActor.h"
#include "ArchonLenswrightPressureBoltCrossbow.h"
#include "ArchonLenswrightSundialOpticActor.h"
#include "ArchonPressureBoltProjectile.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonLenswrightPressureBoltCrossbowDefaultsMatchContractTest,
	"ArchonFactory.Lenswright.PressureBoltCrossbowDefaultsMatchContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonLenswrightPressureBoltCrossbowDefaultsMatchContractTest::RunTest(const FString& Parameters)
{
	UArchonLenswrightPressureBoltCrossbow* Crossbow = NewObject<UArchonLenswrightPressureBoltCrossbow>();
	const FArchonWeaponStats Stats = Crossbow->GetStats();

	TestEqual(TEXT("Lenswright crossbow weapon class"), Stats.WeaponClass, EArchonWeaponClass::LenswrightPressureBoltCrossbow);
	TestEqual(TEXT("Lenswright crossbow single pressure bolt"), Stats.QuiverCapacity, 1);
	TestEqual(TEXT("Lenswright crossbow slow fire cycle"), Stats.FireCycleSeconds, 1.5f);
	TestEqual(TEXT("Lenswright crossbow crank reload"), Stats.ReloadSeconds, 2.2f);
	TestEqual(TEXT("Lenswright pressure bolt speed"), Stats.ProjectileSpeed, 6000.0f);
	TestEqual(TEXT("Lenswright pressure bolt range"), Stats.MaxRangeUnits, 6000.0f);
	TestEqual(TEXT("Lenswright crossbow starts loaded"), Crossbow->GetState().CurrentAmmo, 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonLenswrightPressureBoltProjectileSpeedUnderHitscanTest,
	"ArchonFactory.Lenswright.PressureBoltProjectileSpeedUnderHitscan",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonLenswrightPressureBoltProjectileSpeedUnderHitscanTest::RunTest(const FString& Parameters)
{
	UArchonLenswrightPressureBoltCrossbow* Crossbow = NewObject<UArchonLenswrightPressureBoltCrossbow>();

	TestTrue(
		TEXT("Pressure-bolt speed remains under the no-gunpowder hitscan-feel ceiling"),
		Crossbow->GetStats().ProjectileSpeed <= 7000.0f);
	TestEqual(
		TEXT("Pressure-bolt damage type is contract-named"),
		UArchonCombatPolicyLibrary::GetDefaultWeaponProfile(EArchonWeaponClass::LenswrightPressureBoltCrossbow).BodyDamage,
		40.0f);
	TestEqual(
		TEXT("Projectile subclass exists for visual/audio presentation separation"),
		AArchonPressureBoltProjectile::StaticClass()->GetSuperClass(),
		AArchonArrowProjectile::StaticClass());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonLenswrightBracewrightConfiguresCombatAndAiTest,
	"ArchonFactory.Lenswright.BracewrightConfiguresCombatAndAi",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonLenswrightBracewrightConfiguresCombatAndAiTest::RunTest(const FString& Parameters)
{
	AArchonLenswrightBracewrightActor* Bracewright = NewObject<AArchonLenswrightBracewrightActor>();
	Bracewright->ConfigureBracewright(3, TEXT("bracewright_test"));

	TestNotNull(TEXT("Bracewright owns health component"), Bracewright->GetHealth());
	TestNotNull(TEXT("Bracewright owns pressure-bolt crossbow"), Bracewright->GetCrossbow());
	TestNotNull(TEXT("Bracewright owns AI combat behavior"), Bracewright->GetCombatBehavior());
	TestEqual(TEXT("Bracewright health uses configured team"), Bracewright->GetHealth()->GetTeamId(), 3);
	TestEqual(TEXT("Bracewright max health follows contract"), Bracewright->GetHealth()->GetMaxHealth(), 80.0f);
	TestEqual(TEXT("Bracewright crossbow uses AI owner id"), Bracewright->GetCrossbow()->GetOwnerPlayerId(), INDEX_NONE);
	TestEqual(TEXT("Bracewright behavior uses defensive ranged role"), Bracewright->GetCombatBehavior()->GetRole(), EArchonAiCombatRole::DefensiveRanged);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonLenswrightSundialOpticHasVisionAndNoWeaponTest,
	"ArchonFactory.Lenswright.SundialOpticHasVisionAndNoWeapon",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonLenswrightSundialOpticHasVisionAndNoWeaponTest::RunTest(const FString& Parameters)
{
	AArchonLenswrightSundialOpticActor* Sundial = NewObject<AArchonLenswrightSundialOpticActor>();
	Sundial->ConfigureSundial(3, TEXT("sundial_test"));

	TestNotNull(TEXT("Sundial owns health component"), Sundial->GetHealth());
	TestNotNull(TEXT("Sundial owns AI combat behavior"), Sundial->GetCombatBehavior());
	TestEqual(TEXT("Sundial health uses configured team"), Sundial->GetHealth()->GetTeamId(), 3);
	TestEqual(TEXT("Sundial max health follows scout contract"), Sundial->GetHealth()->GetMaxHealth(), 60.0f);
	TestEqual(TEXT("Sundial vision radius follows contract"), Sundial->GetVisionRadius(), 4500.0f);
	TestEqual(TEXT("Sundial behavior uses scout role"), Sundial->GetCombatBehavior()->GetRole(), EArchonAiCombatRole::ScoutNoWeapon);
	TestNull(TEXT("Sundial has no ranged weapon component at v0"), Sundial->FindComponentByClass<UArchonLenswrightPressureBoltCrossbow>());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonLenswrightBracewrightCanFirePressureBoltAtEnemyTest,
	"ArchonFactory.Lenswright.BracewrightCanFirePressureBoltAtEnemy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonLenswrightBracewrightCanFirePressureBoltAtEnemyTest::RunTest(const FString& Parameters)
{
	AArchonLenswrightBracewrightActor* Bracewright = NewObject<AArchonLenswrightBracewrightActor>();
	Bracewright->ConfigureBracewright(1, TEXT("bracewright_test"));

	int32 FiredEvents = 0;
	FArchonShotPayload Shot;
	Bracewright->GetCrossbow()->OnWeaponFired.AddLambda([&FiredEvents, &Shot](const FArchonShotPayload& InShot)
	{
		++FiredEvents;
		Shot = InShot;
	});

	const bool bFired = Bracewright->GetCrossbow()->TryFire(FVector::ZeroVector, FVector::ForwardVector);

	TestTrue(TEXT("Configured Bracewright crossbow fires when ready"), bFired);
	TestEqual(TEXT("Bracewright fires one pressure bolt"), FiredEvents, 1);
	TestEqual(TEXT("Shot team is Lenswright team"), Shot.InstigatorTeamId, 1);
	TestEqual(TEXT("Shot is Lenswright pressure-bolt class"), Shot.WeaponClass, EArchonWeaponClass::LenswrightPressureBoltCrossbow);
	TestEqual(TEXT("Shot carries LenswrightPressureBolt damage type"), Shot.DamageType, EArchonDamageType::LenswrightPressureBolt);
	TestEqual(TEXT("Single-bolt crossbow empties after fire"), Bracewright->GetCrossbow()->GetState().CurrentAmmo, 0);
	return true;
}

#endif
