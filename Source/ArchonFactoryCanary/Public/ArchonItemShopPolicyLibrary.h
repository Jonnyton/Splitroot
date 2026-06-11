#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ArchonItemShopTypes.h"
#include "ArchonItemShopPolicyLibrary.generated.h"

/**
 * Pure decisions for the item shop. Runtime surfaces (purchase points,
 * the table UI, bots buying gear) feed in world facts; no pricing or
 * gating decision lives in actor code.
 */
UCLASS()
class ARCHONFACTORYCANARY_API UArchonItemShopPolicyLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Archon|Shop")
	static FArchonShopPurchaseDecision EvaluatePurchase(
		const FArchonShopItemRow& Item,
		int32 SupplyAvailable,
		const TArray<FName>& BuiltTech,
		float PriceMultiplier = 1.0f);

	// The v0 catalog: current free basics plus first-pass unit/body
	// rows gated by commander tech buildings. Grows with content;
	// rows are data, not logic.
	UFUNCTION(BlueprintPure, Category = "Archon|Shop")
	static TArray<FArchonShopItemRow> BuildDefaultCatalog();
};
