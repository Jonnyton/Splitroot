#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class UArchonMainMenuController;

/**
 * The clickable front end. Pure Slate (no UMG assets, packaged-build
 * safe): one HOST button per registry map, a JOIN address row, QUIT.
 * Every button calls the same UArchonMainMenuController methods the
 * front-end smoke drives headlessly, so what's proven is what's clicked.
 */
class ARCHONFACTORYCANARY_API SArchonMainMenuPanel : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SArchonMainMenuPanel) {}
		SLATE_ARGUMENT(TWeakObjectPtr<UArchonMainMenuController>, MenuController)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	FReply HandleHostClicked(FName MapId);
	FReply HandleJoinClicked();
	FReply HandleQuitClicked();

	TWeakObjectPtr<UArchonMainMenuController> MenuController;
	TSharedPtr<class SEditableTextBox> JoinAddressBox;
};
