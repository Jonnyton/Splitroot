#include "ArchonItemShopPolicyLibrary.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonItemShopGatingTest,
	"ArchonFactory.Shop.TechGatesAndPricesDecidePurchases",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FArchonItemShopGatingTest::RunTest(const FString& Parameters)
{
	FArchonShopItemRow Crossbow;
	Crossbow.ItemId = TEXT("pressure_bolt_crossbow");
	Crossbow.BaseCost = 100;
	Crossbow.RequiredTech = TEXT("armory");

	TArray<FName> NoTech;
	TArray<FName> WithArmory;
	WithArmory.Add(TEXT("armory"));

	FArchonShopPurchaseDecision Decision =
		UArchonItemShopPolicyLibrary::EvaluatePurchase(Crossbow, 500, NoTech);
	TestFalse(TEXT("No armory, no crossbow"), Decision.bCanPurchase);
	TestEqual(TEXT("Reason names the missing tech gate"), Decision.Reason, FName(TEXT("missing_tech")));

	Decision = UArchonItemShopPolicyLibrary::EvaluatePurchase(Crossbow, 500, WithArmory);
	TestTrue(TEXT("Armory built unlocks the crossbow"), Decision.bCanPurchase);
	TestEqual(TEXT("Base price holds at 1x"), Decision.FinalCost, 100);

	Decision = UArchonItemShopPolicyLibrary::EvaluatePurchase(Crossbow, 99, WithArmory);
	TestFalse(TEXT("Tech without supply still cannot buy"), Decision.bCanPurchase);
	TestEqual(TEXT("Reason is the bank"), Decision.Reason, FName(TEXT("insufficient_supply")));

	// Soft-loss inflation (Renegade power-plant rule): prices rise,
	// the menu never zeroes.
	Decision = UArchonItemShopPolicyLibrary::EvaluatePurchase(Crossbow, 500, WithArmory, 1.5f);
	TestTrue(TEXT("Inflated but affordable still buys"), Decision.bCanPurchase);
	TestEqual(TEXT("Price inflates by the multiplier"), Decision.FinalCost, 150);
	Decision = UArchonItemShopPolicyLibrary::EvaluatePurchase(Crossbow, 500, WithArmory, 0.5f);
	TestEqual(TEXT("Multiplier never discounts below base"), Decision.FinalCost, 100);

	FArchonShopItemRow FreeBow;
	FreeBow.ItemId = TEXT("thornsprout_bow");
	FreeBow.BaseCost = 0;
	FreeBow.RequiredTech = NAME_None;
	Decision = UArchonItemShopPolicyLibrary::EvaluatePurchase(FreeBow, 0, NoTech, 3.0f);
	TestTrue(TEXT("Free basics stay purchasable with zero supply and no tech"), Decision.bCanPurchase);

	FArchonShopItemRow Invalid;
	Decision = UArchonItemShopPolicyLibrary::EvaluatePurchase(Invalid, 9999, WithArmory);
	TestFalse(TEXT("Unnamed items are rejected"), Decision.bCanPurchase);

	const TArray<FArchonShopItemRow> Catalog = UArchonItemShopPolicyLibrary::BuildDefaultCatalog();
	const auto FindCatalogRow = [&Catalog](const FName ItemId) -> const FArchonShopItemRow*
	{
		for (const FArchonShopItemRow& Row : Catalog)
		{
			if (Row.ItemId == ItemId)
			{
				return &Row;
			}
		}

		return nullptr;
	};

	const auto CheckTechGatedCatalogRow =
		[this, &FindCatalogRow](
			const FName ItemId,
			int32 ExpectedBaseCost,
			const FName RequiredTech,
			EArchonShopRowKind ExpectedRowKind,
			const FName ExpectedFactionId,
			const FName ExpectedAssetCandidateId)
	{
		const FString Label = ItemId.ToString();
		const FArchonShopItemRow* Row = FindCatalogRow(ItemId);
		TestTrue(*FString::Printf(TEXT("%s exists in the default catalog"), *Label), Row != nullptr);
		if (Row == nullptr)
		{
			return;
		}

		TestEqual(*FString::Printf(TEXT("%s has the expected base cost"), *Label), Row->BaseCost, ExpectedBaseCost);
		TestEqual(*FString::Printf(TEXT("%s has the expected tech gate"), *Label), Row->RequiredTech, RequiredTech);
		TestEqual(*FString::Printf(TEXT("%s has the expected row kind"), *Label), Row->RowKind, ExpectedRowKind);
		TestEqual(*FString::Printf(TEXT("%s has the expected faction id"), *Label), Row->FactionId, ExpectedFactionId);
		TestEqual(*FString::Printf(TEXT("%s has the expected asset candidate id"), *Label), Row->AssetCandidateId, ExpectedAssetCandidateId);

		TArray<FName> MissingTech;
		FArchonShopPurchaseDecision Locked =
			UArchonItemShopPolicyLibrary::EvaluatePurchase(*Row, 9999, MissingTech);
		TestFalse(*FString::Printf(TEXT("%s is locked before its tech building exists"), *Label), Locked.bCanPurchase);
		TestEqual(*FString::Printf(TEXT("%s reports missing tech before unlock"), *Label), Locked.Reason, FName(TEXT("missing_tech")));

		TArray<FName> BuiltTech;
		BuiltTech.Add(RequiredTech);
		FArchonShopPurchaseDecision Unlocked =
			UArchonItemShopPolicyLibrary::EvaluatePurchase(*Row, ExpectedBaseCost, BuiltTech);
		TestTrue(*FString::Printf(TEXT("%s unlocks once its tech building exists"), *Label), Unlocked.bCanPurchase);
		TestEqual(*FString::Printf(TEXT("%s keeps its catalog cost at 1x"), *Label), Unlocked.FinalCost, ExpectedBaseCost);
	};

	CheckTechGatedCatalogRow(
		FName(TEXT("lenswright_pressure_runner")),
		120,
		FName(TEXT("armory")),
		EArchonShopRowKind::Unit,
		FName(TEXT("LenswrightCompact")),
		FName(TEXT("fab_rogue_character_model")));
	CheckTechGatedCatalogRow(
		FName(TEXT("kinwild_spear_hunter")),
		160,
		FName(TEXT("beast_den")),
		EArchonShopRowKind::Unit,
		FName(TEXT("KinwildCovenant")),
		FName(TEXT("fab_lizard_kin_warrior")));
	CheckTechGatedCatalogRow(
		FName(TEXT("kinwild_pack_companion")),
		80,
		FName(TEXT("beast_den")),
		EArchonShopRowKind::Unit,
		FName(TEXT("KinwildCovenant")),
		FName(TEXT("fab_low_poly_wolf")));
	CheckTechGatedCatalogRow(
		FName(TEXT("verdant_temporary_runner")),
		100,
		FName(TEXT("grove")),
		EArchonShopRowKind::Unit,
		FName(TEXT("VerdantChoir")),
		FName(TEXT("fab_fantasy_game_character")));

	TestTrue(TEXT("Default catalog has item and unit rows"),
		Catalog.Num() >= 7);

	return true;
}
