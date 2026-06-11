#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ArchonInteractPromptPolicyLibrary.generated.h"

USTRUCT(BlueprintType)
struct FArchonInteractPromptDecision
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Archon|Interact")
	bool bInRange = false;

	// Show the "[E] ..." indicator: in range AND the interface it would
	// open is not already open.
	UPROPERTY(BlueprintReadOnly, Category = "Archon|Interact")
	bool bShowPrompt = false;
};

/**
 * Pure policy for the E-interact convention (Jonathan 2026-06-10):
 * every world interactable (map table, purchase terminals, ...) shows a
 * pop-up indicator when the player is close enough, and E opens its
 * interface. Range math lives here so it is unit-testable and shared by
 * every interactable kind.
 */
UCLASS()
class ARCHONFACTORYCANARY_API UArchonInteractPromptPolicyLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Archon|Interact")
	static FArchonInteractPromptDecision EvaluateInteractPrompt(
		const FVector& PlayerLocation,
		const FVector& InteractableLocation,
		float InteractRadius,
		bool bInterfaceAlreadyOpen);

	// One standard reach for v0 interactables; per-kind overrides come
	// later if playtests want them.
	static constexpr float DefaultInteractRadius = 600.0f;
};
