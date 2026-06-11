#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ArchonMatchTypes.h"
#include "ArchonMatchPolicyLibrary.generated.h"

/**
 * Pure match-lifecycle authority: phase machine, win evaluation, resource
 * site capture, and supply income as deterministic functions. Runtime
 * actors feed in world facts (core health, presence counts, elapsed time)
 * and apply the returned state; no policy decision lives in actor code.
 *
 * Match flow (Renegade-pattern, SPLITROOT rules):
 *   Warmup -> Live -> MatchEnd (scoreboard) -> Traveling (next map)
 * Win: enemy base core destroyed; at the time limit, points decide.
 */
UCLASS()
class ARCHONFACTORYCANARY_API UArchonMatchPolicyLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// Advances the phase machine. The winner verdict is an input (use
	// EvaluateWinner) so the clock stays a pure function of its arguments.
	UFUNCTION(BlueprintPure, Category = "Archon|Match")
	static FArchonMatchClock TickMatchClock(
		const FArchonMatchClock& Clock,
		float DeltaSeconds,
		const FArchonMatchConfig& Config,
		EArchonMatchWinner Winner);

	// Win: core destruction outranks everything. At the bell: points,
	// then sites held (stalemate tiebreak, 2026-06-11), then Draw.
	UFUNCTION(BlueprintPure, Category = "Archon|Match")
	static EArchonMatchWinner EvaluateWinner(
		float CoreHealthTeamA,
		float CoreHealthTeamB,
		bool bTimeLimitExpired,
		int32 PointsTeamA,
		int32 PointsTeamB,
		int32 SitesOwnedTeamA = 0,
		int32 SitesOwnedTeamB = 0);

	// Discounted points for damaging defense towers (see
	// FArchonMatchConfig::TowerDamagePointScale).
	UFUNCTION(BlueprintPure, Category = "Archon|Match")
	static int32 ComputeTowerDamagePoints(float DamageApplied, const FArchonMatchConfig& Config);

	// Income loop: can this supply bank field a reinforcement squad?
	// A non-positive cost disables purchasing outright.
	UFUNCTION(BlueprintPure, Category = "Archon|Match")
	static bool CanAffordReinforcement(int32 Supply, const FArchonMatchConfig& Config);

	// The lazy-banker rule for the AUTO spender (manual buys use
	// CanAffordReinforcement directly): only buy once the bank holds
	// AutoReinforceBankMultiple x cost.
	UFUNCTION(BlueprintPure, Category = "Archon|Match")
	static bool ShouldAutoReinforce(int32 Supply, const FArchonMatchConfig& Config);

	// Presence-based capture per the v0 spec: hold the zone long enough to
	// flip it. Contested freezes progress; abandoning decays it; ownership
	// persists while the site is empty.
	UFUNCTION(BlueprintPure, Category = "Archon|Match")
	static FArchonResourceSiteState TickSiteCapture(
		const FArchonResourceSiteState& Site,
		int32 PresentTeamA,
		int32 PresentTeamB,
		float DeltaSeconds,
		const FArchonMatchConfig& Config);

	UFUNCTION(BlueprintPure, Category = "Archon|Match")
	static int32 ComputeSupplyPerTick(int32 SitesOwned, const FArchonMatchConfig& Config);

	UFUNCTION(BlueprintPure, Category = "Archon|Match")
	static bool IsLocationInsideSite(
		const FVector& Location,
		const FVector& SiteLocation,
		const FArchonMatchConfig& Config);
};
