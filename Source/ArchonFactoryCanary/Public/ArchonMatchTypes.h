#pragma once

#include "CoreMinimal.h"
#include "ArchonMatchTypes.generated.h"

UENUM(BlueprintType)
enum class EArchonMatchPhase : uint8
{
	Warmup,
	Live,
	MatchEnd,
	Traveling
};

UENUM(BlueprintType)
enum class EArchonMatchWinner : uint8
{
	Undecided,
	TeamA,
	TeamB,
	Draw
};

/**
 * Tunable match constants. Values are hypotheses until a playtest frame
 * or a live match validates them; the policy functions never hardcode.
 */
USTRUCT(BlueprintType)
struct FArchonMatchConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Archon|Match")
	float WarmupSeconds = 15.0f;

	// Testing cap (Jonathan 2026-06-10): 15 minutes so background bot
	// matches always resolve and the next match starts fresh; lengthen
	// once matches are worth savoring.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Archon|Match")
	float MatchTimeLimitSeconds = 900.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Archon|Match")
	float ScoreboardSeconds = 15.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Archon|Match")
	float SupplyTickIntervalSeconds = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Archon|Match")
	int32 SupplyPerSitePerTick = 25;

	// Passive trickle so a team locked out of every site can still field
	// cheap squads eventually instead of hard-starving.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Archon|Match")
	int32 BaseSupplyPerTick = 5;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Archon|Match")
	float SiteCaptureSeconds = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Archon|Match")
	float SiteCaptureRadius = 900.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Archon|Match")
	float CoreMaxHealth = 2000.0f;

	// Tower damage feeds match points at a discount (stalemate fix,
	// 2026-06-11): every archived all-AI match ended 0-0 time-limit Draw
	// because towers gate the cores and points only counted core damage.
	// Siege pressure now scores — discounted so core damage stays the
	// real prize. 600hp tower fully razed at 0.2 = 120 pts.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Archon|Match")
	float TowerDamagePointScale = 0.2f;

	// Income loop (2026-06-11): supply now BUYS reinforcement bodies
	// (WC3: map control -> income -> army). With two sites held
	// (~55/tick) a squad fields roughly every 30s; locked out of every
	// site, roughly every 5 minutes. All values are hypotheses until
	// watched matches say otherwise.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Archon|Match")
	int32 ReinforcementSquadSupplyCost = 150;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Archon|Match")
	int32 ReinforcementSquadSize = 2;

	// Field cap; purchases at the cap burn the supply (SC-style: bank
	// management is the commander's job). Tuning hypothesis.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Archon|Match")
	int32 MaxFieldedBotsPerTeam = 12;

	// The auto-spender is a LAZY banker: it only buys once the bank
	// holds this multiple of the squad cost, leaving a window where a
	// human commander at the table (B key) spends first. AI-only teams
	// still field continuously, just half a squad behind a sharp human.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Archon|Match")
	float AutoReinforceBankMultiple = 2.0f;
};

USTRUCT(BlueprintType)
struct FArchonMatchClock
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Archon|Match")
	EArchonMatchPhase Phase = EArchonMatchPhase::Warmup;

	// Seconds spent in the current phase.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Archon|Match")
	float PhaseSeconds = 0.0f;
};

USTRUCT(BlueprintType)
struct FArchonResourceSiteState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Archon|Match")
	int32 OwningTeam = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Archon|Match")
	int32 CapturingTeam = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Archon|Match")
	float CaptureProgressSeconds = 0.0f;
};

// A base structure took an accepted hit. Fair-senses rule (Jonathan
// 2026-06-10: "ai players should not cheat"): everyone on the team
// learns WHICH building is under attack (humans get it via HUD) —
// only bots physically near the building when an arrow lands learn
// the shot origin (they watched it arrive, like a human standing
// there would).
USTRUCT(BlueprintType)
struct FArchonBaseAttackEvent
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Archon|Match")
	int32 DefendingTeam = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "Archon|Match")
	FName StructureLabel;

	UPROPERTY(BlueprintReadOnly, Category = "Archon|Match")
	FVector StructureLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Archon|Match")
	FVector ShotOrigin = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Archon|Match")
	float HealthFraction = 1.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Archon|Match")
	double EventTimeSeconds = 0.0;
};

// Wire-friendly match snapshot replicated to clients (the HUD/scoreboard
// data source). The server is the only writer; clients read via OnRep.
USTRUCT(BlueprintType)
struct FArchonMatchNetSnapshot
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Archon|Match")
	EArchonMatchPhase Phase = EArchonMatchPhase::Warmup;

	UPROPERTY(BlueprintReadOnly, Category = "Archon|Match")
	EArchonMatchWinner Winner = EArchonMatchWinner::Undecided;

	UPROPERTY(BlueprintReadOnly, Category = "Archon|Match")
	float PhaseSeconds = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Archon|Match")
	float ScoreboardSeconds = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Archon|Match")
	int32 SupplyA = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Archon|Match")
	int32 SupplyB = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Archon|Match")
	int32 PointsA = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Archon|Match")
	int32 PointsB = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Archon|Match")
	FName NextMapId;

	UPROPERTY(BlueprintReadOnly, Category = "Archon|Match")
	FString MatchEndReason;

	// Build that became effective for this match. The stamp is visible in
	// the HUD so hot-reload adoption can be checked by eye and by replay.
	UPROPERTY(BlueprintReadOnly, Category = "Archon|Match")
	FString BuildVersion;

	UPROPERTY(BlueprintReadOnly, Category = "Archon|Match")
	FString BuildEffectiveUtc;

	UPROPERTY(BlueprintReadOnly, Category = "Archon|Match")
	FString BuildModuleUtc;

	UPROPERTY(BlueprintReadOnly, Category = "Archon|Match")
	FString BuildRuntimeUtc;

	// Compact "BASE UNDER ATTACK" readouts (empty = no recent attack).
	// Built server-side from the attack ledger; the HUD just prints.
	UPROPERTY(BlueprintReadOnly, Category = "Archon|Match")
	FString BaseAlertA;

	UPROPERTY(BlueprintReadOnly, Category = "Archon|Match")
	FString BaseAlertB;
};
