#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ArchonRenownPolicyLibrary.generated.h"

USTRUCT(BlueprintType)
struct FArchonRenownTickResult
{
	GENERATED_BODY()

	// Accumulated fractional seconds carried to the next tick.
	UPROPERTY(BlueprintReadOnly, Category = "Archon|Renown")
	float NewAccumulatorSeconds = 0.0f;

	// Whole Renown paid out this tick (0 most frames).
	UPROPERTY(BlueprintReadOnly, Category = "Archon|Renown")
	int32 Payout = 0;
};

/**
 * Pure policy for the personal wallet (decision 1, Jonathan-ratified):
 * Renown is RTS-driven, Renegade-style — (a) a slow passive trickle for
 * every player, always; (b) harvester-return payouts (event-driven, fed
 * in by the income actor later); (c) deed bounties on top. Personal
 * money never crosses into the RTS pool and never crosses team lines
 * (team switch = wipe). Pure functions so every number is testable
 * before a single UI shows it.
 */
UCLASS()
class ARCHONFACTORYCANARY_API UArchonRenownPolicyLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// Advance the passive trickle: pays TricklePerInterval once per full
	// IntervalSeconds, carrying fractional time without drift.
	UFUNCTION(BlueprintPure, Category = "Archon|Renown")
	static FArchonRenownTickResult EvaluateTrickle(
		float AccumulatorSeconds,
		float DeltaSeconds,
		float IntervalSeconds,
		int32 TricklePerInterval);

	// Deed bounty for a kill (flat v1; hero/value scaling later).
	UFUNCTION(BlueprintPure, Category = "Archon|Renown")
	static int32 GetKillBounty();

	// Renegade pattern: every teammate is paid when the harvester
	// -equivalent docks at the base.
	UFUNCTION(BlueprintPure, Category = "Archon|Renown")
	static int32 GetHarvestPayout();

	static constexpr float DefaultTrickleIntervalSeconds = 10.0f;
	static constexpr int32 DefaultTricklePerInterval = 5;
};
