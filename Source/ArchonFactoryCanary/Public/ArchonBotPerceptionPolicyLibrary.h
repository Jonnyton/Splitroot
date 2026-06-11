#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ArchonBotPerceptionPolicyLibrary.generated.h"

/**
 * Fair-senses doctrine (Jonathan 2026-06-10: "ai players should not
 * cheat. they can only sence what a human playing them would").
 * Every bot perception decision is one of the named human channels:
 *
 *   SIGHT    — forward vision cone (the screen), LOS-checked by the
 *              caller with a world trace.
 *   HEARING  — short omnidirectional footstep radius.
 *   PAIN     — being hit grants direction toward the shot origin.
 *   HUD      — team-wide "base under attack + which building" only;
 *              never the attacker's position.
 *   EYES-ON  — a bot near an attacked building when a hit lands saw
 *              the arrow arrive and earns the origin.
 *
 * Pure math here; world queries (traces) stay in the brain.
 */
UCLASS()
class ARCHONFACTORYCANARY_API UArchonBotPerceptionPolicyLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// SIGHT: target inside the forward cone and within range. The
	// caller still owes a line-of-sight trace before acting on it.
	UFUNCTION(BlueprintPure, Category = "Archon|Bot")
	static bool IsInVisionCone(
		const FVector& ViewerLocation,
		const FVector& ViewerForward,
		const FVector& TargetLocation,
		float MaxRange,
		float HalfAngleDegrees);

	// HEARING: omnidirectional, short range, walls don't block
	// footsteps at this radius.
	UFUNCTION(BlueprintPure, Category = "Archon|Bot")
	static bool IsWithinHearing(
		const FVector& ViewerLocation,
		const FVector& TargetLocation,
		float HearingRadius);

	// EYES-ON: close enough to an attacked structure to have watched
	// the arrow arrive.
	UFUNCTION(BlueprintPure, Category = "Archon|Bot")
	static bool EarnsEyesOnOrigin(
		const FVector& ViewerLocation,
		const FVector& StructureLocation,
		float EyesOnRadius);
};
