#pragma once

#include "CoreMinimal.h"
#include "ArchonCombatTypes.generated.h"

UENUM(BlueprintType)
enum class EArchonHitType : uint8
{
	None UMETA(DisplayName = "None"),
	Body UMETA(DisplayName = "Body"),
	Head UMETA(DisplayName = "Head"),
	Limb UMETA(DisplayName = "Limb")
};

UENUM(BlueprintType)
enum class EArchonDamageType : uint8
{
	Generic UMETA(DisplayName = "Generic"),
	VerdantLivingArrow UMETA(DisplayName = "Verdant Living Arrow"),
	LenswrightPressureBolt UMETA(DisplayName = "Lenswright Pressure Bolt"),
	KinwildBeastBite UMETA(DisplayName = "Kinwild Beast Bite"),
	AlchemicalFire UMETA(DisplayName = "Alchemical Fire"),
	Environmental UMETA(DisplayName = "Environmental")
};

UENUM(BlueprintType)
enum class EArchonWeaponClass : uint8
{
	None UMETA(DisplayName = "None"),
	VerdantThornsproutBow UMETA(DisplayName = "Verdant Thornsprout Bow"),
	LenswrightPressureBoltCrossbow UMETA(DisplayName = "Lenswright Pressure-Bolt Crossbow")
};

USTRUCT(BlueprintType)
struct FArchonShotPayload
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Archon|Combat")
	int32 InstigatorTeamId = INDEX_NONE;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Archon|Combat")
	int32 InstigatorPlayerId = INDEX_NONE;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Archon|Combat")
	EArchonWeaponClass WeaponClass = EArchonWeaponClass::None;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Archon|Combat")
	EArchonDamageType DamageType = EArchonDamageType::Generic;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Archon|Combat")
	FVector ShotOrigin = FVector::ZeroVector;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Archon|Combat")
	FVector ShotDirection = FVector::ForwardVector;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Archon|Combat")
	FVector HitLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Archon|Combat")
	float DistanceTraveled = 0.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Archon|Combat")
	EArchonHitType HitType = EArchonHitType::Body;
};

USTRUCT(BlueprintType)
struct FArchonWeaponDamageProfile
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Archon|Combat")
	float BodyDamage = 35.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Archon|Combat")
	float HeadDamage = 80.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Archon|Combat")
	float LimbDamage = 22.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Archon|Combat")
	float FalloffStartUnits = 3000.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Archon|Combat")
	float FalloffEndUnits = 6000.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Archon|Combat")
	float MinDamage = 12.0f;
};

USTRUCT(BlueprintType)
struct FArchonHitResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Archon|Combat")
	bool bShouldHit = false;

	UPROPERTY(BlueprintReadOnly, Category = "Archon|Combat")
	float FinalDamage = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Archon|Combat")
	EArchonHitType HitType = EArchonHitType::None;

	UPROPERTY(BlueprintReadOnly, Category = "Archon|Combat")
	EArchonDamageType DamageType = EArchonDamageType::Generic;

	UPROPERTY(BlueprintReadOnly, Category = "Archon|Combat")
	int32 InstigatorTeamId = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "Archon|Combat")
	int32 InstigatorPlayerId = INDEX_NONE;

	// Where the shot came from (threat-response seam, 2026-06-11: the
	// first human playtest sniped a core from beyond every aggro radius
	// and nothing could react because the origin was dropped here).
	UPROPERTY(BlueprintReadOnly, Category = "Archon|Combat")
	FVector ShotOrigin = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Archon|Combat")
	FName Reason;
};

USTRUCT(BlueprintType)
struct FArchonDamageApplicationResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Archon|Combat")
	bool bAccepted = false;

	UPROPERTY(BlueprintReadOnly, Category = "Archon|Combat")
	float DamageApplied = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Archon|Combat")
	float PreviousHealth = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Archon|Combat")
	float NewHealth = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Archon|Combat")
	bool bCausedDeath = false;

	UPROPERTY(BlueprintReadOnly, Category = "Archon|Combat")
	FName Reason;
};
