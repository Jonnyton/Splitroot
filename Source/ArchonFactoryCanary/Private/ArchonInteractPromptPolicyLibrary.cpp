#include "ArchonInteractPromptPolicyLibrary.h"

FArchonInteractPromptDecision UArchonInteractPromptPolicyLibrary::EvaluateInteractPrompt(
	const FVector& PlayerLocation,
	const FVector& InteractableLocation,
	float InteractRadius,
	bool bInterfaceAlreadyOpen)
{
	FArchonInteractPromptDecision Decision;

	// 2D distance: a table on a pad or a terminal up a short ramp should
	// not lose its prompt over height differences a player can step over.
	Decision.bInRange =
		InteractRadius > 0.0f &&
		FVector::Dist2D(PlayerLocation, InteractableLocation) <= InteractRadius;
	Decision.bShowPrompt = Decision.bInRange && !bInterfaceAlreadyOpen;
	return Decision;
}
