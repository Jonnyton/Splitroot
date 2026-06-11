#if WITH_DEV_AUTOMATION_TESTS

#include "ArchonItemShopPolicyLibrary.h"
#include "ArchonMatchStateActor.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonMatchTechStateFeedsShopPolicyTest,
	"ArchonFactory.Match.TechStateFeedsShopPolicy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonMatchTechStateFeedsShopPolicyTest::RunTest(const FString& Parameters)
{
	AArchonMatchStateActor* MatchState = NewObject<AArchonMatchStateActor>();

	FArchonShopItemRow Crossbow;
	Crossbow.ItemId = TEXT("pressure_bolt_crossbow");
	Crossbow.BaseCost = 100;
	Crossbow.RequiredTech = TEXT("armory");

	FArchonShopPurchaseDecision Decision = UArchonItemShopPolicyLibrary::EvaluatePurchase(
		Crossbow,
		500,
		MatchState->GetBuiltTech(0));
	TestFalse(TEXT("Crossbow starts locked without armory"), Decision.bCanPurchase);
	TestEqual(TEXT("Lock reason is missing tech"), Decision.Reason, FName(TEXT("missing_tech")));

	MatchState->AddBuiltTech(0, TEXT("armory"));
	Decision = UArchonItemShopPolicyLibrary::EvaluatePurchase(
		Crossbow,
		500,
		MatchState->GetBuiltTech(0));
	TestTrue(TEXT("Armory tech unlocks the crossbow row"), Decision.bCanPurchase);
	TestEqual(TEXT("Armory is tracked once"), MatchState->GetBuiltTech(0).Num(), 1);

	MatchState->AddBuiltTech(0, TEXT("armory"));
	TestEqual(TEXT("Adding the same tech is idempotent"), MatchState->GetBuiltTech(0).Num(), 1);

	MatchState->RemoveBuiltTech(0, TEXT("armory"));
	Decision = UArchonItemShopPolicyLibrary::EvaluatePurchase(
		Crossbow,
		500,
		MatchState->GetBuiltTech(0));
	TestFalse(TEXT("Removing armory locks the crossbow row again"), Decision.bCanPurchase);
	TestEqual(TEXT("Function-loss reason is missing tech"), Decision.Reason, FName(TEXT("missing_tech")));

	FArchonShopItemRow SpearHunter;
	SpearHunter.ItemId = TEXT("kinwild_spear_hunter");
	SpearHunter.BaseCost = 160;
	SpearHunter.RequiredTech = TEXT("beast_den");

	Decision = UArchonItemShopPolicyLibrary::EvaluatePurchase(
		SpearHunter,
		500,
		MatchState->GetBuiltTech(0));
	TestFalse(TEXT("Kinwild spear hunter starts locked without Beast Den"), Decision.bCanPurchase);
	MatchState->AddBuiltTech(0, TEXT("beast_den"));
	Decision = UArchonItemShopPolicyLibrary::EvaluatePurchase(
		SpearHunter,
		500,
		MatchState->GetBuiltTech(0));
	TestTrue(TEXT("Beast Den tech unlocks the Kinwild spear hunter row"), Decision.bCanPurchase);

	FArchonShopItemRow VerdantRunner;
	VerdantRunner.ItemId = TEXT("verdant_temporary_runner");
	VerdantRunner.BaseCost = 100;
	VerdantRunner.RequiredTech = TEXT("grove");

	Decision = UArchonItemShopPolicyLibrary::EvaluatePurchase(
		VerdantRunner,
		500,
		MatchState->GetBuiltTech(0));
	TestFalse(TEXT("Verdant runner starts locked without Grove"), Decision.bCanPurchase);
	MatchState->AddBuiltTech(0, TEXT("grove"));
	Decision = UArchonItemShopPolicyLibrary::EvaluatePurchase(
		VerdantRunner,
		500,
		MatchState->GetBuiltTech(0));
	TestTrue(TEXT("Grove tech unlocks the Verdant runner row"), Decision.bCanPurchase);

	return true;
}

#endif
