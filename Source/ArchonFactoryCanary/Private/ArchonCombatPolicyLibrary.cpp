#include "ArchonCombatPolicyLibrary.h"

FArchonHitResult UArchonCombatPolicyLibrary::ResolveShot(
	const FArchonShotPayload& Shot,
	int32 TargetTeamId,
	bool bTargetIsAlive,
	float TargetArmorModifier,
	const FArchonWeaponDamageProfile& WeaponProfile)
{
	FArchonHitResult Result;
	Result.HitType = Shot.HitType;
	Result.DamageType = Shot.DamageType;
	Result.InstigatorTeamId = Shot.InstigatorTeamId;
	Result.InstigatorPlayerId = Shot.InstigatorPlayerId;
	Result.ShotOrigin = Shot.ShotOrigin;

	if (Shot.WeaponClass == EArchonWeaponClass::None)
	{
		Result.Reason = TEXT("no_weapon");
		return Result;
	}

	if (!bTargetIsAlive)
	{
		Result.Reason = TEXT("target_dead");
		return Result;
	}

	if (IsFriendlyFire(Shot.InstigatorTeamId, TargetTeamId))
	{
		Result.Reason = TEXT("friendly_fire_disabled");
		return Result;
	}

	if (Shot.HitType == EArchonHitType::None)
	{
		Result.Reason = TEXT("no_hit_type");
		return Result;
	}

	const float BaseDamage = GetBaseDamageForHitType(Shot.HitType, WeaponProfile);
	const float FalloffDamage = ComputeDistanceFalloff(BaseDamage, Shot.DistanceTraveled, WeaponProfile);
	Result.bShouldHit = true;
	Result.FinalDamage = FMath::Max(0.0f, FalloffDamage * TargetArmorModifier);
	Result.Reason = TEXT("accepted_combat_shot");
	return Result;
}

float UArchonCombatPolicyLibrary::ComputeDistanceFalloff(
	float BaseDamage,
	float DistanceUnits,
	const FArchonWeaponDamageProfile& WeaponProfile)
{
	if (DistanceUnits <= WeaponProfile.FalloffStartUnits)
	{
		return BaseDamage;
	}

	if (DistanceUnits >= WeaponProfile.FalloffEndUnits || WeaponProfile.FalloffEndUnits <= WeaponProfile.FalloffStartUnits)
	{
		return WeaponProfile.MinDamage;
	}

	const float Alpha = (DistanceUnits - WeaponProfile.FalloffStartUnits) / (WeaponProfile.FalloffEndUnits - WeaponProfile.FalloffStartUnits);
	return FMath::Lerp(BaseDamage, WeaponProfile.MinDamage, FMath::Clamp(Alpha, 0.0f, 1.0f));
}

float UArchonCombatPolicyLibrary::GetBaseDamageForHitType(
	EArchonHitType HitType,
	const FArchonWeaponDamageProfile& WeaponProfile)
{
	switch (HitType)
	{
	case EArchonHitType::Body:
		return WeaponProfile.BodyDamage;
	case EArchonHitType::Head:
		return WeaponProfile.HeadDamage;
	case EArchonHitType::Limb:
		return WeaponProfile.LimbDamage;
	case EArchonHitType::None:
	default:
		return 0.0f;
	}
}

bool UArchonCombatPolicyLibrary::IsFriendlyFire(int32 InstigatorTeamId, int32 TargetTeamId)
{
	return InstigatorTeamId != INDEX_NONE && InstigatorTeamId == TargetTeamId;
}

FArchonWeaponDamageProfile UArchonCombatPolicyLibrary::GetDefaultWeaponProfile(EArchonWeaponClass WeaponClass)
{
	FArchonWeaponDamageProfile Profile;

	switch (WeaponClass)
	{
	case EArchonWeaponClass::VerdantThornsproutBow:
		Profile.BodyDamage = 35.0f;
		Profile.HeadDamage = 80.0f;
		Profile.LimbDamage = 22.0f;
		Profile.FalloffStartUnits = 3000.0f;
		Profile.FalloffEndUnits = 6000.0f;
		Profile.MinDamage = 12.0f;
		break;

	case EArchonWeaponClass::LenswrightPressureBoltCrossbow:
		Profile.BodyDamage = 40.0f;
		Profile.HeadDamage = 90.0f;
		Profile.LimbDamage = 25.0f;
		Profile.FalloffStartUnits = 2500.0f;
		Profile.FalloffEndUnits = 5000.0f;
		Profile.MinDamage = 15.0f;
		break;

	case EArchonWeaponClass::None:
	default:
		Profile.BodyDamage = 0.0f;
		Profile.HeadDamage = 0.0f;
		Profile.LimbDamage = 0.0f;
		Profile.FalloffStartUnits = 0.0f;
		Profile.FalloffEndUnits = 0.0f;
		Profile.MinDamage = 0.0f;
		break;
	}

	return Profile;
}
