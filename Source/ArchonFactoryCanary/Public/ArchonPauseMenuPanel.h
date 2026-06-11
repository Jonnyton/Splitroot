#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class UArchonPlayerInputBridgeComponent;

/**
 * In-game system menu (Escape). Pure Slate, packaged-build safe.
 * RESUME / CONTROLS (key reference + mouse look-speed slider) /
 * QUIT TO MAIN MENU / QUIT GAME. Buttons call the same input-bridge
 * methods the pause-menu proof drives headlessly, so what's proven is
 * what's clicked. The controls list is the player's lookup surface for
 * what keys exist (Jonathan 2026-06-10: "a place to look up controls").
 */
class ARCHONFACTORYCANARY_API SArchonPauseMenuPanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SArchonPauseMenuPanel) {}
		SLATE_ARGUMENT(TWeakObjectPtr<UArchonPlayerInputBridgeComponent>, InputBridge)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	FReply HandleResumeClicked();
	FReply HandleQuitToMenuClicked();
	FReply HandleQuitGameClicked();
	void HandleLookScaleChanged(float NewNormalizedValue);
	float GetLookScaleNormalized() const;
	FText GetLookScaleText() const;
	void HandleFlySpeedChanged(float NewNormalizedValue);
	float GetFlySpeedNormalized() const;
	FText GetFlySpeedText() const;

	TWeakObjectPtr<UArchonPlayerInputBridgeComponent> InputBridge;
};
