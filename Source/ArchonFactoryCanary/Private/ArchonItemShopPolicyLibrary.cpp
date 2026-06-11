#include "ArchonItemShopPolicyLibrary.h"

FArchonShopPurchaseDecision UArchonItemShopPolicyLibrary::EvaluatePurchase(
	const FArchonShopItemRow& Item,
	int32 SupplyAvailable,
	const TArray<FName>& BuiltTech,
	float PriceMultiplier)
{
	FArchonShopPurchaseDecision Decision;

	if (Item.ItemId.IsNone())
	{
		Decision.Reason = TEXT("invalid_item");
		return Decision;
	}

	if (!Item.RequiredTech.IsNone() && !BuiltTech.Contains(Item.RequiredTech))
	{
		Decision.Reason = TEXT("missing_tech");
		return Decision;
	}

	const float SafeMultiplier = FMath::Max(1.0f, PriceMultiplier);
	Decision.FinalCost = FMath::CeilToInt(FMath::Max(0, Item.BaseCost) * SafeMultiplier);

	if (SupplyAvailable < Decision.FinalCost)
	{
		Decision.Reason = TEXT("insufficient_supply");
		return Decision;
	}

	Decision.bCanPurchase = true;
	Decision.Reason = TEXT("accepted");
	return Decision;
}

TArray<FArchonShopItemRow> UArchonItemShopPolicyLibrary::BuildDefaultCatalog()
{
	// v0 rows: free basics keep a fully-razed team in the fight
	// (Renegade rule); tech-built rows become the first body/loadout
	// slots for the one-body item-shop direction.
	TArray<FArchonShopItemRow> Catalog;

	FArchonShopItemRow Bow;
	Bow.ItemId = TEXT("thornsprout_bow");
	Bow.RowKind = EArchonShopRowKind::Item;
	Bow.FactionId = TEXT("VerdantChoir");
	Bow.BaseCost = 0;
	Bow.RequiredTech = NAME_None;
	Catalog.Add(Bow);

	FArchonShopItemRow Crossbow;
	Crossbow.ItemId = TEXT("pressure_bolt_crossbow");
	Crossbow.RowKind = EArchonShopRowKind::Item;
	Crossbow.FactionId = TEXT("LenswrightCompact");
	Crossbow.BaseCost = 100;
	Crossbow.RequiredTech = TEXT("armory");
	Catalog.Add(Crossbow);

	FArchonShopItemRow ReinforcementSquad;
	ReinforcementSquad.ItemId = TEXT("reinforcement_squad");
	ReinforcementSquad.RowKind = EArchonShopRowKind::Squad;
	ReinforcementSquad.BaseCost = 150;
	ReinforcementSquad.RequiredTech = NAME_None;
	Catalog.Add(ReinforcementSquad);

	FArchonShopItemRow LenswrightPressureRunner;
	LenswrightPressureRunner.ItemId = TEXT("lenswright_pressure_runner");
	LenswrightPressureRunner.RowKind = EArchonShopRowKind::Unit;
	LenswrightPressureRunner.FactionId = TEXT("LenswrightCompact");
	LenswrightPressureRunner.AssetCandidateId = TEXT("fab_rogue_character_model");
	LenswrightPressureRunner.BaseCost = 120;
	LenswrightPressureRunner.RequiredTech = TEXT("armory");
	Catalog.Add(LenswrightPressureRunner);

	FArchonShopItemRow KinwildSpearHunter;
	KinwildSpearHunter.ItemId = TEXT("kinwild_spear_hunter");
	KinwildSpearHunter.RowKind = EArchonShopRowKind::Unit;
	KinwildSpearHunter.FactionId = TEXT("KinwildCovenant");
	KinwildSpearHunter.AssetCandidateId = TEXT("fab_lizard_kin_warrior");
	KinwildSpearHunter.BaseCost = 160;
	KinwildSpearHunter.RequiredTech = TEXT("beast_den");
	Catalog.Add(KinwildSpearHunter);

	FArchonShopItemRow KinwildPackCompanion;
	KinwildPackCompanion.ItemId = TEXT("kinwild_pack_companion");
	KinwildPackCompanion.RowKind = EArchonShopRowKind::Unit;
	KinwildPackCompanion.FactionId = TEXT("KinwildCovenant");
	KinwildPackCompanion.AssetCandidateId = TEXT("fab_low_poly_wolf");
	KinwildPackCompanion.BaseCost = 80;
	KinwildPackCompanion.RequiredTech = TEXT("beast_den");
	Catalog.Add(KinwildPackCompanion);

	FArchonShopItemRow VerdantTemporaryRunner;
	VerdantTemporaryRunner.ItemId = TEXT("verdant_temporary_runner");
	VerdantTemporaryRunner.RowKind = EArchonShopRowKind::Unit;
	VerdantTemporaryRunner.FactionId = TEXT("VerdantChoir");
	VerdantTemporaryRunner.AssetCandidateId = TEXT("fab_fantasy_game_character");
	VerdantTemporaryRunner.BaseCost = 100;
	VerdantTemporaryRunner.RequiredTech = TEXT("grove");
	Catalog.Add(VerdantTemporaryRunner);

	return Catalog;
}
