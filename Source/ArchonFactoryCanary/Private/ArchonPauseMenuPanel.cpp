#include "ArchonPauseMenuPanel.h"
#include "ArchonFactionPaletteLibrary.h"
#include "ArchonFactionTypes.h"
#include "ArchonPlayerInputBridgeComponent.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
	struct FArchonControlRow
	{
		const TCHAR* Key;
		const TCHAR* Action;
	};

	// The player-facing key reference. Keep in sync with the input bridge's
	// TickComponent handlers — this list IS the lookup surface for controls.
	const FArchonControlRow GArchonControlRows[] =
	{
		{ TEXT("W A S D"),     TEXT("Move") },
		{ TEXT("MOUSE"),       TEXT("Look") },
		{ TEXT("SHIFT"),       TEXT("Sprint (hold)") },
		{ TEXT("SPACE"),       TEXT("Jump / root-vault") },
		{ TEXT("CTRL / C"),    TEXT("Crouch") },
		{ TEXT("LMB"),         TEXT("Fire bow") },
		{ TEXT("R"),           TEXT("Reload") },
		{ TEXT("E"),           TEXT("Interact (at prompt; move order when table open)") },
		{ TEXT("TAB"),         TEXT("RTS map table (toggle from anywhere)") },
		{ TEXT("LMB at table"), TEXT("Attack order") },
		{ TEXT("V"),           TEXT("First / third person view") },
		{ TEXT("N"),           TEXT("Bot match: next player view") },
		{ TEXT("M"),           TEXT("Bot match: shared match view") },
		{ TEXT("F"),           TEXT("Bot match: take control / release") },
		{ TEXT("ESC"),         TEXT("This menu") },
	};
}

void SArchonPauseMenuPanel::Construct(const FArguments& InArgs)
{
	InputBridge = InArgs._InputBridge;

	const FLinearColor TitleColor = UArchonFactionPaletteLibrary::GetFactionColor(
		EArchonFaction::VerdantChoir, EArchonFactionPaletteSlot::Accent);

	TSharedRef<SVerticalBox> Rows = SNew(SVerticalBox);

	Rows->AddSlot()
		.AutoHeight()
		.Padding(0.0f, 0.0f, 0.0f, 16.0f)
		.HAlign(HAlign_Center)
		[
			SNew(STextBlock)
			.Text(NSLOCTEXT("ArchonPauseMenu", "Title", "PAUSED"))
			.ColorAndOpacity(FSlateColor(TitleColor))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 36))
		];

	Rows->AddSlot()
		.AutoHeight()
		.Padding(0.0f, 4.0f)
		.HAlign(HAlign_Center)
		[
			SNew(SButton)
			.ContentPadding(FMargin(24.0f, 8.0f))
			.OnClicked(this, &SArchonPauseMenuPanel::HandleResumeClicked)
			[
				SNew(STextBlock)
				.Text(NSLOCTEXT("ArchonPauseMenu", "Resume", "RESUME"))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 18))
			]
		];

	Rows->AddSlot()
		.AutoHeight()
		.Padding(0.0f, 18.0f, 0.0f, 6.0f)
		.HAlign(HAlign_Center)
		[
			SNew(STextBlock)
			.Text(NSLOCTEXT("ArchonPauseMenu", "Controls", "CONTROLS"))
			.ColorAndOpacity(FSlateColor(TitleColor))
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", 20))
		];

	for (const FArchonControlRow& Row : GArchonControlRows)
	{
		Rows->AddSlot()
			.AutoHeight()
			.Padding(0.0f, 1.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SBox)
					.WidthOverride(150.0f)
					[
						SNew(STextBlock)
						.Text(FText::FromString(Row.Key))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 13))
					]
				]
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(Row.Action))
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 13))
				]
			];
	}

	Rows->AddSlot()
		.AutoHeight()
		.Padding(0.0f, 14.0f, 0.0f, 2.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(SBox)
				.WidthOverride(175.0f)
				[
					SNew(STextBlock)
					.Text(NSLOCTEXT("ArchonPauseMenu", "LookSpeed", "MOUSE LOOK SPEED"))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 13))
				]
			]
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.VAlign(VAlign_Center)
			.Padding(0.0f, 0.0f, 8.0f, 0.0f)
			[
				SNew(SSlider)
				.Value(this, &SArchonPauseMenuPanel::GetLookScaleNormalized)
				.OnValueChanged(this, &SArchonPauseMenuPanel::HandleLookScaleChanged)
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(this, &SArchonPauseMenuPanel::GetLookScaleText)
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 13))
			]
		];

	Rows->AddSlot()
		.AutoHeight()
		.Padding(0.0f, 18.0f, 0.0f, 4.0f)
		.HAlign(HAlign_Center)
		[
			SNew(SButton)
			.ContentPadding(FMargin(24.0f, 8.0f))
			.OnClicked(this, &SArchonPauseMenuPanel::HandleQuitToMenuClicked)
			[
				SNew(STextBlock)
				.Text(NSLOCTEXT("ArchonPauseMenu", "QuitToMenu", "QUIT TO MAIN MENU"))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 16))
			]
		];

	Rows->AddSlot()
		.AutoHeight()
		.Padding(0.0f, 4.0f)
		.HAlign(HAlign_Center)
		[
			SNew(SButton)
			.ContentPadding(FMargin(24.0f, 8.0f))
			.OnClicked(this, &SArchonPauseMenuPanel::HandleQuitGameClicked)
			[
				SNew(STextBlock)
				.Text(NSLOCTEXT("ArchonPauseMenu", "QuitGame", "QUIT GAME"))
				.Font(FCoreStyle::GetDefaultFontStyle("Regular", 16))
			]
		];

	// Fullscreen dim border: catches every click outside the panel so a
	// stray click can never reach the viewport and recapture the mouse.
	// Inner card: the controls text needs its own solid backing to stay
	// readable over a bright world (playtest finding 2026-06-10).
	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
		.BorderBackgroundColor(FSlateColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.65f)))
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
			.BorderBackgroundColor(FSlateColor(FLinearColor(0.02f, 0.04f, 0.02f, 0.9f)))
			.Padding(FMargin(28.0f, 22.0f))
			[
				SNew(SBox)
				.WidthOverride(560.0f)
				[
					Rows
				]
			]
		]
	];
}

FReply SArchonPauseMenuPanel::HandleResumeClicked()
{
	if (UArchonPlayerInputBridgeComponent* Bridge = InputBridge.Get())
	{
		Bridge->TogglePauseMenu();
	}
	return FReply::Handled();
}

FReply SArchonPauseMenuPanel::HandleQuitToMenuClicked()
{
	if (UArchonPlayerInputBridgeComponent* Bridge = InputBridge.Get())
	{
		Bridge->QuitToMainMenu();
	}
	return FReply::Handled();
}

FReply SArchonPauseMenuPanel::HandleQuitGameClicked()
{
	if (UArchonPlayerInputBridgeComponent* Bridge = InputBridge.Get())
	{
		Bridge->QuitGame();
	}
	return FReply::Handled();
}

void SArchonPauseMenuPanel::HandleLookScaleChanged(float NewNormalizedValue)
{
	if (UArchonPlayerInputBridgeComponent* Bridge = InputBridge.Get())
	{
		const float Scale = FMath::Lerp(
			UArchonPlayerInputBridgeComponent::MinMouseLookScale,
			UArchonPlayerInputBridgeComponent::MaxMouseLookScale,
			FMath::Clamp(NewNormalizedValue, 0.0f, 1.0f));
		Bridge->SetMouseLookScale(Scale);
	}
}

float SArchonPauseMenuPanel::GetLookScaleNormalized() const
{
	if (const UArchonPlayerInputBridgeComponent* Bridge = InputBridge.Get())
	{
		return FMath::GetRangePct(
			UArchonPlayerInputBridgeComponent::MinMouseLookScale,
			UArchonPlayerInputBridgeComponent::MaxMouseLookScale,
			Bridge->GetMouseLookScale());
	}
	return 0.0f;
}

FText SArchonPauseMenuPanel::GetLookScaleText() const
{
	if (const UArchonPlayerInputBridgeComponent* Bridge = InputBridge.Get())
	{
		return FText::FromString(FString::Printf(TEXT("%.3f"), Bridge->GetMouseLookScale()));
	}
	return FText::GetEmpty();
}
