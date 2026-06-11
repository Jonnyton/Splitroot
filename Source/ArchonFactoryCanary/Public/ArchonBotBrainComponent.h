#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ArchonBotBrainComponent.generated.h"

class AArchonBaseCoreActor;
class AArchonBaseDefenseTowerActor;
class AArchonBotAIController;
class AArchonCanaryFpsCharacter;
class AArchonMatchStateActor;
class UArchonCombatHealthComponent;
class UWorld;
struct FArchonBotStrategyTuning;
struct FArchonDamageApplicationResult;

UENUM(BlueprintType)
enum class EArchonBotState : uint8
{
	MoveToObjective,
	EngageEnemy,
	Pursuing,
	HuntingThreat,
	DefendingBase,
	AttackTower,
	AttackCore,
	Dead
};

/**
 * V1 bot player brain (Jonathan 2026-06-10, decision 9): bots are REAL
 * players on programmed behavior — same body, same bow, same rules. V1:
 * run to the enemy base, fight any enemy met on the way, attack the
 * core on arrival, steer around obstacles (pure policy in
 * UArchonBotSteeringPolicyLibrary). Deepen later; replaceable by a
 * human at any time because the pawn is a standard player pawn.
 */
UCLASS(ClassGroup=(Archon), Blueprintable, meta=(BlueprintSpawnableComponent))
class ARCHONFACTORYCANARY_API UArchonBotBrainComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UArchonBotBrainComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category = "Archon|Bot")
	void ConfigureBot(int32 InTeamId, AArchonBaseCoreActor* InTargetCore, AArchonBaseCoreActor* InOwnCore, int32 InBotIndex);

	void ConfigureBotWithTuning(
		int32 InTeamId,
		AArchonBaseCoreActor* InTargetCore,
		AArchonBaseCoreActor* InOwnCore,
		int32 InBotIndex,
		const FArchonBotStrategyTuning& StrategyTuning);

	// Route waypoints walked BEFORE the enemy core (e.g. an assigned
	// resource site): spreads the teams across lanes and captures sites
	// on the way (the match state's presence scan does the rest).
	UFUNCTION(BlueprintCallable, Category = "Archon|Bot")
	void SetRouteWaypoint(const FVector& InWaypoint);

	UFUNCTION(BlueprintPure, Category = "Archon|Bot")
	EArchonBotState GetBotState() const { return BotState; }

	UFUNCTION(BlueprintPure, Category = "Archon|Bot")
	int32 GetTeamId() const { return TeamId; }

	UFUNCTION(BlueprintPure, Category = "Archon|Bot")
	int32 GetBotIndex() const { return BotIndex; }

	// A human can take any bot over on the fly (decision 9). The brain
	// sleeps while a human drives and resumes when released.
	UFUNCTION(BlueprintCallable, Category = "Archon|Bot")
	void SetHumanControlled(bool bInHumanControlled);

	UFUNCTION(BlueprintPure, Category = "Archon|Bot")
	bool IsHumanControlled() const { return bHumanControlled; }

private:
	AArchonCanaryFpsCharacter* ResolveBotCharacter() const;
	AArchonCanaryFpsCharacter* FindNearestNativePerceivedEnemy(
		const AArchonCanaryFpsCharacter& Bot,
		FVector& OutLastKnownLocation,
		FName& OutSenseTag) const;
	// Fair senses: SIGHT (vision cone + LOS) or HEARING (short radius).
	AArchonCanaryFpsCharacter* FindNearestPerceivedEnemy(const AArchonCanaryFpsCharacter& Bot) const;
	bool HasLineOfSight(const AArchonCanaryFpsCharacter& Bot, const AArchonCanaryFpsCharacter& Target) const;
	AArchonBaseDefenseTowerActor* FindNearestLivingEnemyTower(const FVector& FromLocation, float MaxRange) const;
	AArchonMatchStateActor* ResolveMatchState();
	void HandleOwnDamage(const FArchonDamageApplicationResult& DamageResult);
	void SetBotState(EArchonBotState NewState, const TCHAR* Detail);
	FVector SelectUnstickEscapeTarget(
		UWorld& World,
		const AArchonCanaryFpsCharacter& Bot,
		const FVector& BotLocation,
		const FVector& Objective,
		bool& bOutClearCandidate,
		int32& OutCandidateCount) const;
	bool TryAttackObjectiveStructure(AArchonCanaryFpsCharacter& Bot, const FVector& BotLocation);
	bool FireBowAt(
		AArchonCanaryFpsCharacter& Bot,
		const FVector& TargetLocation,
		UArchonCombatHealthComponent* DirectStructureHitTarget = nullptr,
		float DirectStructureDamageScale = 1.0f);

	UPROPERTY()
	TObjectPtr<AArchonBaseCoreActor> TargetCore;

	UPROPERTY()
	TObjectPtr<AArchonBaseCoreActor> OwnCore;

	EArchonBotState BotState = EArchonBotState::MoveToObjective;
	int32 TeamId = 0;
	int32 BotIndex = INDEX_NONE;
	float DeadSeconds = 0.0f;
	int32 Deaths = 0;
	FVector RouteWaypoint = FVector::ZeroVector;
	float MoveReissueSeconds = 0.0f;
	bool bNavMoveWorking = false;
	bool bHasRouteWaypoint = false;
	bool bRouteWaypointReached = false;
	bool bHumanControlled = false;
	bool bHasStuckSample = false;
	FName LastFiringPositionFeatureTarget = NAME_None;
	FName LastObjectiveStallFeatureTarget = NAME_None;

	// V1 tuning (hypotheses until a watched match says otherwise).
	float EngageRange = 2200.0f;
	float CoreAttackRange = 2800.0f;
	// Siege focus (2026-06-11): towers gate the cores, and bots never
	// targeted them, so every archived match stalled. Bots now raze the
	// gate tower first, then pressure the core from the same base-attack
	// envelope.
	float TowerAttackRange = 2400.0f;
	// Temporary canary siege bridge until the item-shop/tech-building
	// siege kit exists: base cores must fall in bounded all-bot proofs,
	// but body duels and tower grind stay on normal weapon numbers.
	float CoreSiegeDamageScale = 5.0f;

	// Fair senses (Jonathan 2026-06-10: bots only know what a human in
	// their seat would). Numbers are hypotheses for watched matches.
	float VisionHalfAngleDegrees = 70.0f;
	float HearingRadius = 800.0f;
	float EyesOnRadius = 2500.0f;
	float PursuitMemoryWindowSeconds = 8.0f;
	float ThreatWindowSeconds = 10.0f;
	bool bBreacher = false;

	// MEMORY: the enemy this bot is aware of and where it last saw it.
	TWeakObjectPtr<AArchonCanaryFpsCharacter> PursuedEnemy;
	TWeakObjectPtr<AArchonCanaryFpsCharacter> LastNativePerceptionLoggedTarget;
	TWeakObjectPtr<AArchonCanaryFpsCharacter> LastFallbackPerceptionLoggedTarget;
	FName LastNativePerceptionLoggedSense = NAME_None;
	FVector PursuitLastKnownLocation = FVector::ZeroVector;
	float PursuitMemorySeconds = 0.0f;

	// PAIN: where the last hit on this body came from.
	FVector ThreatLocation = FVector::ZeroVector;
	float ThreatSeconds = 0.0f;

	// HUD: last base-attack event this bot already responded to.
	double LastBaseAttackHandled = -1.0;

	UPROPERTY()
	TObjectPtr<AArchonMatchStateActor> MatchState;
	float ObstacleProbeDistance = 220.0f;
	float ObjectiveScanRange = 100000.0f;
	float TowerStandOffDistance = 1700.0f;
	float CoreStandOffDistance = 2100.0f;
	float ObjectiveLaneSpacing = 450.0f;
	float RespawnSeconds = 5.0f;
	FVector LastStuckSampleLocation = FVector::ZeroVector;
	FVector UnstickObjective = FVector::ZeroVector;
	float StuckSampleSeconds = 0.0f;
	float LowProgressSeconds = 0.0f;
	float UnstickSeconds = 0.0f;
	int32 UnstickAttempt = 0;
	float StuckSampleIntervalSeconds = 0.75f;
	float StuckMinimumProgress = 80.0f;
	float StuckTriggerSeconds = 1.5f;
	float UnstickDurationSeconds = 1.1f;
	float UnstickLateralDistance = 650.0f;
	float UnstickBackstepDistance = 250.0f;
	int32 RouteWaypointStallAttempts = 0;
	int32 ObjectiveStallAttempts = 0;
	int32 ObjectiveLaneShift = 0;
	int32 RouteStallMaxUnstickAttempts = 8;
	int32 ObjectiveStallLaneShiftAttempts = 6;
	int32 ObjectiveStallMaxLaneShift = 3;
};
