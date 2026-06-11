#if WITH_DEV_AUTOMATION_TESTS

#include "ArchonCombatPolicyLibrary.h"
#include "Misc/AutomationTest.h"

namespace
{
	FArchonShotPayload MakeShot(EArchonWeaponClass WeaponClass, EArchonHitType HitType, float Distance = 0.0f)
	{
		FArchonShotPayload Shot;
		Shot.InstigatorTeamId = 0;
		Shot.InstigatorPlayerId = 7;
		Shot.WeaponClass = WeaponClass;
		Shot.DamageType = EArchonDamageType::VerdantLivingArrow;
		Shot.DistanceTraveled = Distance;
		Shot.HitType = HitType;
		return Shot;
	}

	FArchonWeaponDamageProfile DefaultProfile()
	{
		return UArchonCombatPolicyLibrary::GetDefaultWeaponProfile(EArchonWeaponClass::VerdantThornsproutBow);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonCombatResolveShotAcceptedOnBodyTest,
	"ArchonFactory.Combat.ResolveShotAcceptedOnBody",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonCombatResolveShotAcceptedOnBodyTest::RunTest(const FString& Parameters)
{
	const FArchonHitResult Result = UArchonCombatPolicyLibrary::ResolveShot(MakeShot(EArchonWeaponClass::VerdantThornsproutBow, EArchonHitType::Body), 1, true, 1.0f, DefaultProfile());

	TestTrue(TEXT("Valid enemy body shot should hit"), Result.bShouldHit);
	TestEqual(TEXT("Body shot uses Verdant body damage"), Result.FinalDamage, 35.0f);
	TestEqual(TEXT("Accepted reason is stable"), Result.Reason, FName(TEXT("accepted_combat_shot")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonCombatResolveShotHeadUsesHeadDamageTest,
	"ArchonFactory.Combat.ResolveShotHeadUsesHeadDamage",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonCombatResolveShotHeadUsesHeadDamageTest::RunTest(const FString& Parameters)
{
	const FArchonHitResult Result = UArchonCombatPolicyLibrary::ResolveShot(MakeShot(EArchonWeaponClass::VerdantThornsproutBow, EArchonHitType::Head), 1, true, 1.0f, DefaultProfile());
	TestEqual(TEXT("Head shot uses Verdant head damage"), Result.FinalDamage, 80.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonCombatResolveShotLimbUsesLimbDamageTest,
	"ArchonFactory.Combat.ResolveShotLimbUsesLimbDamage",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonCombatResolveShotLimbUsesLimbDamageTest::RunTest(const FString& Parameters)
{
	const FArchonHitResult Result = UArchonCombatPolicyLibrary::ResolveShot(MakeShot(EArchonWeaponClass::VerdantThornsproutBow, EArchonHitType::Limb), 1, true, 1.0f, DefaultProfile());
	TestEqual(TEXT("Limb shot uses Verdant limb damage"), Result.FinalDamage, 22.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonCombatRejectsWeaponNoneTest,
	"ArchonFactory.Combat.ResolveShotRejectsWeaponNone",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonCombatRejectsWeaponNoneTest::RunTest(const FString& Parameters)
{
	const FArchonHitResult Result = UArchonCombatPolicyLibrary::ResolveShot(MakeShot(EArchonWeaponClass::None, EArchonHitType::Body), 1, true, 1.0f, DefaultProfile());
	TestFalse(TEXT("No weapon should not hit"), Result.bShouldHit);
	TestEqual(TEXT("No weapon reason is stable"), Result.Reason, FName(TEXT("no_weapon")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonCombatRejectsDeadTargetTest,
	"ArchonFactory.Combat.ResolveShotRejectsDeadTarget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonCombatRejectsDeadTargetTest::RunTest(const FString& Parameters)
{
	const FArchonHitResult Result = UArchonCombatPolicyLibrary::ResolveShot(MakeShot(EArchonWeaponClass::VerdantThornsproutBow, EArchonHitType::Body), 1, false, 1.0f, DefaultProfile());
	TestFalse(TEXT("Dead target should not take shot damage"), Result.bShouldHit);
	TestEqual(TEXT("Dead target reason is stable"), Result.Reason, FName(TEXT("target_dead")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonCombatRejectsFriendlyFireTest,
	"ArchonFactory.Combat.ResolveShotRejectsFriendlyFire",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonCombatRejectsFriendlyFireTest::RunTest(const FString& Parameters)
{
	const FArchonHitResult Result = UArchonCombatPolicyLibrary::ResolveShot(MakeShot(EArchonWeaponClass::VerdantThornsproutBow, EArchonHitType::Body), 0, true, 1.0f, DefaultProfile());
	TestFalse(TEXT("Friendly fire is off at v0"), Result.bShouldHit);
	TestEqual(TEXT("Friendly fire reason is stable"), Result.Reason, FName(TEXT("friendly_fire_disabled")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonCombatRejectsHitTypeNoneTest,
	"ArchonFactory.Combat.ResolveShotRejectsHitTypeNone",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonCombatRejectsHitTypeNoneTest::RunTest(const FString& Parameters)
{
	const FArchonHitResult Result = UArchonCombatPolicyLibrary::ResolveShot(MakeShot(EArchonWeaponClass::VerdantThornsproutBow, EArchonHitType::None), 1, true, 1.0f, DefaultProfile());
	TestFalse(TEXT("No hit type should not hit"), Result.bShouldHit);
	TestEqual(TEXT("No hit type reason is stable"), Result.Reason, FName(TEXT("no_hit_type")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonCombatFalloffFullDamageBeforeStartTest,
	"ArchonFactory.Combat.FalloffKeepsFullDamageBeforeStart",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonCombatFalloffFullDamageBeforeStartTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("Damage before falloff start remains full"), UArchonCombatPolicyLibrary::ComputeDistanceFalloff(35.0f, 2500.0f, DefaultProfile()), 35.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonCombatFalloffInterpolatesBetweenStartAndEndTest,
	"ArchonFactory.Combat.FalloffInterpolatesBetweenStartAndEnd",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonCombatFalloffInterpolatesBetweenStartAndEndTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("Halfway through Verdant falloff linearly interpolates"), UArchonCombatPolicyLibrary::ComputeDistanceFalloff(35.0f, 4500.0f, DefaultProfile()), 23.5f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonCombatFalloffUsesMinDamageAfterEndTest,
	"ArchonFactory.Combat.FalloffUsesMinDamageAfterEnd",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonCombatFalloffUsesMinDamageAfterEndTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("Damage after falloff end clamps to min damage"), UArchonCombatPolicyLibrary::ComputeDistanceFalloff(35.0f, 8000.0f, DefaultProfile()), 12.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonCombatDefaultWeaponProfilesUseSliceValuesTest,
	"ArchonFactory.Combat.DefaultWeaponProfilesUseSliceValues",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonCombatDefaultWeaponProfilesUseSliceValuesTest::RunTest(const FString& Parameters)
{
	const FArchonWeaponDamageProfile Verdant = UArchonCombatPolicyLibrary::GetDefaultWeaponProfile(EArchonWeaponClass::VerdantThornsproutBow);
	const FArchonWeaponDamageProfile Lenswright = UArchonCombatPolicyLibrary::GetDefaultWeaponProfile(EArchonWeaponClass::LenswrightPressureBoltCrossbow);

	TestEqual(TEXT("Verdant body damage"), Verdant.BodyDamage, 35.0f);
	TestEqual(TEXT("Verdant falloff end"), Verdant.FalloffEndUnits, 6000.0f);
	TestEqual(TEXT("Lenswright body damage"), Lenswright.BodyDamage, 40.0f);
	TestEqual(TEXT("Lenswright falloff end"), Lenswright.FalloffEndUnits, 5000.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonCombatArmorModifierScalesFinalDamageTest,
	"ArchonFactory.Combat.ArmorModifierScalesFinalDamage",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonCombatArmorModifierScalesFinalDamageTest::RunTest(const FString& Parameters)
{
	const FArchonHitResult Result = UArchonCombatPolicyLibrary::ResolveShot(MakeShot(EArchonWeaponClass::VerdantThornsproutBow, EArchonHitType::Body), 1, true, 0.5f, DefaultProfile());
	TestEqual(TEXT("Armor modifier scales final damage"), Result.FinalDamage, 17.5f);
	return true;
}

#endif
