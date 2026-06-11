#if WITH_DEV_AUTOMATION_TESTS

#include "ArchonFactionTintedWidget.h"
#include "ArchonFactionPaletteLibrary.h"
#include "Misc/AutomationTest.h"

namespace
{
	bool LinearColorsNearlyEqual(const FLinearColor& A, const FLinearColor& B, float Tolerance = 0.0001f)
	{
		return FMath::IsNearlyEqual(A.R, B.R, Tolerance)
			&& FMath::IsNearlyEqual(A.G, B.G, Tolerance)
			&& FMath::IsNearlyEqual(A.B, B.B, Tolerance);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonFactionTintedWidgetDefaultsToNoneFactionTest,
	"ArchonFactory.HudFactionTint.DefaultsToNoneFaction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonFactionTintedWidgetDefaultsToNoneFactionTest::RunTest(const FString& Parameters)
{
	UArchonFactionTintedWidget* Widget = NewObject<UArchonFactionTintedWidget>(GetTransientPackage());
	TestEqual(TEXT("Default faction is None"), Widget->GetFaction(), EArchonFaction::None);
	// None falls back to mid-grey via palette library.
	TestTrue(TEXT("None-faction primary tint is mid-grey fallback"),
		LinearColorsNearlyEqual(Widget->GetPrimaryTint(), FLinearColor(0.50f, 0.50f, 0.50f, 1.0f)));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonFactionTintedWidgetConfigureUpdatesFactionTest,
	"ArchonFactory.HudFactionTint.ConfigureUpdatesFaction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonFactionTintedWidgetConfigureUpdatesFactionTest::RunTest(const FString& Parameters)
{
	UArchonFactionTintedWidget* Widget = NewObject<UArchonFactionTintedWidget>(GetTransientPackage());
	Widget->ConfigureFactionTint(EArchonFaction::VerdantChoir);
	TestEqual(TEXT("Faction is Verdant after configure"),
		Widget->GetFaction(), EArchonFaction::VerdantChoir);
	TestTrue(TEXT("Verdant primary tint matches palette"),
		LinearColorsNearlyEqual(Widget->GetPrimaryTint(),
			UArchonFactionPaletteLibrary::GetFactionColor(EArchonFaction::VerdantChoir, EArchonFactionPaletteSlot::Primary)));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonFactionTintedWidgetSlotAccessorsMatchPaletteTest,
	"ArchonFactory.HudFactionTint.SlotAccessorsMatchPalette",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonFactionTintedWidgetSlotAccessorsMatchPaletteTest::RunTest(const FString& Parameters)
{
	UArchonFactionTintedWidget* Widget = NewObject<UArchonFactionTintedWidget>(GetTransientPackage());
	Widget->ConfigureFactionTint(EArchonFaction::LenswrightCompact);

	TestTrue(TEXT("Lenswright Primary via widget matches palette"),
		LinearColorsNearlyEqual(Widget->GetPrimaryTint(),
			UArchonFactionPaletteLibrary::GetFactionColor(EArchonFaction::LenswrightCompact, EArchonFactionPaletteSlot::Primary)));
	TestTrue(TEXT("Lenswright Secondary via widget matches palette"),
		LinearColorsNearlyEqual(Widget->GetSecondaryTint(),
			UArchonFactionPaletteLibrary::GetFactionColor(EArchonFaction::LenswrightCompact, EArchonFactionPaletteSlot::Secondary)));
	TestTrue(TEXT("Lenswright Tertiary via widget matches palette"),
		LinearColorsNearlyEqual(Widget->GetTertiaryTint(),
			UArchonFactionPaletteLibrary::GetFactionColor(EArchonFaction::LenswrightCompact, EArchonFactionPaletteSlot::Tertiary)));
	TestTrue(TEXT("Lenswright Accent via widget matches palette (cyan, not muzzle-flash)"),
		LinearColorsNearlyEqual(Widget->GetAccentTint(),
			UArchonFactionPaletteLibrary::GetFactionColor(EArchonFaction::LenswrightCompact, EArchonFactionPaletteSlot::Accent)));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonFactionTintedWidgetSlotByEnumAccessorTest,
	"ArchonFactory.HudFactionTint.SlotByEnumAccessor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonFactionTintedWidgetSlotByEnumAccessorTest::RunTest(const FString& Parameters)
{
	UArchonFactionTintedWidget* Widget = NewObject<UArchonFactionTintedWidget>(GetTransientPackage());
	Widget->ConfigureFactionTint(EArchonFaction::KinwildCovenant);

	TestTrue(TEXT("Kinwild Accent via GetTintForSlot matches palette"),
		LinearColorsNearlyEqual(Widget->GetTintForSlot(EArchonFactionPaletteSlot::Accent),
			UArchonFactionPaletteLibrary::GetFactionColor(EArchonFaction::KinwildCovenant, EArchonFactionPaletteSlot::Accent)));
	return true;
}

#endif
