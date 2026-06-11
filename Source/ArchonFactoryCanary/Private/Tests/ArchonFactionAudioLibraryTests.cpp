#if WITH_DEV_AUTOMATION_TESTS

#include "ArchonFactionAudioLibrary.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonAudioForbiddenTermsCoverFirearmFamilyTest,
	"ArchonFactory.Audio.ForbiddenTermsCoverFirearmFamily",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonAudioForbiddenTermsCoverFirearmFamilyTest::RunTest(const FString& Parameters)
{
	const TArray<FString> Terms = UArchonFactionAudioLibrary::GetLenswrightForbiddenTerms();

	TestTrue(TEXT("Forbidden terms include 'gunshot'"), Terms.Contains(TEXT("gunshot")));
	TestTrue(TEXT("Forbidden terms include 'firearm'"), Terms.Contains(TEXT("firearm")));
	TestTrue(TEXT("Forbidden terms include 'muzzle'"), Terms.Contains(TEXT("muzzle")));
	TestTrue(TEXT("Forbidden terms include 'gunpowder'"), Terms.Contains(TEXT("gunpowder")));
	TestTrue(TEXT("Forbidden terms include 'flintlock'"), Terms.Contains(TEXT("flintlock")));
	TestTrue(TEXT("At least 16 firearm-family terms guarded"), Terms.Num() >= 16);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonAudioLenswrightCueValidatorRejectsFirearmNamesTest,
	"ArchonFactory.Audio.LenswrightCueValidatorRejectsFirearmNames",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonAudioLenswrightCueValidatorRejectsFirearmNamesTest::RunTest(const FString& Parameters)
{
	TestFalse(TEXT("'lenswright_gunshot_close' rejected"),
		UArchonFactionAudioLibrary::ValidateLenswrightCueName(TEXT("lenswright_gunshot_close")));
	TestFalse(TEXT("'muzzle_flash_blast' rejected"),
		UArchonFactionAudioLibrary::ValidateLenswrightCueName(TEXT("muzzle_flash_blast")));
	TestFalse(TEXT("'OldWest_Rifle_Crack' rejected (case-insensitive)"),
		UArchonFactionAudioLibrary::ValidateLenswrightCueName(TEXT("OldWest_Rifle_Crack")));
	TestFalse(TEXT("'flintlock_pistol_pop' rejected"),
		UArchonFactionAudioLibrary::ValidateLenswrightCueName(TEXT("flintlock_pistol_pop")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonAudioLenswrightCueValidatorAcceptsLegitimateNamesTest,
	"ArchonFactory.Audio.LenswrightCueValidatorAcceptsLegitimateNames",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonAudioLenswrightCueValidatorAcceptsLegitimateNamesTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("'lenswright_pressure_vent_hiss' accepted"),
		UArchonFactionAudioLibrary::ValidateLenswrightCueName(TEXT("lenswright_pressure_vent_hiss")));
	TestTrue(TEXT("'bolt_thwap_metal' accepted"),
		UArchonFactionAudioLibrary::ValidateLenswrightCueName(TEXT("bolt_thwap_metal")));
	TestTrue(TEXT("'clockwork_tick_loop' accepted"),
		UArchonFactionAudioLibrary::ValidateLenswrightCueName(TEXT("clockwork_tick_loop")));
	TestTrue(TEXT("'gear_grind_short' accepted"),
		UArchonFactionAudioLibrary::ValidateLenswrightCueName(TEXT("gear_grind_short")));
	TestTrue(TEXT("'hydraulic_clunk_deploy' accepted"),
		UArchonFactionAudioLibrary::ValidateLenswrightCueName(TEXT("hydraulic_clunk_deploy")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonAudioPerFactionValidatorOnlyEnforcedForLenswrightTest,
	"ArchonFactory.Audio.PerFactionValidatorOnlyEnforcedForLenswright",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonAudioPerFactionValidatorOnlyEnforcedForLenswrightTest::RunTest(const FString& Parameters)
{
	// Verdant/Kinwild are not gated on the firearm terms; a cue named
	// 'verdant_arrow_crack' is acceptable (a crack is a wood-string sound for them).
	TestTrue(TEXT("Verdant 'verdant_arrow_crack' accepted"),
		UArchonFactionAudioLibrary::ValidateCueNameForFaction(EArchonFaction::VerdantChoir, TEXT("verdant_arrow_crack")));
	TestTrue(TEXT("Kinwild 'kinwild_horn_bang' accepted"),
		UArchonFactionAudioLibrary::ValidateCueNameForFaction(EArchonFaction::KinwildCovenant, TEXT("kinwild_horn_bang")));

	// Lenswright is gated.
	TestFalse(TEXT("Lenswright 'lenswright_gunshot' rejected"),
		UArchonFactionAudioLibrary::ValidateCueNameForFaction(EArchonFaction::LenswrightCompact, TEXT("lenswright_gunshot")));
	TestTrue(TEXT("Lenswright 'lenswright_pressure_vent' accepted"),
		UArchonFactionAudioLibrary::ValidateCueNameForFaction(EArchonFaction::LenswrightCompact, TEXT("lenswright_pressure_vent")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonAudioCueLookupReturnsNullWhileAssetsPendingTest,
	"ArchonFactory.Audio.CueLookupReturnsNullWhileAssetsPending",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonAudioCueLookupReturnsNullWhileAssetsPendingTest::RunTest(const FString& Parameters)
{
	// Honest absence: audio assets are not imported yet. Lookups return nullptr
	// so callers gate on null rather than crashing. When Jonathan/Hex imports
	// Content/Audio/Verdant/*.uasset etc., these become non-null and this test
	// flips its expectation in the same commit that wires the lookups.
	TestNull(TEXT("Verdant weapon-fire cue: not yet imported"),
		UArchonFactionAudioLibrary::GetFactionWeaponFireCue(EArchonFaction::VerdantChoir));
	TestNull(TEXT("Lenswright weapon-fire cue: not yet imported"),
		UArchonFactionAudioLibrary::GetFactionWeaponFireCue(EArchonFaction::LenswrightCompact));
	TestNull(TEXT("MatchStart strategic cue: not yet imported"),
		UArchonFactionAudioLibrary::GetStrategicEventCue(EArchonStrategicAudioEvent::MatchStart));
	return true;
}

#endif
