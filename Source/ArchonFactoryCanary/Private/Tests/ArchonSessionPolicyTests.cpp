#if WITH_DEV_AUTOMATION_TESTS

#include "ArchonSessionPolicyLibrary.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonSteamOnlineFreePreviewOnlyTest,
	"ArchonFactory.SessionPolicy.SteamOnlineFreePreviewOnly",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonSteamOnlineFreePreviewOnlyTest::RunTest(const FString& Parameters)
{
	const FArchonEffectiveAccess Access = UArchonSessionPolicyLibrary::MakeEffectiveAccess(
		EArchonSessionRoute::SteamOnline,
		false,
		false);

	TestTrue(TEXT("Core combat remains free online"), Access.bCoreCombat);
	TestTrue(TEXT("Respawn remains free online"), Access.bRespawn);
	TestTrue(TEXT("Default heroes remain free online"), Access.bDefaultHeroes);
	TestTrue(TEXT("Map table preview remains free online"), Access.bMapTablePreview);
	TestFalse(TEXT("Live RTS execution is paid online"), Access.bLiveRtsExecution);
	TestTrue(TEXT("Free online map table is preview-only"), UArchonSessionPolicyLibrary::IsMapTablePreviewOnly(EArchonSessionRoute::SteamOnline, false));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonLocalRoutesFullFreeTest,
	"ArchonFactory.SessionPolicy.LocalRoutesFullFree",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonLocalRoutesFullFreeTest::RunTest(const FString& Parameters)
{
	const EArchonSessionRoute Routes[] = {
		EArchonSessionRoute::OfflineSkirmish,
		EArchonSessionRoute::LANHosted,
		EArchonSessionRoute::PrivateHost
	};

	for (const EArchonSessionRoute Route : Routes)
	{
		const FArchonEffectiveAccess Access = UArchonSessionPolicyLibrary::MakeEffectiveAccess(Route, false, false);
		TestTrue(TEXT("Local route is full-free"), Access.bAllHeroes);
		TestTrue(TEXT("Local route includes live RTS execution"), Access.bLiveRtsExecution);
		TestFalse(TEXT("Local route is not preview-only"), UArchonSessionPolicyLibrary::IsMapTablePreviewOnly(Route, false));
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonPaidHeroesHorizontalOnlyTest,
	"ArchonFactory.SessionPolicy.PaidHeroesHorizontalOnly",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonPaidHeroesHorizontalOnlyTest::RunTest(const FString& Parameters)
{
	FArchonHeroEntitlement PaidHero;
	PaidHero.HeroId = TEXT("paid_support_variant");
	PaidHero.bIsDefaultHero = false;
	PaidHero.bIsHorizontalPaidHero = true;

	TestFalse(TEXT("Paid hero locked without entitlement"), UArchonSessionPolicyLibrary::CanSpawnHero(EArchonSessionRoute::SteamOnline, PaidHero, false));
	TestTrue(TEXT("Paid hero unlocks with entitlement"), UArchonSessionPolicyLibrary::CanSpawnHero(EArchonSessionRoute::SteamOnline, PaidHero, true));

	PaidHero.bImprovesCombatPower = true;
	TestFalse(TEXT("Strict combat upgrades are rejected"), UArchonSessionPolicyLibrary::CanSpawnHero(EArchonSessionRoute::SteamOnline, PaidHero, true));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonAmbiguousOnlineFallbackTest,
	"ArchonFactory.SessionPolicy.AmbiguousOnlineFallback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonAmbiguousOnlineFallbackTest::RunTest(const FString& Parameters)
{
	TestEqual(
		TEXT("Ambiguous online route falls back to SteamOnline"),
		UArchonSessionPolicyLibrary::ResolveAmbiguousOnlineFallback(),
		EArchonSessionRoute::SteamOnline);

	TestEqual(
		TEXT("Unknown route names resolve conservatively to SteamOnline"),
		UArchonSessionPolicyLibrary::ResolveRouteFromName(TEXT("internet_unknown")),
		EArchonSessionRoute::SteamOnline);
	return true;
}

#endif
