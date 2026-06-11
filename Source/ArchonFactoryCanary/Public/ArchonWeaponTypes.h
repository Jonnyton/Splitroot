#pragma once

#include "CoreMinimal.h"
#include "ArchonCombatTypes.h"
#include "ArchonWeaponTypes.generated.h"

UENUM(BlueprintType)
enum class EArchonWeaponFireState : uint8
{
	Ready UMETA(DisplayName = "Ready"),
	OnFireCycle UMETA(DisplayName = "On Fire Cycle"),
	Reloading UMETA(DisplayName = "Reloading"),
	Empty UMETA(DisplayName = "Empty")
};

USTRUCT(BlueprintType)
struct FArchonWeaponStats
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Archon|Weapon")
	EArchonWeaponClass WeaponClass = EArchonWeaponClass::None;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Archon|Weapon")
	int32 QuiverCapacity = 3;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Archon|Weapon")
	float FireCycleSeconds = 1.2f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Archon|Weapon")
	float ReloadSeconds = 1.8f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Archon|Weapon")
	float ProjectileSpeed = 8000.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Archon|Weapon")
	float MaxRangeUnits = 8000.0f;
};

USTRUCT(BlueprintType)
struct FArchonWeaponState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Archon|Weapon")
	EArchonWeaponFireState FireState = EArchonWeaponFireState::Ready;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Archon|Weapon")
	int32 CurrentAmmo = 3;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Archon|Weapon")
	float SecondsUntilReady = 0.0f;
};

USTRUCT(BlueprintType)
struct FArchonWeaponFireDecision
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Archon|Weapon")
	bool bShouldFire = false;

	UPROPERTY(BlueprintReadOnly, Category = "Archon|Weapon")
	FArchonWeaponState NewState;

	UPROPERTY(BlueprintReadOnly, Category = "Archon|Weapon")
	FName Reason;
};

USTRUCT(BlueprintType)
struct FArchonWeaponReloadDecision
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Archon|Weapon")
	bool bShouldReload = false;

	UPROPERTY(BlueprintReadOnly, Category = "Archon|Weapon")
	FArchonWeaponState NewState;

	UPROPERTY(BlueprintReadOnly, Category = "Archon|Weapon")
	FName Reason;
};
