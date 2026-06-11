#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ArchonCombatTypes.h"
#include "ArchonCombatPolicyLibrary.generated.h"

UCLASS()
class ARCHONFACTORYCANARY_API UArchonCombatPolicyLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Archon|Combat")
	static FArchonHitResult ResolveShot(
		const FArchonShotPayload& Shot,
		int32 TargetTeamId,
		bool bTargetIsAlive,
		float TargetArmorModifier,
		const FArchonWeaponDamageProfile& WeaponProfile);

	UFUNCTION(BlueprintPure, Category = "Archon|Combat")
	static float ComputeDistanceFalloff(
		float BaseDamage,
		float DistanceUnits,
		const FArchonWeaponDamageProfile& WeaponProfile);

	UFUNCTION(BlueprintPure, Category = "Archon|Combat")
	static float GetBaseDamageForHitType(
		EArchonHitType HitType,
		const FArchonWeaponDamageProfile& WeaponProfile);

	UFUNCTION(BlueprintPure, Category = "Archon|Combat")
	static bool IsFriendlyFire(int32 InstigatorTeamId, int32 TargetTeamId);

	UFUNCTION(BlueprintPure, Category = "Archon|Combat")
	static FArchonWeaponDamageProfile GetDefaultWeaponProfile(EArchonWeaponClass WeaponClass);
};
