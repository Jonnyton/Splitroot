#if WITH_DEV_AUTOMATION_TESTS

#include "ArchonFactionSilhouetteLibrary.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonSilhouetteVerdantIsVerticalAsymmetricOrganicTest,
	"ArchonFactory.Silhouette.VerdantIsVerticalAsymmetricOrganic",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonSilhouetteVerdantIsVerticalAsymmetricOrganicTest::RunTest(const FString& Parameters)
{
	const FArchonFactionSilhouette S = UArchonFactionSilhouetteLibrary::GetFactionSilhouette(EArchonFaction::VerdantChoir);
	TestTrue(TEXT("Verdant is taller than baseline (vertical bias)"), S.CapsuleHeightMultiplier > 1.0f);
	TestEqual(TEXT("Verdant top cap is rounded organic"), S.MantleType, EArchonMantleType::RoundedTopCap);
	TestTrue(TEXT("Verdant has an asymmetric shoulder protrusion"), S.ShoulderProtrusionUnits > 0.0f);
	TestEqual(TEXT("Verdant shoulder is on the right (quiver / sprouting bramble)"), S.ShoulderSide, EArchonShoulderSide::Right);
	TestFalse(TEXT("Verdant has no animal companion"), S.bHasAnimalCompanion);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonSilhouetteLenswrightIsAnchoredMechanicalTest,
	"ArchonFactory.Silhouette.LenswrightIsAnchoredMechanical",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonSilhouetteLenswrightIsAnchoredMechanicalTest::RunTest(const FString& Parameters)
{
	const FArchonFactionSilhouette S = UArchonFactionSilhouetteLibrary::GetFactionSilhouette(EArchonFaction::LenswrightCompact);
	TestTrue(TEXT("Lenswright has wider base (1.2x lower radius — anchored)"), S.LowerCapsuleRadiusMultiplier > 1.0f);
	TestEqual(TEXT("Lenswright mantle is the crossbow shoulder protrusion"), S.MantleType, EArchonMantleType::RightShoulderProtrusion);
	TestTrue(TEXT("Lenswright shoulder protrusion is substantial (crossbow silhouette)"), S.ShoulderProtrusionUnits >= 50.0f);
	TestTrue(TEXT("Lenswright has reflective head-lens sphere"), S.HeadLensSphereRadiusUnits > 0.0f);
	TestFalse(TEXT("Lenswright has no animal companion"), S.bHasAnimalCompanion);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonSilhouetteKinwildIsForwardLeaningWithCompanionTest,
	"ArchonFactory.Silhouette.KinwildIsForwardLeaningWithCompanion",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonSilhouetteKinwildIsForwardLeaningWithCompanionTest::RunTest(const FString& Parameters)
{
	const FArchonFactionSilhouette S = UArchonFactionSilhouetteLibrary::GetFactionSilhouette(EArchonFaction::KinwildCovenant);
	TestTrue(TEXT("Kinwild leans forward (hunt-stalk)"), S.ForwardLeanDegrees > 0.0f);
	TestEqual(TEXT("Kinwild wears a mantle cloak"), S.MantleType, EArchonMantleType::MantleCloak);
	TestTrue(TEXT("Kinwild has an animal companion"), S.bHasAnimalCompanion);
	TestTrue(TEXT("Kinwild companion has a positive capsule height"), S.CompanionCapsuleHeightUnits > 0.0f);
	TestTrue(TEXT("Kinwild companion is offset to the right of the unit"), S.CompanionOffsetRightUnits > 0.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonSilhouetteFactionsAreDistinctSilhouettesTest,
	"ArchonFactory.Silhouette.FactionsAreDistinctSilhouettes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonSilhouetteFactionsAreDistinctSilhouettesTest::RunTest(const FString& Parameters)
{
	const FArchonFactionSilhouette V = UArchonFactionSilhouetteLibrary::GetFactionSilhouette(EArchonFaction::VerdantChoir);
	const FArchonFactionSilhouette L = UArchonFactionSilhouetteLibrary::GetFactionSilhouette(EArchonFaction::LenswrightCompact);
	const FArchonFactionSilhouette K = UArchonFactionSilhouetteLibrary::GetFactionSilhouette(EArchonFaction::KinwildCovenant);

	// The discipline rule says factions must be silhouette-distinct at 30m.
	// Mantle type alone should distinguish all three.
	TestTrue(TEXT("Verdant vs Lenswright mantle differs"), V.MantleType != L.MantleType);
	TestTrue(TEXT("Verdant vs Kinwild mantle differs"), V.MantleType != K.MantleType);
	TestTrue(TEXT("Lenswright vs Kinwild mantle differs"), L.MantleType != K.MantleType);

	// Plus at least one orthogonal silhouette cue is unique per faction.
	TestTrue(TEXT("Only Verdant is taller than baseline"),
		V.CapsuleHeightMultiplier > 1.0f && L.CapsuleHeightMultiplier == 1.0f && K.CapsuleHeightMultiplier == 1.0f);
	TestTrue(TEXT("Only Lenswright has the wider base"),
		L.LowerCapsuleRadiusMultiplier > 1.0f && V.LowerCapsuleRadiusMultiplier == 1.0f && K.LowerCapsuleRadiusMultiplier == 1.0f);
	TestTrue(TEXT("Only Kinwild forward-leans"),
		K.ForwardLeanDegrees > 0.0f && V.ForwardLeanDegrees == 0.0f && L.ForwardLeanDegrees == 0.0f);
	TestTrue(TEXT("Only Kinwild has an animal companion"),
		K.bHasAnimalCompanion && !V.bHasAnimalCompanion && !L.bHasAnimalCompanion);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonSilhouetteHeroOverlayIsScaleAndRingTest,
	"ArchonFactory.Silhouette.HeroOverlayIsScaleAndRing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonSilhouetteHeroOverlayIsScaleAndRingTest::RunTest(const FString& Parameters)
{
	const FArchonHeroSilhouetteOverlay H = UArchonFactionSilhouetteLibrary::GetHeroOverlay();
	TestTrue(TEXT("Hero scale is greater than 1.0 (always-recognizable presence)"), H.ScaleMultiplier > 1.0f);
	TestTrue(TEXT("Hero ground-ring decal is on"), H.bGroundRingDecal);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonSilhouetteResolvedCapsuleScalesByMultiplierTest,
	"ArchonFactory.Silhouette.ResolvedCapsuleScalesByMultiplier",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonSilhouetteResolvedCapsuleScalesByMultiplierTest::RunTest(const FString& Parameters)
{
	const float Baseline = UArchonFactionSilhouetteLibrary::GetBaselineCapsuleHeightUnits();
	TestEqual(TEXT("Verdant resolved height is 1.05x baseline"),
		UArchonFactionSilhouetteLibrary::GetResolvedCapsuleHeightUnits(EArchonFaction::VerdantChoir),
		Baseline * 1.05f);
	TestEqual(TEXT("Lenswright resolved height matches baseline"),
		UArchonFactionSilhouetteLibrary::GetResolvedCapsuleHeightUnits(EArchonFaction::LenswrightCompact),
		Baseline);
	TestEqual(TEXT("None faction resolved height matches baseline"),
		UArchonFactionSilhouetteLibrary::GetResolvedCapsuleHeightUnits(EArchonFaction::None),
		Baseline);
	return true;
}

#endif
