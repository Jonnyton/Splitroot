#include "ArchonMainMenuPanel.h"
#include "ArchonFactionPaletteLibrary.h"
#include "ArchonFactionTypes.h"
#include "ArchonMainMenuController.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

void SArchonMainMenuPanel::Construct(const FArguments& InArgs)
{
	MenuController = InArgs._MenuController;

	const FLinearColor TitleColor = UArchonFactionPaletteLibrary::GetFactionColor(
		EArchonFaction::VerdantChoir, EArchonFactionPaletteSlot::Accent);

	TSharedRef<SVerticalBox> Rows = SNew(SVerticalBox);

	Rows->AddSlot()
		.AutoHeight()
		.Padding(0.0f, 0.0f, 0.0f, 24.0f)
		.HAlign(HAlign_Center)
		[
			SNew(STextBlock)
			.Text(NSLOCTEXT("ArchonMainMenu", "Title", "SPLITROOT"))
			.ColorAndOpacity(FSlateColor(TitleColor))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 42))
		];

	for (const FArchonMapEntry& Entry : UArchonMainMenuController::GetHostableMaps())
	{
		Rows->AddSlot()
			.AutoHeight()
			.Padding(0.0f, 4.0f)
			[
				SNew(SButton)
				.HAlign(HAlign_Center)
				.ContentPadding(FMargin(24.0f, 10.0f))
				.OnClicked(this, &SArchonMainMenuPanel::HandleHostClicked, Entry.MapId)
				[
					SNew(STextBlock)
					.Text(FText::Format(
						NSLOCTEXT("ArchonMainMenu", "HostMap", "HOST — {0}"),
						FText::FromString(Entry.DisplayName)))
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 18))
				]
			];
	}

	Rows->AddSlot()
		.AutoHeight()
		.Padding(0.0f, 16.0f, 0.0f, 4.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.Padding(0.0f, 0.0f, 8.0f, 0.0f)
			[
				SAssignNew(JoinAddressBox, SEditableTextBox)
				.HintText(NSLOCTEXT("ArchonMainMenu", "JoinHint", "host address (ip[:port])"))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 16))
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SButton)
				.ContentPadding(FMargin(24.0f, 8.0f))
				.OnClicked(this, &SArchonMainMenuPanel::HandleJoinClicked)
				[
					SNew(STextBlock)
					.Text(NSLOCTEXT("ArchonMainMenu", "Join", "JOIN"))
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 18))
				]
			]
		];

	Rows->AddSlot()
		.AutoHeight()
		.Padding(0.0f, 24.0f, 0.0f, 0.0f)
		.HAlign(HAlign_Center)
		[
			SNew(SButton)
			.ContentPadding(FMargin(24.0f, 8.0f))
			.OnClicked(this, &SArchonMainMenuPanel::HandleQuitClicked)
			[
				SNew(STextBlock)
				.Text(NSLOCTEXT("ArchonMainMenu", "Quit", "QUIT"))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 16))
			]
		];

	ChildSlot
	[
		SNew(SBox)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SBox)
			.WidthOverride(520.0f)
			[
				Rows
			]
		]
	];
}

FReply SArchonMainMenuPanel::HandleHostClicked(FName MapId)
{
	if (UArchonMainMenuController* Controller = MenuController.Get())
	{
		// Decision 9: any single-player host gets the all-AI match as
		// standard — bots fill the slots, the human starts spectating
		// and can take any player over (joining humans replace bots
		// later, decision 6).
		Controller->HostMap(MapId, { TEXT("ArchonBotMatch") });
	}
	return FReply::Handled();
}

FReply SArchonMainMenuPanel::HandleJoinClicked()
{
	UArchonMainMenuController* Controller = MenuController.Get();
	if (Controller && JoinAddressBox.IsValid())
	{
		Controller->JoinByAddress(JoinAddressBox->GetText().ToString());
	}
	return FReply::Handled();
}

FReply SArchonMainMenuPanel::HandleQuitClicked()
{
	if (UArchonMainMenuController* Controller = MenuController.Get())
	{
		Controller->QuitGame();
	}
	return FReply::Handled();
}
