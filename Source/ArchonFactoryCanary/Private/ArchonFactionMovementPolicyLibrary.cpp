#include "ArchonFactionMovementPolicyLibrary.h"

EArchonFactionMovementVerb UArchonFactionMovementPolicyLibrary::GetMovementVerbForFaction(EArchonFaction Faction)
{
	switch (Faction)
	{
	case EArchonFaction::VerdantChoir:
		return EArchonFactionMovementVerb::VerdantRootVault;
	case EArchonFaction::KinwildCovenant:
	case EArchonFaction::LenswrightCompact:
	case EArchonFaction::None:
	default:
		// v0: only Verdant has a movement verb. Kinwild and Lenswright land in later slices.
		return EArchonFactionMovementVerb::None;
	}
}

FArchonFactionMovementDecision UArchonFactionMovementPolicyLibrary::EvaluateLaunch(
	EArchonFaction Faction,
	const FArchonFactionMovementInputState& Input,
	const FArchonFactionMovementCooldown& CurrentCooldown,
	const FArchonFactionMovementTuning& Tuning)
{
	FArchonFactionMovementDecision Decision;

	const EArchonFactionMovementVerb Verb = GetMovementVerbForFaction(Faction);
	if (Verb == EArchonFactionMovementVerb::None)
	{
		return Decision;
	}

	if (!IsCooldownReady(CurrentCooldown))
	{
		return Decision;
	}

	if (!Input.bSprintHeld)
	{
		return Decision;
	}

	if (Input.SprintHeldSeconds < Tuning.MinSprintHeldSeconds)
	{
		return Decision;
	}

	if (!Input.bJumpPressedThisFrame)
	{
		return Decision;
	}

	if (Tuning.bRequireGroundedAtLaunch && !Input.bGrounded)
	{
		return Decision;
	}

	Decision.bShouldLaunch = true;
	Decision.ForwardImpulse = Tuning.LaunchImpulseForward;
	Decision.VerticalImpulse = Tuning.LaunchImpulseVertical;
	Decision.NewCooldown.SecondsRemaining = Tuning.CooldownSeconds;
	Decision.NewCooldown.TotalSeconds = Tuning.CooldownSeconds;
	Decision.VerbTriggered = Verb;
	return Decision;
}

FArchonFactionMovementCooldown UArchonFactionMovementPolicyLibrary::AdvanceCooldown(
	const FArchonFactionMovementCooldown& Current,
	float DeltaSeconds)
{
	FArchonFactionMovementCooldown Next = Current;
	Next.SecondsRemaining = FMath::Max(0.0f, Current.SecondsRemaining - DeltaSeconds);
	return Next;
}

bool UArchonFactionMovementPolicyLibrary::IsCooldownReady(const FArchonFactionMovementCooldown& Cooldown)
{
	return Cooldown.SecondsRemaining <= 0.0f;
}
