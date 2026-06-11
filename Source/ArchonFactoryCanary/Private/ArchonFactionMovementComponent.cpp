#include "ArchonFactionMovementComponent.h"
#include "ArchonFactionMovementPolicyLibrary.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

UArchonFactionMovementComponent::UArchonFactionMovementComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UArchonFactionMovementComponent::BeginPlay()
{
	Super::BeginPlay();
	Cooldown = FArchonFactionMovementCooldown();
	LaunchCount = 0;
	CooldownBlockedCount = 0;
	bSprintHeld = false;
	SprintHeldSeconds = 0.0f;
	bJumpPressedThisFrame = false;
	bWillVaultThisFrame = false;
}

void UArchonFactionMovementComponent::ConfigureFaction(EArchonFaction InFaction)
{
	Faction = InFaction;
}

void UArchonFactionMovementComponent::NotifySprintHeld(bool bHeld)
{
	if (bHeld && !bSprintHeld)
	{
		SprintHeldSeconds = 0.0f;
	}
	bSprintHeld = bHeld;
	if (!bHeld)
	{
		SprintHeldSeconds = 0.0f;
		bWillVaultThisFrame = false;
	}
}

void UArchonFactionMovementComponent::NotifyJumpPressed()
{
	bJumpPressedThisFrame = true;
	bWillVaultThisFrame = ShouldVaultOnPendingInput();
}

bool UArchonFactionMovementComponent::IsCooldownReady() const
{
	return UArchonFactionMovementPolicyLibrary::IsCooldownReady(Cooldown);
}

void UArchonFactionMovementComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	if (IsRegistered())
	{
		Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	}

	bWillVaultThisFrame = false;

	if (bSprintHeld)
	{
		SprintHeldSeconds += DeltaTime;
	}

	FArchonFactionMovementInputState Input;
	Input.bSprintHeld = bSprintHeld;
	Input.SprintHeldSeconds = SprintHeldSeconds;
	Input.bJumpPressedThisFrame = bJumpPressedThisFrame;
	Input.bGrounded = IsOwnerGrounded();

	const FArchonFactionMovementDecision Decision = UArchonFactionMovementPolicyLibrary::EvaluateLaunch(
		Faction, Input, Cooldown, Tuning);

	if (Decision.bShouldLaunch)
	{
		if (ACharacter* OwningCharacter = GetOwningCharacter())
		{
			const FVector Direction = ComputeForwardImpulseDirection();
			const FVector Impulse = Direction * Decision.ForwardImpulse + FVector(0.0f, 0.0f, Decision.VerticalImpulse);
			OwningCharacter->LaunchCharacter(Impulse, /*bXYOverride=*/ false, /*bZOverride=*/ false);
		}
		Cooldown = Decision.NewCooldown;
		LaunchCount++;
		bWillVaultThisFrame = true;
		UE_LOG(
			LogTemp,
			Display,
			TEXT("ArchonFactoryCanary: First60RootVault launchIndex=%d magnitudeForward=%.0f magnitudeUp=%.0f"),
			LaunchCount,
			Decision.ForwardImpulse,
			Decision.VerticalImpulse);
		OnLaunched.Broadcast(Decision.VerbTriggered);
	}
	else
	{
		Cooldown = UArchonFactionMovementPolicyLibrary::AdvanceCooldown(Cooldown, DeltaTime);
		if (bJumpPressedThisFrame && bSprintHeld && SprintHeldSeconds >= Tuning.MinSprintHeldSeconds && !IsCooldownReady())
		{
			CooldownBlockedCount++;
			UE_LOG(
				LogTemp,
				Display,
				TEXT("ArchonFactoryCanary: First60RootVault cooldownEnforced=true blocked=%d"),
				CooldownBlockedCount);
		}
	}

	bJumpPressedThisFrame = false;
}

ACharacter* UArchonFactionMovementComponent::GetOwningCharacter() const
{
	return Cast<ACharacter>(GetOwner());
}

bool UArchonFactionMovementComponent::IsOwnerGrounded() const
{
	if (const ACharacter* OwningCharacter = GetOwningCharacter())
	{
		if (const UCharacterMovementComponent* Movement = OwningCharacter->GetCharacterMovement())
		{
			return Movement->IsMovingOnGround();
		}
	}
	// Outside a character context (e.g. unit-test bare component): treat as grounded so policy can evaluate.
	return true;
}

bool UArchonFactionMovementComponent::ShouldVaultOnPendingInput() const
{
	FArchonFactionMovementInputState Input;
	Input.bSprintHeld = bSprintHeld;
	Input.SprintHeldSeconds = SprintHeldSeconds;
	Input.bJumpPressedThisFrame = bJumpPressedThisFrame;
	Input.bGrounded = IsOwnerGrounded();

	const FArchonFactionMovementDecision Decision = UArchonFactionMovementPolicyLibrary::EvaluateLaunch(
		Faction,
		Input,
		Cooldown,
		Tuning);
	return Decision.bShouldLaunch;
}

FVector UArchonFactionMovementComponent::ComputeForwardImpulseDirection() const
{
	FVector Forward = FVector::ZeroVector;
	if (const ACharacter* OwningCharacter = GetOwningCharacter())
	{
		// Prefer first-person camera forward if available.
		if (UCameraComponent* Camera = OwningCharacter->FindComponentByClass<UCameraComponent>())
		{
			Forward = Camera->GetForwardVector();
		}
		else
		{
			Forward = OwningCharacter->GetActorForwardVector();
		}
	}
	Forward.Z = 0.0f;
	if (!Forward.Normalize())
	{
		Forward = FVector::ForwardVector;
	}
	return Forward;
}
