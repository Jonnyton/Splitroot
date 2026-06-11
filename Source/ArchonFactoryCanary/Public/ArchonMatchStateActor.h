#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ArchonMapRegistryLibrary.h"
#include "ArchonMatchTypes.h"
#include "ArchonValleyLayoutTypes.h"
#include "ArchonMatchStateActor.generated.h"

class AArchonBaseCoreActor;
class AArchonBaseDefenseTowerActor;
struct FArchonDamageApplicationResult;

// Income loop: a team's supply bank just bought a reinforcement squad;
// the world subsystem (the only thing that knows how to field bodies)
// listens and spawns.
DECLARE_MULTICAST_DELEGATE_TwoParams(FArchonReinforcementPurchased, int32 /*TeamId*/, int32 /*SquadSize*/);

// Tech tree: a commander bought a tech building; the subsystem places
// the actor. Function-loss (Renegade rule): when the building dies its
// tech leaves the team's shop.
DECLARE_MULTICAST_DELEGATE_TwoParams(FArchonTechBuildingPurchased, int32 /*TeamId*/, FName /*TechId*/);

/**
 * Runtime owner of one match: ticks real frames into the pure
 * UArchonMatchPolicyLibrary functions and applies the results. Emits
 * `ArchonFactoryCanary:` log markers (MatchPhase / SiteCaptured /
 * SupplyTick / MatchEnd / TravelRequested) that the match-loop smoke
 * asserts. All decisions live in the policy library; this actor only
 * gathers world facts and carries state between frames.
 *
 * Proof mode (`-ArchonMatchProof`) shortens the clock and scripts a
 * deterministic arc: warmup ends, the player is moved onto the central
 * site, the site flips, supply ticks, the enemy core is destroyed, and
 * the scoreboard runs out. Plain proof quits at the first travel boundary;
 * restart/rotation proofs only quit after travel when the launch URL
 * explicitly carries ArchonMatchProofQuitAfterTravel.
 */
UCLASS()
class ARCHONFACTORYCANARY_API AArchonMatchStateActor : public AActor
{
	GENERATED_BODY()

public:
	AArchonMatchStateActor();

	void InitializeMatch(
		const FArchonMatchConfig& InConfig,
		const TArray<FArchonValleyPlacement>& SitePlacements,
		AArchonBaseCoreActor* InTeamACore,
		AArchonBaseCoreActor* InTeamBCore,
		bool bInProofScript);

	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure, Category = "Archon|Match")
	FArchonMatchNetSnapshot GetNetSnapshot() const { return NetSnapshot; }

	UFUNCTION(BlueprintPure, Category = "Archon|Match")
	FArchonMatchClock GetMatchClock() const { return MatchClock; }

	UFUNCTION(BlueprintPure, Category = "Archon|Match")
	EArchonMatchWinner GetWinner() const { return Winner; }

	UFUNCTION(BlueprintPure, Category = "Archon|Match")
	int32 GetTeamSupply(int32 InTeamId) const;

	UFUNCTION(BlueprintPure, Category = "Archon|Match")
	int32 GetTeamPoints(int32 InTeamId) const;

	UFUNCTION(BlueprintPure, Category = "Archon|Match")
	int32 GetSitesOwnedByTeam(int32 InTeamId) const;

	// Read-only site access for presentation surfaces (the map table
	// miniature board reads ownership + position).
	UFUNCTION(BlueprintPure, Category = "Archon|Match")
	int32 GetSiteCount() const { return Sites.Num(); }

	UFUNCTION(BlueprintPure, Category = "Archon|Match")
	bool GetSiteInfo(int32 SiteIndex, FVector& OutLocation, int32& OutOwningTeam) const;

	// Towers report accepted damage here; the attacker scores discounted
	// points (stalemate fix, 2026-06-11) and the hit lands in the
	// base-attack ledger (fair-senses HUD/defense, 2026-06-11).
	void NotifyTowerDamaged(AArchonBaseDefenseTowerActor* Tower, const FArchonDamageApplicationResult& DamageResult);

	// Latest base-attack event for a team within MaxAgeSeconds. Bots and
	// HUD read this; per the fair-senses doctrine the ShotOrigin field
	// is only acted on by bots that pass the eyes-on radius check.
	bool GetLatestBaseAttack(int32 DefendingTeam, double MaxAgeSeconds, FArchonBaseAttackEvent& OutEvent) const;

	FArchonReinforcementPurchased OnReinforcementPurchased;

	// Spend the team bank on a reinforcement squad NOW (the commander's
	// B-key at the table). Returns false outside Live or under-bank.
	bool TryPurchaseReinforcement(int32 TeamId);

	// Commander build verb (one-body item-shop direction, 2026-06-11):
	// buy a tech building through the shop policy (affordability +
	// dedup), debit the bank, broadcast for the subsystem to place.
	bool TryPurchaseTechBuilding(int32 TeamId, FName TechId, int32 Cost);

	// Function-loss bookkeeping. The shop reads GetBuiltTech.
	void AddBuiltTech(int32 TeamId, FName TechId);
	void RemoveBuiltTech(int32 TeamId, FName TechId);
	TArray<FName> GetBuiltTech(int32 TeamId) const;

	FArchonTechBuildingPurchased OnTechBuildingPurchased;

private:
	void ScanSitePresence(int32 SiteIndex, int32& OutPresentTeamA, int32& OutPresentTeamB) const;
	void TickSites(float DeltaSeconds);
	void TickSupply(float DeltaSeconds);
	void TickAutoReinforce();
	EArchonMatchWinner EvaluateCurrentWinner() const;
	void HandlePhaseChange(EArchonMatchPhase OldPhase, EArchonMatchPhase NewPhase);
	FArchonMapEntry ResolveNextMapEntry(const UWorld* World) const;
	void RunProofScript();
	void HandleCoreDamaged(int32 DamagedCoreTeam, const FArchonDamageApplicationResult& DamageResult);
	void RecordBaseAttack(int32 DefendingTeam, FName StructureLabel, const FVector& StructureLocation, const FVector& ShotOrigin, float HealthFraction);
	void RefreshNetSnapshot();

	UFUNCTION()
	void OnRep_NetSnapshot();

	FArchonMatchConfig Config;
	FArchonMatchClock MatchClock;
	EArchonMatchWinner Winner = EArchonMatchWinner::Undecided;

	UPROPERTY(ReplicatedUsing = OnRep_NetSnapshot)
	FArchonMatchNetSnapshot NetSnapshot;

	// Client-side: last phase already logged by OnRep, so the marker fires
	// once per phase instead of once per snapshot delta.
	uint8 LastClientLoggedPhase = 0xFF;

	UPROPERTY()
	TObjectPtr<AArchonBaseCoreActor> TeamACore;

	UPROPERTY()
	TObjectPtr<AArchonBaseCoreActor> TeamBCore;

	TArray<FArchonValleyPlacement> Sites;
	TArray<FArchonResourceSiteState> SiteStates;

	FName PendingNextMapId;
	FString MatchEndReason;

	// Ring of recent structure hits (pruned on write, ~12s window).
	TArray<FArchonBaseAttackEvent> RecentBaseAttacks;

	// Tech the team currently OWNS A LIVING BUILDING for.
	TArray<FName> BuiltTech[2];

	int32 TeamSupply[2] = { 0, 0 };
	int32 TeamPoints[2] = { 0, 0 };
	float SupplyAccumulatorSeconds = 0.0f;
	float LiveSeconds = 0.0f;

	FString EffectiveBuildVersion;
	FString EffectiveBuildUtc;
	FString EffectiveBuildModuleUtc;
	FString EffectiveBuildRuntimeUtc;

	bool bInitialized = false;
	bool bProofScript = false;
	bool bProofPlayerPlaced = false;
	bool bProofCoreKillIssued = false;
	bool bAnySupplyTickLogged = false;
	bool bTravelRequested = false;
};
