#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ArchonFactionTypes.h"
#include "ArchonFactionPaletteTypes.h"
#include "ArchonFactionTintedWidget.generated.h"

/**
 * Shared UMG base for every HUD widget in SPLITROOT. Routes through the faction
 * palette library so no widget hardcodes a hex color. Subclasses bind to the
 * OnFactionTintChangedBP event to re-color their UMG layout when the active
 * faction changes (e.g., when the player swaps team mid-match).
 *
 * Required by every UMG H2-H8 widget per splitroot-polish-umg-hud-2026-05-24.
 */
UCLASS(Blueprintable)
class ARCHONFACTORYCANARY_API UArchonFactionTintedWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Archon|HUD")
	void ConfigureFactionTint(EArchonFaction InFaction);

	UFUNCTION(BlueprintPure, Category = "Archon|HUD")
	EArchonFaction GetFaction() const { return Faction; }

	UFUNCTION(BlueprintPure, Category = "Archon|HUD")
	FLinearColor GetPrimaryTint() const;

	UFUNCTION(BlueprintPure, Category = "Archon|HUD")
	FLinearColor GetSecondaryTint() const;

	UFUNCTION(BlueprintPure, Category = "Archon|HUD")
	FLinearColor GetTertiaryTint() const;

	UFUNCTION(BlueprintPure, Category = "Archon|HUD")
	FLinearColor GetAccentTint() const;

	UFUNCTION(BlueprintPure, Category = "Archon|HUD")
	FLinearColor GetTintForSlot(EArchonFactionPaletteSlot PaletteSlot) const;

protected:
	UFUNCTION(BlueprintImplementableEvent, Category = "Archon|HUD")
	void OnFactionTintChangedBP();

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Archon|HUD")
	EArchonFaction Faction = EArchonFaction::None;
};
