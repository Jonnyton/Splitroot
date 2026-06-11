#pragma once

#include "CoreMinimal.h"
#include "ArchonItemShopTypes.generated.h"

UENUM(BlueprintType)
enum class EArchonShopRowKind : uint8
{
	Item,
	Unit,
	Squad
};

/**
 * One shared body, an item shop that the commander's tech buildings
 * expand (Jonathan 2026-06-10; Renegade purchase-terminal + Savage 2
 * tech-gating lineage — see wiki
 * splitroot-item-shop-tech-building-direction-2026-06-11).
 * Tech is FName-keyed so other factory games can remix the tree.
 */
USTRUCT(BlueprintType)
struct FArchonShopItemRow
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Archon|Shop")
	FName ItemId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Archon|Shop")
	EArchonShopRowKind RowKind = EArchonShopRowKind::Item;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Archon|Shop")
	FName FactionId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Archon|Shop")
	FName AssetCandidateId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Archon|Shop")
	int32 BaseCost = 0;

	// NAME_None = always available (the Renegade free-classes rule:
	// a team with every building dead can still field basic bodies).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Archon|Shop")
	FName RequiredTech;
};

USTRUCT(BlueprintType)
struct FArchonShopPurchaseDecision
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Archon|Shop")
	bool bCanPurchase = false;

	// Base cost after the soft-loss inflation multiplier (Renegade
	// power-plant rule: losing economy buildings raises prices, it
	// never zeroes the menu).
	UPROPERTY(BlueprintReadOnly, Category = "Archon|Shop")
	int32 FinalCost = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Archon|Shop")
	FName Reason;
};
