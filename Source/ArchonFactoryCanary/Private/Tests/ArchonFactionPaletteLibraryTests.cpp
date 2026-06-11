#if WITH_DEV_AUTOMATION_TESTS

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
	FArchonPaletteVerdantSlotsMatchSpecTest,
	"ArchonFactory.Palette.VerdantSlotsMatchSpec",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonPaletteVerdantSlotsMatchSpecTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("Verdant Primary green-cream"),
		LinearColorsNearlyEqual(
			UArchonFactionPaletteLibrary::GetFactionColor(EArchonFaction::VerdantChoir, EArchonFactionPaletteSlot::Primary),
			FLinearColor(0.30f, 0.55f, 0.25f, 1.0f)));
	TestTrue(TEXT("Verdant Secondary bone cream"),
		LinearColorsNearlyEqual(
			UArchonFactionPaletteLibrary::GetFactionColor(EArchonFaction::VerdantChoir, EArchonFactionPaletteSlot::Secondary),
			FLinearColor(0.96f, 0.91f, 0.78f, 1.0f)));
	TestTrue(TEXT("Verdant Tertiary mossy depth"),
		LinearColorsNearlyEqual(
			UArchonFactionPaletteLibrary::GetFactionColor(EArchonFaction::VerdantChoir, EArchonFactionPaletteSlot::Tertiary),
			FLinearColor(0.11f, 0.23f, 0.12f, 1.0f)));
	TestTrue(TEXT("Verdant Accent chartreuse"),
		LinearColorsNearlyEqual(
			UArchonFactionPaletteLibrary::GetFactionColor(EArchonFaction::VerdantChoir, EArchonFactionPaletteSlot::Accent),
			FLinearColor(0.66f, 0.82f, 0.38f, 1.0f)));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonPaletteLenswrightSlotsMatchSpecTest,
	"ArchonFactory.Palette.LenswrightSlotsMatchSpec",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonPaletteLenswrightSlotsMatchSpecTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("Lenswright Primary brass-oxblood"),
		LinearColorsNearlyEqual(
			UArchonFactionPaletteLibrary::GetFactionColor(EArchonFaction::LenswrightCompact, EArchonFactionPaletteSlot::Primary),
			FLinearColor(0.55f, 0.30f, 0.15f, 1.0f)));
	TestTrue(TEXT("Lenswright Secondary polished brass"),
		LinearColorsNearlyEqual(
			UArchonFactionPaletteLibrary::GetFactionColor(EArchonFaction::LenswrightCompact, EArchonFactionPaletteSlot::Secondary),
			FLinearColor(0.85f, 0.66f, 0.42f, 1.0f)));
	TestTrue(TEXT("Lenswright Tertiary oxblood iron"),
		LinearColorsNearlyEqual(
			UArchonFactionPaletteLibrary::GetFactionColor(EArchonFaction::LenswrightCompact, EArchonFactionPaletteSlot::Tertiary),
			FLinearColor(0.24f, 0.12f, 0.10f, 1.0f)));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonPaletteLenswrightAccentIsCyanNotMuzzleFlashTest,
	"ArchonFactory.Palette.LenswrightAccentIsCyanNotMuzzleFlash",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonPaletteLenswrightAccentIsCyanNotMuzzleFlashTest::RunTest(const FString& Parameters)
{
	const FLinearColor Accent =
		UArchonFactionPaletteLibrary::GetFactionColor(EArchonFaction::LenswrightCompact, EArchonFactionPaletteSlot::Accent);

	// Cyan alchemical glow: blue+green dominant, red minimal. Muzzle-flash orange would
	// be red+green dominant with low blue. Test the inequality to catch any drift toward
	// firearm colors that would break the no-gunpowder hill at the palette layer.
	TestTrue(TEXT("Lenswright accent matches cyan spec"),
		LinearColorsNearlyEqual(Accent, FLinearColor(0.37f, 0.78f, 0.88f, 1.0f)));
	TestTrue(TEXT("Lenswright accent blue dominates red (cyan, not muzzle-flash)"), Accent.B > Accent.R);
	TestTrue(TEXT("Lenswright accent green dominates red (cyan, not muzzle-flash)"), Accent.G > Accent.R);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonPaletteKinwildSlotsMatchSpecTest,
	"ArchonFactory.Palette.KinwildSlotsMatchSpec",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonPaletteKinwildSlotsMatchSpecTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("Kinwild Primary ochre"),
		LinearColorsNearlyEqual(
			UArchonFactionPaletteLibrary::GetFactionColor(EArchonFaction::KinwildCovenant, EArchonFactionPaletteSlot::Primary),
			FLinearColor(0.65f, 0.50f, 0.24f, 1.0f)));
	TestTrue(TEXT("Kinwild Secondary granite grey"),
		LinearColorsNearlyEqual(
			UArchonFactionPaletteLibrary::GetFactionColor(EArchonFaction::KinwildCovenant, EArchonFactionPaletteSlot::Secondary),
			FLinearColor(0.49f, 0.54f, 0.55f, 1.0f)));
	TestTrue(TEXT("Kinwild Accent slate-blue ritual paint"),
		LinearColorsNearlyEqual(
			UArchonFactionPaletteLibrary::GetFactionColor(EArchonFaction::KinwildCovenant, EArchonFactionPaletteSlot::Accent),
			FLinearColor(0.25f, 0.43f, 0.48f, 1.0f)));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonPaletteNoneFallsBackToNeutralTest,
	"ArchonFactory.Palette.NoneFallsBackToNeutral",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonPaletteNoneFallsBackToNeutralTest::RunTest(const FString& Parameters)
{
	const FLinearColor Fallback =
		UArchonFactionPaletteLibrary::GetFactionColor(EArchonFaction::None, EArchonFactionPaletteSlot::Primary);
	TestTrue(TEXT("None faction returns mid-grey fallback"),
		LinearColorsNearlyEqual(Fallback, FLinearColor(0.50f, 0.50f, 0.50f, 1.0f)));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonPaletteMaterialResponsePerFactionTest,
	"ArchonFactory.Palette.MaterialResponsePerFaction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonPaletteMaterialResponsePerFactionTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("Verdant organic matte: metallic 0.1"),
		UArchonFactionPaletteLibrary::GetFactionMetallic(EArchonFaction::VerdantChoir), 0.1f);
	TestEqual(TEXT("Verdant organic matte: roughness 0.7"),
		UArchonFactionPaletteLibrary::GetFactionRoughness(EArchonFaction::VerdantChoir), 0.7f);

	TestEqual(TEXT("Lenswright clockwork sheen: metallic 0.6"),
		UArchonFactionPaletteLibrary::GetFactionMetallic(EArchonFaction::LenswrightCompact), 0.6f);
	TestEqual(TEXT("Lenswright clockwork sheen: roughness 0.3"),
		UArchonFactionPaletteLibrary::GetFactionRoughness(EArchonFaction::LenswrightCompact), 0.3f);

	TestEqual(TEXT("Kinwild weathered: metallic 0.2"),
		UArchonFactionPaletteLibrary::GetFactionMetallic(EArchonFaction::KinwildCovenant), 0.2f);
	TestEqual(TEXT("Kinwild weathered: roughness 0.5"),
		UArchonFactionPaletteLibrary::GetFactionRoughness(EArchonFaction::KinwildCovenant), 0.5f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonPaletteNeutralSlotsMatchShippedDebugColorsTest,
	"ArchonFactory.Palette.NeutralSlotsMatchShippedDebugColors",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonPaletteNeutralSlotsMatchShippedDebugColorsTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("Cover stone grey matches shipped blockout debug color"),
		LinearColorsNearlyEqual(
			UArchonFactionPaletteLibrary::GetNeutralColor(EArchonNeutralPaletteSlot::CoverStone),
			FLinearColor(0.35f, 0.35f, 0.35f, 1.0f)));
	TestTrue(TEXT("Splitroot wood matches shipped central tree debug color"),
		LinearColorsNearlyEqual(
			UArchonFactionPaletteLibrary::GetNeutralColor(EArchonNeutralPaletteSlot::SplitrootWood),
			FLinearColor(0.45f, 0.30f, 0.15f, 1.0f)));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonPaletteLightingAnchorMatchesArtDirectionPlanTest,
	"ArchonFactory.Palette.LightingAnchorMatchesArtDirectionPlan",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonPaletteLightingAnchorMatchesArtDirectionPlanTest::RunTest(const FString& Parameters)
{
	const FArchonLightingAnchor Anchor = UArchonFactionPaletteLibrary::GetLightingAnchor();
	TestEqual(TEXT("Late-afternoon sun pitch -25 deg"), Anchor.DirectionalPitchDegrees, -25.0f);
	TestEqual(TEXT("Sun yaw 30 deg from north"), Anchor.DirectionalYawDegrees, 30.0f);
	TestEqual(TEXT("Sky light intensity 0.3"), Anchor.SkyLightIntensity, 0.3f);
	TestEqual(TEXT("Bloom 0.4 — production preset, no spam"), Anchor.PostProcessBloom, 0.4f);
	return true;
}

#endif
