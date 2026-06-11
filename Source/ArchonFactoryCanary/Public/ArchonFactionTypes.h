#pragma once

#include "CoreMinimal.h"
#include "ArchonFactionTypes.generated.h"

UENUM(BlueprintType)
enum class EArchonFaction : uint8
{
	None UMETA(DisplayName = "None"),
	VerdantChoir UMETA(DisplayName = "Verdant Choir"),
	KinwildCovenant UMETA(DisplayName = "Kinwild Covenant"),
	LenswrightCompact UMETA(DisplayName = "Lenswright Compact")
};

UENUM(BlueprintType)
enum class EArchonFactionMovementVerb : uint8
{
	None UMETA(DisplayName = "None"),
	VerdantRootVault UMETA(DisplayName = "Verdant Root-Vault"),
	KinwildBoundLeap UMETA(DisplayName = "Kinwild Bound-Leap"),
	LenswrightPressureThrust UMETA(DisplayName = "Lenswright Pressure-Thrust")
};

USTRUCT(BlueprintType)
struct FArchonFactionMovementCooldown
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Archon|Locomotion")
	float SecondsRemaining = 0.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Archon|Locomotion")
	float TotalSeconds = 0.0f;
};

USTRUCT(BlueprintType)
struct FArchonFactionMovementInputState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Archon|Locomotion")
	bool bSprintHeld = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Archon|Locomotion")
	float SprintHeldSeconds = 0.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Archon|Locomotion")
	bool bJumpPressedThisFrame = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Archon|Locomotion")
	bool bGrounded = false;
};

USTRUCT(BlueprintType)
struct FArchonFactionMovementDecision
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Archon|Locomotion")
	bool bShouldLaunch = false;

	UPROPERTY(BlueprintReadOnly, Category = "Archon|Locomotion")
	float ForwardImpulse = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Archon|Locomotion")
	float VerticalImpulse = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Archon|Locomotion")
	FArchonFactionMovementCooldown NewCooldown;

	UPROPERTY(BlueprintReadOnly, Category = "Archon|Locomotion")
	EArchonFactionMovementVerb VerbTriggered = EArchonFactionMovementVerb::None;
};

USTRUCT(BlueprintType)
struct FArchonFactionMovementTuning
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Archon|Locomotion")
	float LaunchImpulseForward = 850.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Archon|Locomotion")
	float LaunchImpulseVertical = 450.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Archon|Locomotion")
	float CooldownSeconds = 3.0f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Archon|Locomotion")
	float MinSprintHeldSeconds = 0.15f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Archon|Locomotion")
	bool bRequireGroundedAtLaunch = true;
};
