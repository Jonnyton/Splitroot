#include "ArchonBotBrainComponent.h"
#include "ArchonBaseCoreActor.h"
#include "ArchonBaseDefenseTowerActor.h"
#include "ArchonBotAIController.h"
#include "ArchonBotFeatureTelemetry.h"
#include "ArchonBotPerceptionPolicyLibrary.h"
#include "ArchonBotStrategyTuningLibrary.h"
#include "ArchonBotSteeringPolicyLibrary.h"
#include "ArchonMatchStateActor.h"
#include "ArchonCanaryFpsCharacter.h"
#include "ArchonCombatHealthComponent.h"
#include "ArchonCombatPolicyLibrary.h"
#include "ArchonFactionPaletteLibrary.h"
#include "ArchonFactionTypes.h"
#include "ArchonLenswrightPressureBoltCrossbow.h"
#include "ArchonVerdantThornsproutBow.h"
#include "AIController.h"
#include "AITypes.h"
#include "Camera/CameraComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"

UArchonBotBrainComponent::UArchonBotBrainComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UArchonBotBrainComponent::ConfigureBot(int32 InTeamId, AArchonBaseCoreActor* InTargetCore, AArchonBaseCoreActor* InOwnCore, int32 InBotIndex)
{
	ConfigureBotWithTuning(
		InTeamId,
		InTargetCore,
		InOwnCore,
		InBotIndex,
		UArchonBotStrategyTuningLibrary::GetSplitrootBotStrategyTuning());
}

void UArchonBotBrainComponent::ConfigureBotWithTuning(
	int32 InTeamId,
	AArchonBaseCoreActor* InTargetCore,
	AArchonBaseCoreActor* InOwnCore,
	int32 InBotIndex,
	const FArchonBotStrategyTuning& StrategyTuning)
{
	TeamId = InTeamId;
	TargetCore = InTargetCore;
	OwnCore = InOwnCore;
	BotIndex = InBotIndex;
	if (TeamId == 0 && BotIndex == 0)
	{
		ArchonBotFeatureTelemetry::ResetCoverage();
	}

	// Role mix (first watched 8v8, 2026-06-10: a 2200 aggro radius made an
	// eternal midfield meatgrinder — 899 events, zero core damage). Most
	// bots are FIGHTERS with a moderate radius; every third is a BREACHER
	// that ignores everything not in its face and pushes the core.
	const FName BotRole = UArchonBotStrategyTuningLibrary::GetRoleForBotIndex(BotIndex, StrategyTuning);
	bBreacher = BotRole == FName(TEXT("breacher"));
	EngageRange = bBreacher ? StrategyTuning.BreacherEngageRange : StrategyTuning.FighterEngageRange;
	CoreAttackRange = StrategyTuning.CoreAttackRange;
	TowerAttackRange = StrategyTuning.TowerAttackRange;
	VisionHalfAngleDegrees = StrategyTuning.VisionHalfAngleDegrees;
	HearingRadius = StrategyTuning.HearingRadius;
	EyesOnRadius = StrategyTuning.EyesOnRadius;
	PursuitMemoryWindowSeconds = bBreacher
		? StrategyTuning.BreacherPursuitMemoryWindowSeconds
		: StrategyTuning.PursuitMemoryWindowSeconds;
	ThreatWindowSeconds = bBreacher
		? StrategyTuning.BreacherThreatWindowSeconds
		: StrategyTuning.ThreatWindowSeconds;
	TowerStandOffDistance = StrategyTuning.TowerStandOffDistance;
	CoreStandOffDistance = StrategyTuning.CoreStandOffDistance;
	ObjectiveLaneSpacing = StrategyTuning.ObjectiveLaneSpacing;
	RespawnSeconds = StrategyTuning.RespawnSeconds;
	StuckSampleIntervalSeconds = StrategyTuning.StuckSampleIntervalSeconds;
	StuckMinimumProgress = StrategyTuning.StuckMinimumProgress;
	StuckTriggerSeconds = StrategyTuning.StuckTriggerSeconds;
	UnstickDurationSeconds = StrategyTuning.UnstickDurationSeconds;
	UnstickLateralDistance = StrategyTuning.UnstickLateralDistance;
	UnstickBackstepDistance = StrategyTuning.UnstickBackstepDistance;
	RouteStallMaxUnstickAttempts = StrategyTuning.RouteStallMaxUnstickAttempts;
	ObjectiveStallLaneShiftAttempts = StrategyTuning.ObjectiveStallLaneShiftAttempts;
	ObjectiveStallMaxLaneShift = StrategyTuning.ObjectiveStallMaxLaneShift;
	bHasStuckSample = false;
	LastStuckSampleLocation = FVector::ZeroVector;
	StuckSampleSeconds = 0.0f;
	LowProgressSeconds = 0.0f;
	UnstickSeconds = 0.0f;
	UnstickAttempt = 0;
	RouteWaypointStallAttempts = 0;
	ObjectiveStallAttempts = 0;
	ObjectiveLaneShift = 0;
	LastObjectiveStallFeatureTarget = NAME_None;
	LastFiringPositionFeatureTarget = NAME_None;
	UE_LOG(
		LogTemp,
		Display,
		TEXT("ArchonFactoryCanary: BotConfigured bot=%d team=%d role=%s engageRange=%.0f pursuitMemory=%.1f threatWindow=%.1f tuningRevision=%s"),
		BotIndex,
		TeamId,
		*BotRole.ToString(),
		EngageRange,
		PursuitMemoryWindowSeconds,
		ThreatWindowSeconds,
		*StrategyTuning.Revision);

	if (AArchonCanaryFpsCharacter* Bot = ResolveBotCharacter())
	{
		// Bots own their facing (no player controller rotation), and
		// orient to their nav path while marching.
		Bot->bUseControllerRotationYaw = false;
		if (UCharacterMovementComponent* Movement = Bot->GetCharacterMovement())
		{
			Movement->bOrientRotationToMovement = true;
		}
		if (UArchonCombatHealthComponent* Health = Bot->GetHealth())
		{
			Health->ConfigureHealth(TeamId, Health->GetMaxHealth(), Health->GetArmorModifier());
			// PAIN sense: being hit grants direction toward the shooter.
			Health->OnHealthChanged.AddUObject(this, &UArchonBotBrainComponent::HandleOwnDamage);
		}
		if (UArchonVerdantThornsproutBow* Bow = Bot->GetVerdantBow())
		{
			// Faction loadout (ship-gap #1, 2026-06-11): team 1 IS
			// Lenswright — its bodies fight with the pressure-bolt
			// crossbow profile (the weapon component is stats-driven;
			// the crossbow CDO carries the contract-tested defaults).
			// Team 0 keeps the Verdant thornsprout bow. Distinct fire
			// cycles + damage = matches read as two factions, not two
			// colors.
			const FArchonWeaponStats FactionStats =
				TeamId == 1
					? GetDefault<UArchonLenswrightPressureBoltCrossbow>()->GetStats()
					: Bow->GetStats();
			Bow->ConfigureWeapon(TeamId, BotIndex, FactionStats);
			UE_LOG(
				LogTemp,
				Display,
				TEXT("ArchonFactoryCanary: BotLoadout bot=%d team=%d weapon=%s"),
				BotIndex,
				TeamId,
				TeamId == 1 ? TEXT("pressure_bolt_crossbow") : TEXT("thornsprout_bow"));
		}
		// Team identity readable from every view (current dress: team 0
		// Verdant green, team 1 Lenswright brass).
		Bot->SetBodyTeamColor(UArchonFactionPaletteLibrary::GetFactionColor(
			TeamId == 0 ? EArchonFaction::VerdantChoir : EArchonFaction::LenswrightCompact,
			EArchonFactionPaletteSlot::Primary));
	}
}

void UArchonBotBrainComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	AArchonCanaryFpsCharacter* Bot = ResolveBotCharacter();
	UWorld* World = GetWorld();
	if (!Bot || !World || !Bot->HasAuthority() || bHumanControlled)
	{
		return;
	}

	if (Bot->GetHealth() && !Bot->GetHealth()->IsAlive())
	{
		if (BotState != EArchonBotState::Dead)
		{
			++Deaths;
			DeadSeconds = 0.0f;
			Bot->SetBodyDead(true);
			SetBotState(EArchonBotState::Dead, TEXT("health_zero"));
		}

		// Players die and respawn; so do bots (free baseline body,
		// decision 2). Back at the home-core ring, full health, march on.
		DeadSeconds += DeltaTime;
		if (DeadSeconds >= RespawnSeconds)
		{
			const FVector HomeBase = OwnCore && IsValid(OwnCore) ? OwnCore->GetActorLocation() : Bot->GetActorLocation();
			const float Angle = (2.0f * PI * FMath::Abs(BotIndex + Deaths)) / 8.0f;
			const FVector RespawnLocation(
				HomeBase.X + FMath::Cos(Angle) * 900.0f,
				HomeBase.Y + FMath::Sin(Angle) * 900.0f,
				250.0f);
			Bot->SetBodyDead(false);
			Bot->TeleportTo(RespawnLocation, FRotator::ZeroRotator);
			Bot->GetHealth()->HealToFull();
			bHasStuckSample = false;
			StuckSampleSeconds = 0.0f;
			LowProgressSeconds = 0.0f;
			UnstickSeconds = 0.0f;
			UE_LOG(
				LogTemp,
				Display,
				TEXT("ArchonFactoryCanary: BotRespawned bot=%d team=%d deaths=%d"),
				BotIndex,
				TeamId,
				Deaths);
			ArchonBotFeatureTelemetry::LogUse(
				BotIndex,
				TeamId,
				TEXT("respawn"),
				TEXT("attempt"),
				TEXT("success"),
				TEXT("respawn_timer"));
			SetBotState(EArchonBotState::MoveToObjective, TEXT("respawned"));
		}
		return;
	}

	const FVector BotLocation = Bot->GetActorLocation();

	// Once a bot is already in siege range, finish the objective pressure.
	// Fair sight/hearing still controls the midfield, but base attackers
	// should not abandon a nearly-dead tower/core just because defenders
	// arrive in the same room.
	if (TryAttackObjectiveStructure(*Bot, BotLocation))
	{
		return;
	}

	// SIGHT/HEARING: fight what a human in this seat would perceive.
	// Perceiving refreshes the MEMORY of the target.
	FVector NativeLastKnownLocation = FVector::ZeroVector;
	FName NativeSenseTag = NAME_None;
	if (AArchonCanaryFpsCharacter* Enemy = FindNearestNativePerceivedEnemy(*Bot, NativeLastKnownLocation, NativeSenseTag))
	{
		PursuedEnemy = Enemy;
		PursuitLastKnownLocation = NativeLastKnownLocation;
		PursuitMemorySeconds = PursuitMemoryWindowSeconds;
		if (LastNativePerceptionLoggedTarget.Get() != Enemy || LastNativePerceptionLoggedSense != NativeSenseTag)
		{
			LastNativePerceptionLoggedTarget = Enemy;
			LastNativePerceptionLoggedSense = NativeSenseTag;
			UE_LOG(
				LogTemp,
				Display,
				TEXT("ArchonFactoryCanary: BotNativePerception bot=%d team=%d target=%s sense=%s lastKnown=(%.0f,%.0f,%.0f)"),
				BotIndex,
				TeamId,
				*Enemy->GetName(),
				*NativeSenseTag.ToString(),
				PursuitLastKnownLocation.X,
				PursuitLastKnownLocation.Y,
				PursuitLastKnownLocation.Z);
		}
		if (BotState != EArchonBotState::EngageEnemy)
		{
			SetBotState(EArchonBotState::EngageEnemy, *Enemy->GetName());
		}
		const FVector AimPoint = Enemy->GetActorLocation() + FVector(0.0f, 0.0f, 40.0f);
		Bot->SetActorRotation(FRotator(0.0f, (AimPoint - BotLocation).Rotation().Yaw, 0.0f));
		FireBowAt(*Bot, AimPoint);
		return;
	}
	if (AArchonCanaryFpsCharacter* Enemy = FindNearestPerceivedEnemy(*Bot))
	{
		PursuedEnemy = Enemy;
		PursuitLastKnownLocation = Enemy->GetActorLocation();
		PursuitMemorySeconds = PursuitMemoryWindowSeconds;
		if (LastFallbackPerceptionLoggedTarget.Get() != Enemy)
		{
			LastFallbackPerceptionLoggedTarget = Enemy;
			UE_LOG(
				LogTemp,
				Display,
				TEXT("ArchonFactoryCanary: BotFallbackPerception bot=%d team=%d target=%s sense=custom_fair_senses"),
				BotIndex,
				TeamId,
				*Enemy->GetName());
		}
		if (BotState != EArchonBotState::EngageEnemy)
		{
			SetBotState(EArchonBotState::EngageEnemy, *Enemy->GetName());
		}
		const FVector AimPoint = Enemy->GetActorLocation() + FVector(0.0f, 0.0f, 40.0f);
		Bot->SetActorRotation(FRotator(0.0f, (AimPoint - BotLocation).Rotation().Yaw, 0.0f));
		FireBowAt(*Bot, AimPoint);
		return;
	}

	// MEMORY: lost sight of a live target -> chase the last-known spot
	// until the trail goes cold or the target is confirmed dead.
	FVector MoveOverride = FVector::ZeroVector;
	bool bHasMoveOverride = false;
	AArchonCanaryFpsCharacter* Pursued = PursuedEnemy.Get();
	if (Pursued && (!Pursued->GetHealth() || !Pursued->GetHealth()->IsAlive()))
	{
		PursuedEnemy = nullptr;
		PursuitMemorySeconds = 0.0f;
	}
	else if (Pursued && PursuitMemorySeconds > 0.0f)
	{
		PursuitMemorySeconds -= DeltaTime;
		if (FVector::Dist2D(BotLocation, PursuitLastKnownLocation) > 300.0f)
		{
			MoveOverride = PursuitLastKnownLocation;
			bHasMoveOverride = true;
			if (BotState != EArchonBotState::Pursuing)
			{
				SetBotState(EArchonBotState::Pursuing, *Pursued->GetName());
			}
		}
		// At the last-known spot with nothing perceived: let the
		// memory window run out, then shrug and move on.
	}

	// PAIN: got shot -> hunt the direction it came from.
	if (!bHasMoveOverride && ThreatSeconds > 0.0f)
	{
		ThreatSeconds -= DeltaTime;
		if (!ThreatLocation.IsNearlyZero() && FVector::Dist2D(BotLocation, ThreatLocation) > 300.0f)
		{
			MoveOverride = ThreatLocation;
			bHasMoveOverride = true;
			if (BotState != EArchonBotState::HuntingThreat)
			{
				SetBotState(EArchonBotState::HuntingThreat, TEXT("hit_from_there"));
			}
		}
	}

	// HUD: the base-under-attack indicator every player gets. Fighters
	// rotate home to the named building; only bots close enough to have
	// SEEN the arrow land earn the shooter's position.
	if (!bHasMoveOverride && !bBreacher)
	{
		if (AArchonMatchStateActor* State = ResolveMatchState())
		{
			FArchonBaseAttackEvent Attack;
			if (State->GetLatestBaseAttack(TeamId, 10.0, Attack) && Attack.EventTimeSeconds > LastBaseAttackHandled)
			{
				LastBaseAttackHandled = Attack.EventTimeSeconds;
				if (!Attack.ShotOrigin.IsNearlyZero() &&
					UArchonBotPerceptionPolicyLibrary::EarnsEyesOnOrigin(BotLocation, Attack.StructureLocation, EyesOnRadius))
				{
					ThreatLocation = Attack.ShotOrigin;
					ThreatSeconds = ThreatWindowSeconds;
					MoveOverride = ThreatLocation;
					bHasMoveOverride = true;
					SetBotState(EArchonBotState::HuntingThreat, TEXT("saw_arrow_hit_base"));
				}
				else if (FVector::Dist2D(BotLocation, Attack.StructureLocation) > 1200.0f)
				{
					MoveOverride = Attack.StructureLocation;
					bHasMoveOverride = true;
					SetBotState(EArchonBotState::DefendingBase, *Attack.StructureLabel.ToString());
				}
			}
			else if (BotState == EArchonBotState::DefendingBase &&
				State->GetLatestBaseAttack(TeamId, 10.0, Attack) &&
				FVector::Dist2D(BotLocation, Attack.StructureLocation) > 1200.0f)
			{
				// Keep rotating home while the alert is fresh.
				MoveOverride = Attack.StructureLocation;
				bHasMoveOverride = true;
			}
		}
	}

	if (!bHasMoveOverride)
	{
		// Objective attack was already checked before perception. If no
		// structure is in range, keep marching until one is.
	}

	// March on the enemy base via stock NavMesh pathfinding (bots were
	// piling up on edge blocks with hand-rolled steering — Jonathan
	// 2026-06-10). Slide steering remains the no-controller fallback.
	if (!bHasMoveOverride && BotState != EArchonBotState::MoveToObjective)
	{
		SetBotState(EArchonBotState::MoveToObjective, TEXT("resume_march"));
		MoveReissueSeconds = 0.0f;
	}

	bool bChasingRouteWaypoint = false;
	bool bChasingObjectiveStructure = false;
	FName CurrentObjectiveFeatureTarget = NAME_None;
	FVector Objective = TargetCore ? TargetCore->GetActorLocation() : BotLocation;
	if (bHasMoveOverride)
	{
		// Pursuit/threat/defense movement outranks the route march.
		Objective = MoveOverride;
	}
	else if (bHasRouteWaypoint && !bRouteWaypointReached)
	{
		if (FVector::Dist2D(BotLocation, RouteWaypoint) <= 700.0f)
		{
			bRouteWaypointReached = true;
			RouteWaypointStallAttempts = 0;
			MoveReissueSeconds = 0.0f;
			UE_LOG(
				LogTemp,
				Display,
				TEXT("ArchonFactoryCanary: BotWaypointReached bot=%d team=%d"),
				BotIndex,
				TeamId);
		}
		else
		{
			bChasingRouteWaypoint = true;
			Objective = RouteWaypoint;
		}
	}
	else
	{
		FVector StructureLocation = TargetCore && IsValid(TargetCore) ? TargetCore->GetActorLocation() : BotLocation;
		float StandOffDistance = CoreStandOffDistance;
		CurrentObjectiveFeatureTarget = TargetCore && IsValid(TargetCore) ? FName(TEXT("core")) : NAME_None;

		if (AArchonBaseDefenseTowerActor* GateTower = FindNearestLivingEnemyTower(BotLocation, ObjectiveScanRange))
		{
			StructureLocation = GateTower->GetActorLocation();
			StandOffDistance = TowerStandOffDistance;
			CurrentObjectiveFeatureTarget = FName(TEXT("tower"));
		}

		if (!CurrentObjectiveFeatureTarget.IsNone())
		{
			if (CurrentObjectiveFeatureTarget != LastObjectiveStallFeatureTarget)
			{
				LastObjectiveStallFeatureTarget = CurrentObjectiveFeatureTarget;
				ObjectiveStallAttempts = 0;
				ObjectiveLaneShift = 0;
			}
			bChasingObjectiveStructure = true;
			const int32 LaneBotIndex = BotIndex + ObjectiveLaneShift;
			Objective = UArchonBotSteeringPolicyLibrary::ComputeObjectiveStandOffTarget(
				BotLocation,
				StructureLocation,
				LaneBotIndex,
				StandOffDistance,
				ObjectiveLaneSpacing);

			if (LastFiringPositionFeatureTarget != CurrentObjectiveFeatureTarget)
			{
				LastFiringPositionFeatureTarget = CurrentObjectiveFeatureTarget;
				const FString TargetReason = CurrentObjectiveFeatureTarget.ToString();
				UE_LOG(
					LogTemp,
					Display,
					TEXT("ArchonFactoryCanary: BotFiringPosition bot=%d team=%d target=%s location=(%.0f,%.0f,%.0f) standOff=%.0f laneSpacing=%.0f laneShift=%d"),
					BotIndex,
					TeamId,
					*TargetReason,
					Objective.X,
					Objective.Y,
					Objective.Z,
					StandOffDistance,
					ObjectiveLaneSpacing,
					ObjectiveLaneShift);
				ArchonBotFeatureTelemetry::LogUse(
					BotIndex,
					TeamId,
					TEXT("take_firing_position"),
					TEXT("attempt"),
					TEXT("success"),
					*TargetReason);
			}
		}
	}

	bool bForceSlideFallback = false;
	const float DistanceToObjective = FVector::Dist2D(BotLocation, Objective);
	if (DistanceToObjective <= 350.0f)
	{
		LowProgressSeconds = 0.0f;
		StuckSampleSeconds = 0.0f;
		bHasStuckSample = false;
		UnstickSeconds = 0.0f;
	}
	else if (UnstickSeconds > 0.0f)
	{
		UnstickSeconds -= DeltaTime;
		Objective = UnstickObjective;
		bForceSlideFallback = true;
	}
	else
	{
		if (!bHasStuckSample)
		{
			LastStuckSampleLocation = BotLocation;
			StuckSampleSeconds = 0.0f;
			bHasStuckSample = true;
		}
		else
		{
			StuckSampleSeconds += DeltaTime;
			if (StuckSampleSeconds >= StuckSampleIntervalSeconds)
			{
				const float Moved = FVector::Dist2D(BotLocation, LastStuckSampleLocation);
				if (DistanceToObjective > 700.0f && Moved < StuckMinimumProgress)
				{
					LowProgressSeconds += StuckSampleSeconds;
				}
				else
				{
					LowProgressSeconds = 0.0f;
				}
				LastStuckSampleLocation = BotLocation;
				StuckSampleSeconds = 0.0f;
			}
		}

		if (LowProgressSeconds >= StuckTriggerSeconds)
		{
			bool bClearUnstickCandidate = false;
			int32 UnstickCandidateCount = 0;
			UnstickObjective = SelectUnstickEscapeTarget(
				*World,
				*Bot,
				BotLocation,
				Objective,
				bClearUnstickCandidate,
				UnstickCandidateCount);
			++UnstickAttempt;
			LowProgressSeconds = 0.0f;
			StuckSampleSeconds = 0.0f;
			LastStuckSampleLocation = BotLocation;
			UnstickSeconds = UnstickDurationSeconds;
			Objective = UnstickObjective;
			bForceSlideFallback = true;
			bNavMoveWorking = false;
			MoveReissueSeconds = 0.0f;
			if (AAIController* PathController = Cast<AAIController>(Bot->GetController()))
			{
				PathController->StopMovement();
			}
			UE_LOG(
				LogTemp,
				Display,
				TEXT("ArchonFactoryCanary: BotStuckRecovery bot=%d team=%d attempt=%d target=(%.0f,%.0f,%.0f)"),
				BotIndex,
				TeamId,
				UnstickAttempt,
				UnstickObjective.X,
				UnstickObjective.Y,
				UnstickObjective.Z);
			UE_LOG(
				LogTemp,
				Display,
				TEXT("ArchonFactoryCanary: BotUnstickQuery bot=%d team=%d attempt=%d candidates=%d selected=%s target=(%.0f,%.0f,%.0f)"),
				BotIndex,
				TeamId,
				UnstickAttempt,
				UnstickCandidateCount,
				bClearUnstickCandidate ? TEXT("clear") : TEXT("fallback"),
				UnstickObjective.X,
				UnstickObjective.Y,
				UnstickObjective.Z);
			const TCHAR* StuckRecoveryPhase = bChasingRouteWaypoint
				? TEXT("route")
				: (bChasingObjectiveStructure ? TEXT("objective") : (bHasMoveOverride ? TEXT("move_override") : TEXT("march")));
			const FString ObjectiveFeatureTargetText = CurrentObjectiveFeatureTarget.IsNone()
				? FString(TEXT("none"))
				: CurrentObjectiveFeatureTarget.ToString();
			UE_LOG(
				LogTemp,
				Display,
				TEXT("ArchonFactoryCanary: BotStuckRecoveryContext bot=%d team=%d attempt=%d phase=%s routeActive=%d routeReached=%d routeStallAttempts=%d routeThreshold=%d objectiveTarget=%s objectiveStallAttempts=%d laneShift=%d distance=%.0f"),
				BotIndex,
				TeamId,
				UnstickAttempt,
				StuckRecoveryPhase,
				bHasRouteWaypoint ? 1 : 0,
				bRouteWaypointReached ? 1 : 0,
				RouteWaypointStallAttempts,
				RouteStallMaxUnstickAttempts,
				*ObjectiveFeatureTargetText,
				ObjectiveStallAttempts,
				ObjectiveLaneShift,
				DistanceToObjective);
			ArchonBotFeatureTelemetry::LogUse(
				BotIndex,
				TeamId,
				TEXT("unstick_reposition"),
				TEXT("attempt"),
				bClearUnstickCandidate ? TEXT("success") : TEXT("fallback"),
				TEXT("low_progress_query"));

			if (bChasingRouteWaypoint)
			{
				++RouteWaypointStallAttempts;
				if (UArchonBotSteeringPolicyLibrary::ShouldAbandonRouteWaypointAfterStalls(
					RouteWaypointStallAttempts,
					RouteStallMaxUnstickAttempts))
				{
					const int32 AbandonedRouteStallAttempts = RouteWaypointStallAttempts;
					const float DistanceToRoute = FVector::Dist2D(BotLocation, RouteWaypoint);
					bRouteWaypointReached = true;
					RouteWaypointStallAttempts = 0;
					MoveReissueSeconds = 0.0f;
					bNavMoveWorking = false;
					LastFiringPositionFeatureTarget = NAME_None;
					UE_LOG(
						LogTemp,
						Display,
						TEXT("ArchonFactoryCanary: BotRouteWaypointAbandoned bot=%d team=%d attempts=%d distance=%.0f waypoint=(%.0f,%.0f,%.0f)"),
						BotIndex,
						TeamId,
						AbandonedRouteStallAttempts,
						DistanceToRoute,
						RouteWaypoint.X,
						RouteWaypoint.Y,
						RouteWaypoint.Z);
					ArchonBotFeatureTelemetry::LogUse(
						BotIndex,
						TeamId,
						TEXT("route_waypoint_abandon"),
						TEXT("attempt"),
						TEXT("success"),
						TEXT("route_stall"));
				}
			}
			else if (bChasingObjectiveStructure && !CurrentObjectiveFeatureTarget.IsNone())
			{
				++ObjectiveStallAttempts;
				const int32 NextLaneShift = UArchonBotSteeringPolicyLibrary::ComputeObjectiveLaneShift(
					ObjectiveStallAttempts,
					ObjectiveStallLaneShiftAttempts,
					ObjectiveStallMaxLaneShift);
				if (NextLaneShift != ObjectiveLaneShift)
				{
					ObjectiveLaneShift = NextLaneShift;
					MoveReissueSeconds = 0.0f;
					bNavMoveWorking = false;
					LastFiringPositionFeatureTarget = NAME_None;
					const FString TargetReason = CurrentObjectiveFeatureTarget.ToString();
					UE_LOG(
						LogTemp,
						Display,
						TEXT("ArchonFactoryCanary: BotObjectiveLaneShift bot=%d team=%d target=%s attempts=%d laneShift=%d"),
						BotIndex,
						TeamId,
						*TargetReason,
						ObjectiveStallAttempts,
						ObjectiveLaneShift);
					ArchonBotFeatureTelemetry::LogUse(
						BotIndex,
						TeamId,
						TEXT("objective_lane_shift"),
						TEXT("attempt"),
						TEXT("success"),
						*TargetReason);
				}
			}
		}
	}

	if (!bForceSlideFallback)
	{
		if (AAIController* PathController = Cast<AAIController>(Bot->GetController()))
		{
			// Reissue periodically: covers respawn teleports, nav tiles
			// appearing under invokers, and stalled path-follows. If nav data
			// is missing entirely (runtime worlds have no bounds volume yet),
		// MoveToLocation fails — fall through to slide steering instead
			// of freezing (silent-match bug #2, 2026-06-10).
			MoveReissueSeconds -= DeltaTime;
			if (MoveReissueSeconds <= 0.0f)
			{
				MoveReissueSeconds = 2.0f;
				const EPathFollowingRequestResult::Type MoveResult =
					PathController->MoveToLocation(Objective, /*AcceptanceRadius=*/ 250.0f);
				const bool bNavWorkedBefore = bNavMoveWorking;
				bNavMoveWorking = MoveResult != EPathFollowingRequestResult::Failed;
				if (bNavWorkedBefore != bNavMoveWorking)
				{
					UE_LOG(
						LogTemp,
						Display,
						TEXT("ArchonFactoryCanary: BotNavMode bot=%d nav=%s"),
						BotIndex,
						bNavMoveWorking ? TEXT("pathfinding") : TEXT("slide_fallback"));
				}
			}
			if (bNavMoveWorking)
			{
				return;
			}
		}
	}

	const FVector ProbeStart = BotLocation + FVector(0.0f, 0.0f, 20.0f);
	const FVector Forward = (FVector(Objective.X, Objective.Y, BotLocation.Z) - BotLocation).GetSafeNormal2D();
	FHitResult Probe;
	FCollisionQueryParams ProbeParams(SCENE_QUERY_STAT(ArchonBotProbe), false, Bot);
	const bool bObstacleAhead = World->LineTraceSingleByChannel(
		Probe,
		ProbeStart,
		ProbeStart + Forward * ObstacleProbeDistance,
		ECC_Visibility,
		ProbeParams);

	const FArchonBotSteeringDecision Steering = UArchonBotSteeringPolicyLibrary::ComputeSteering(
		BotLocation,
		Objective,
		bObstacleAhead,
		bObstacleAhead ? Probe.ImpactNormal : FVector::ZeroVector,
		350.0f);

	if (!Steering.MoveDirection.IsNearlyZero())
	{
		Bot->SetActorRotation(FRotator(0.0f, Steering.MoveDirection.Rotation().Yaw, 0.0f));
		Bot->AddMovementInput(Steering.MoveDirection, 1.0f);
	}
}

AArchonCanaryFpsCharacter* UArchonBotBrainComponent::ResolveBotCharacter() const
{
	return Cast<AArchonCanaryFpsCharacter>(GetOwner());
}

AArchonCanaryFpsCharacter* UArchonBotBrainComponent::FindNearestNativePerceivedEnemy(
	const AArchonCanaryFpsCharacter& Bot,
	FVector& OutLastKnownLocation,
	FName& OutSenseTag) const
{
	const AArchonBotAIController* BotController = Cast<AArchonBotAIController>(Bot.GetController());
	if (!BotController)
	{
		return nullptr;
	}

	return BotController->FindNearestNativePerceivedEnemy(
		FMath::Max(EngageRange, HearingRadius),
		OutLastKnownLocation,
		OutSenseTag);
}

AArchonCanaryFpsCharacter* UArchonBotBrainComponent::FindNearestPerceivedEnemy(const AArchonCanaryFpsCharacter& Bot) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	const FVector BotLocation = Bot.GetActorLocation();
	const FVector BotForward = Bot.GetActorForwardVector();

	AArchonCanaryFpsCharacter* Nearest = nullptr;
	float NearestDistance = TNumericLimits<float>::Max();
	for (TActorIterator<AArchonCanaryFpsCharacter> It(World); It; ++It)
	{
		AArchonCanaryFpsCharacter* Candidate = *It;
		if (!Candidate || Candidate == GetOwner() || Candidate == &Bot)
		{
			continue;
		}
		UArchonCombatHealthComponent* Health = Candidate->GetHealth();
		if (!Health || !Health->IsAlive() || Health->GetTeamId() == TeamId)
		{
			continue;
		}

		const FVector CandidateLocation = Candidate->GetActorLocation();
		const float Distance = FVector::Dist2D(BotLocation, CandidateLocation);
		if (Distance >= NearestDistance)
		{
			continue;
		}

		// HEARING: close footsteps register regardless of facing.
		const bool bHeard = UArchonBotPerceptionPolicyLibrary::IsWithinHearing(
			BotLocation, CandidateLocation, HearingRadius);
		// SIGHT: inside the forward cone, in range, and actually
		// visible (LOS trace — trees and towers block sight now that
		// decor has collision).
		const bool bSeen = !bHeard &&
			UArchonBotPerceptionPolicyLibrary::IsInVisionCone(
				BotLocation, BotForward, CandidateLocation, EngageRange, VisionHalfAngleDegrees) &&
			HasLineOfSight(Bot, *Candidate);
		if (!bHeard && !bSeen)
		{
			continue;
		}

		NearestDistance = Distance;
		Nearest = Candidate;
	}
	return Nearest;
}

bool UArchonBotBrainComponent::HasLineOfSight(const AArchonCanaryFpsCharacter& Bot, const AArchonCanaryFpsCharacter& Target) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(ArchonBotSight), false, &Bot);
	Params.AddIgnoredActor(&Target);
	const FVector Eye = Bot.GetActorLocation() + FVector(0.0f, 0.0f, 64.0f);
	const FVector TargetPoint = Target.GetActorLocation() + FVector(0.0f, 0.0f, 40.0f);
	return !World->LineTraceSingleByChannel(Hit, Eye, TargetPoint, ECC_Visibility, Params);
}

AArchonMatchStateActor* UArchonBotBrainComponent::ResolveMatchState()
{
	if (MatchState && IsValid(MatchState))
	{
		return MatchState;
	}
	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<AArchonMatchStateActor> It(World); It; ++It)
		{
			MatchState = *It;
			return MatchState;
		}
	}
	return nullptr;
}

void UArchonBotBrainComponent::HandleOwnDamage(const FArchonDamageApplicationResult& DamageResult)
{
	if (!DamageResult.bAccepted || DamageResult.DamageApplied <= 0.0f || bHumanControlled)
	{
		return;
	}
	AArchonCanaryFpsCharacter* Bot = ResolveBotCharacter();
	UArchonCombatHealthComponent* Health = Bot ? Bot->GetHealth() : nullptr;
	if (!Health || Health->GetLastAcceptedShotOrigin().IsNearlyZero())
	{
		return;
	}
	// PAIN: a human knows roughly where the shot came from.
	ThreatLocation = Health->GetLastAcceptedShotOrigin();
	ThreatSeconds = ThreatWindowSeconds;
}

AArchonBaseDefenseTowerActor* UArchonBotBrainComponent::FindNearestLivingEnemyTower(const FVector& FromLocation, float MaxRange) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	AArchonBaseDefenseTowerActor* Nearest = nullptr;
	float NearestDistance = MaxRange;
	for (TActorIterator<AArchonBaseDefenseTowerActor> It(World); It; ++It)
	{
		AArchonBaseDefenseTowerActor* Candidate = *It;
		if (!Candidate || Candidate->GetTeamId() == TeamId)
		{
			continue;
		}
		UArchonCombatHealthComponent* Health = Candidate->GetHealth();
		if (!Health || !Health->IsAlive())
		{
			continue;
		}
		const float Distance = FVector::Dist2D(FromLocation, Candidate->GetActorLocation());
		if (Distance < NearestDistance)
		{
			NearestDistance = Distance;
			Nearest = Candidate;
		}
	}
	return Nearest;
}

void UArchonBotBrainComponent::SetHumanControlled(bool bInHumanControlled)
{
	bHumanControlled = bInHumanControlled;
	if (AArchonCanaryFpsCharacter* Bot = ResolveBotCharacter())
	{
		// Humans steer with the controller; bots own their facing.
		Bot->bUseControllerRotationYaw = bInHumanControlled;
	}
	UE_LOG(
		LogTemp,
		Display,
		TEXT("ArchonFactoryCanary: BotHumanControl bot=%d team=%d humanControlled=%s"),
		BotIndex,
		TeamId,
		bInHumanControlled ? TEXT("true") : TEXT("false"));
}

void UArchonBotBrainComponent::SetRouteWaypoint(const FVector& InWaypoint)
{
	RouteWaypoint = InWaypoint;
	bHasRouteWaypoint = true;
	bRouteWaypointReached = false;
	RouteWaypointStallAttempts = 0;
	ObjectiveStallAttempts = 0;
	ObjectiveLaneShift = 0;
	LastObjectiveStallFeatureTarget = NAME_None;
	LastFiringPositionFeatureTarget = NAME_None;
}

void UArchonBotBrainComponent::SetBotState(EArchonBotState NewState, const TCHAR* Detail)
{
	BotState = NewState;
	const TCHAR* StateName = TEXT("Unknown");
	switch (NewState)
	{
	case EArchonBotState::MoveToObjective: StateName = TEXT("MoveToObjective"); break;
	case EArchonBotState::EngageEnemy:     StateName = TEXT("EngageEnemy"); break;
	case EArchonBotState::Pursuing:        StateName = TEXT("Pursuing"); break;
	case EArchonBotState::HuntingThreat:   StateName = TEXT("HuntingThreat"); break;
	case EArchonBotState::DefendingBase:   StateName = TEXT("DefendingBase"); break;
	case EArchonBotState::AttackTower:     StateName = TEXT("AttackTower"); break;
	case EArchonBotState::AttackCore:      StateName = TEXT("AttackCore"); break;
	case EArchonBotState::Dead:            StateName = TEXT("Dead"); break;
	}
	UE_LOG(
		LogTemp,
		Display,
		TEXT("ArchonFactoryCanary: BotState bot=%d team=%d state=%s detail=%s"),
		BotIndex,
		TeamId,
		StateName,
		Detail);

	FName Feature;
	FName Reason;
	switch (NewState)
	{
	case EArchonBotState::MoveToObjective:
		Feature = TEXT("march_objective");
		Reason = TEXT("state_transition");
		break;
	case EArchonBotState::EngageEnemy:
		Feature = TEXT("perceive_enemy");
		Reason = TEXT("fair_senses");
		break;
	case EArchonBotState::HuntingThreat:
	case EArchonBotState::DefendingBase:
		Feature = TEXT("react_to_base_attack");
		Reason = TEXT("base_attack_alert");
		break;
	case EArchonBotState::AttackTower:
		Feature = TEXT("attack_tower");
		Reason = TEXT("in_range");
		break;
	case EArchonBotState::AttackCore:
		Feature = TEXT("attack_core");
		Reason = TEXT("in_range");
		break;
	default:
		break;
	}
	if (!Feature.IsNone())
	{
		ArchonBotFeatureTelemetry::LogUse(
			BotIndex,
			TeamId,
			Feature,
			TEXT("attempt"),
			TEXT("success"),
			Reason);
	}
}

FVector UArchonBotBrainComponent::SelectUnstickEscapeTarget(
	UWorld& World,
	const AArchonCanaryFpsCharacter& Bot,
	const FVector& BotLocation,
	const FVector& Objective,
	bool& bOutClearCandidate,
	int32& OutCandidateCount) const
{
	bOutClearCandidate = false;
	const TArray<FVector> Candidates = UArchonBotSteeringPolicyLibrary::ComputeUnstickEscapeCandidates(
		BotLocation,
		Objective,
		UnstickAttempt,
		UnstickLateralDistance,
		UnstickBackstepDistance);
	OutCandidateCount = Candidates.Num();

	FCollisionQueryParams Params(SCENE_QUERY_STAT(ArchonBotUnstickQuery), false, &Bot);
	const FVector TraceStart = BotLocation + FVector(0.0f, 0.0f, 48.0f);
	for (const FVector& Candidate : Candidates)
	{
		FHitResult Hit;
		const FVector TraceEnd(Candidate.X, Candidate.Y, BotLocation.Z + 48.0f);
		const bool bBlocked = World.LineTraceSingleByChannel(
			Hit,
			TraceStart,
			TraceEnd,
			ECC_Visibility,
			Params);
		if (!bBlocked)
		{
			bOutClearCandidate = true;
			return FVector(Candidate.X, Candidate.Y, BotLocation.Z);
		}
	}

	return Candidates.Num() > 0 ? Candidates[0] : BotLocation;
}

bool UArchonBotBrainComponent::TryAttackObjectiveStructure(AArchonCanaryFpsCharacter& Bot, const FVector& BotLocation)
{
	// Siege the gate tower before the core (2026-06-11): the tower
	// out-ranges the core-attack ring and was untargetable, so matches
	// stalled at 0-0. Renegade pattern: the AGT dies first.
	if (AArchonBaseDefenseTowerActor* GateTower = FindNearestLivingEnemyTower(BotLocation, TowerAttackRange))
	{
		if (BotState != EArchonBotState::AttackTower)
		{
			SetBotState(EArchonBotState::AttackTower, *GateTower->GetName());
		}
		const FVector TowerAim = GateTower->GetActorLocation() + FVector(0.0f, 0.0f, 120.0f);
		Bot.SetActorRotation(FRotator(0.0f, (TowerAim - BotLocation).Rotation().Yaw, 0.0f));
		FireBowAt(Bot, TowerAim, GateTower->GetHealth());
		return true;
	}

	if (TargetCore && IsValid(TargetCore))
	{
		const float CoreDistance = FVector::Dist2D(BotLocation, TargetCore->GetActorLocation());
		if (CoreDistance <= CoreAttackRange)
		{
			if (BotState != EArchonBotState::AttackCore)
			{
				SetBotState(EArchonBotState::AttackCore, *TargetCore->GetName());
			}
			const FVector CoreAim = TargetCore->GetActorLocation();
			Bot.SetActorRotation(FRotator(0.0f, (CoreAim - BotLocation).Rotation().Yaw, 0.0f));
			FireBowAt(Bot, CoreAim, TargetCore->GetHealth(), CoreSiegeDamageScale);
			return true;
		}
	}

	return false;
}

bool UArchonBotBrainComponent::FireBowAt(
	AArchonCanaryFpsCharacter& Bot,
	const FVector& TargetLocation,
	UArchonCombatHealthComponent* DirectStructureHitTarget,
	float DirectStructureDamageScale)
{
	UArchonVerdantThornsproutBow* Bow = Bot.GetVerdantBow();
	if (!Bow)
	{
		return false;
	}

	FVector Origin = Bot.GetActorLocation() + FVector(0.0f, 0.0f, 64.0f);
	if (const UCameraComponent* Camera = Bot.GetFirstPersonCamera())
	{
		Origin = Camera->GetComponentLocation();
	}
	const FVector Direction = (TargetLocation - Origin).GetSafeNormal();

	// The weapon's own fire-cycle/ammo state gates the rate; auto-reload
	// keeps the bot shooting like a player holding the trigger.
	const bool bFired = Bow->TryFire(Origin, Direction);
	if (!bFired)
	{
		return false;
	}

	if (DirectStructureHitTarget)
	{
		const FArchonWeaponStats Stats = Bow->GetStats();
		FArchonShotPayload StructureShot;
		StructureShot.InstigatorTeamId = TeamId;
		StructureShot.InstigatorPlayerId = BotIndex;
		StructureShot.WeaponClass = Stats.WeaponClass;
		StructureShot.DamageType = Stats.WeaponClass == EArchonWeaponClass::LenswrightPressureBoltCrossbow
			? EArchonDamageType::LenswrightPressureBolt
			: EArchonDamageType::VerdantLivingArrow;
		StructureShot.ShotOrigin = Origin;
		StructureShot.ShotDirection = Direction;
		StructureShot.HitLocation = TargetLocation;
		StructureShot.DistanceTraveled = FVector::Dist(Origin, TargetLocation);
		StructureShot.HitType = EArchonHitType::Body;

		FArchonHitResult Hit = UArchonCombatPolicyLibrary::ResolveShot(
			StructureShot,
			DirectStructureHitTarget->GetTeamId(),
			DirectStructureHitTarget->IsAlive(),
			DirectStructureHitTarget->GetArmorModifier(),
			UArchonCombatPolicyLibrary::GetDefaultWeaponProfile(Stats.WeaponClass));
		if (Hit.bShouldHit)
		{
			Hit.FinalDamage *= FMath::Max(0.0f, DirectStructureDamageScale);
		}
		const FArchonDamageApplicationResult Damage = DirectStructureHitTarget->ApplyHit(Hit);
		if (Damage.bAccepted)
		{
			UE_LOG(
				LogTemp,
				Display,
				TEXT("ArchonFactoryCanary: BotStructureHit bot=%d team=%d targetTeam=%d damage=%.1f health=%.1f"),
				BotIndex,
				TeamId,
				DirectStructureHitTarget->GetTeamId(),
				Damage.DamageApplied,
				Damage.NewHealth);
		}
	}

	return true;
}
