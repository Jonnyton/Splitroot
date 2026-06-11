#include "ArchonBotAIController.h"

#include "ArchonBotStrategyTuningLibrary.h"
#include "ArchonCanaryFpsCharacter.h"
#include "ArchonCombatHealthComponent.h"
#include "AITypes.h"
#include "GenericTeamAgentInterface.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISense.h"
#include "Perception/AISense_Damage.h"
#include "Perception/AISense_Hearing.h"
#include "Perception/AISense_Sight.h"
#include "Perception/AISenseConfig_Damage.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "Perception/AISenseConfig_Sight.h"

namespace
{
	FGenericTeamId ToGenericTeamId(int32 TeamId)
	{
		if (TeamId < 0 || TeamId >= FGenericTeamId::NoTeam.GetId())
		{
			return FGenericTeamId::NoTeam;
		}
		return FGenericTeamId(static_cast<uint8>(TeamId));
	}

	bool IsEnemyCharacter(const AArchonCanaryFpsCharacter& Candidate, int32 TeamId)
	{
		const UArchonCombatHealthComponent* Health = Candidate.GetHealth();
		return Health && Health->IsAlive() && Health->GetTeamId() != TeamId;
	}

	FName ResolveActiveSenseTag(const FActorPerceptionInfo& Info, FVector& OutStimulusLocation)
	{
		const FAISenseID SightId = UAISense::GetSenseID<UAISense_Sight>();
		if (Info.IsSenseActive(SightId))
		{
			OutStimulusLocation = Info.GetStimulusLocation(SightId);
			return TEXT("sight");
		}

		const FAISenseID HearingId = UAISense::GetSenseID<UAISense_Hearing>();
		if (Info.IsSenseActive(HearingId))
		{
			OutStimulusLocation = Info.GetStimulusLocation(HearingId);
			return TEXT("hearing");
		}

		const FAISenseID DamageId = UAISense::GetSenseID<UAISense_Damage>();
		if (Info.IsSenseActive(DamageId))
		{
			OutStimulusLocation = Info.GetStimulusLocation(DamageId);
			return TEXT("damage");
		}

		OutStimulusLocation = Info.GetLastStimulusLocation();
		return TEXT("unknown");
	}
}

AArchonBotAIController::AArchonBotAIController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	BotPerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("BotPerception"));
	SetPerceptionComponent(*BotPerceptionComponent);

	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("BotSight"));
	SightConfig->SightRadius = 1400.0f;
	SightConfig->LoseSightRadius = 1900.0f;
	SightConfig->PeripheralVisionAngleDegrees = 70.0f;
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = false;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = false;

	HearingConfig = CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("BotHearing"));
	HearingConfig->HearingRange = 800.0f;
	HearingConfig->DetectionByAffiliation.bDetectEnemies = true;
	HearingConfig->DetectionByAffiliation.bDetectFriendlies = false;
	HearingConfig->DetectionByAffiliation.bDetectNeutrals = false;

	DamageConfig = CreateDefaultSubobject<UAISenseConfig_Damage>(TEXT("BotDamage"));

	BotPerceptionComponent->ConfigureSense(*SightConfig);
	BotPerceptionComponent->ConfigureSense(*HearingConfig);
	BotPerceptionComponent->ConfigureSense(*DamageConfig);
	BotPerceptionComponent->SetDominantSense(SightConfig->GetSenseImplementation());
}

void AArchonBotAIController::ConfigureBotTeam(int32 InTeamId, int32 InBotIndex)
{
	ConfigureBotTeamWithTuning(InTeamId, InBotIndex, UArchonBotStrategyTuningLibrary::GetSplitrootBotStrategyTuning());
}

void AArchonBotAIController::ConfigureBotTeamWithTuning(
	int32 InTeamId,
	int32 InBotIndex,
	const FArchonBotStrategyTuning& StrategyTuning)
{
	TeamId = InTeamId;
	BotIndex = InBotIndex;
	SetGenericTeamId(ToGenericTeamId(TeamId));

	const bool bBreacher = UArchonBotStrategyTuningLibrary::IsBreacherBot(BotIndex, StrategyTuning);
	const float SightRadius = bBreacher ? StrategyTuning.BreacherEngageRange : StrategyTuning.FighterEngageRange;
	SightConfig->SightRadius = SightRadius;
	SightConfig->LoseSightRadius = SightRadius + 500.0f;
	SightConfig->PeripheralVisionAngleDegrees = StrategyTuning.VisionHalfAngleDegrees;
	HearingConfig->HearingRange = StrategyTuning.HearingRadius;

	BotPerceptionComponent->ConfigureSense(*SightConfig);
	BotPerceptionComponent->ConfigureSense(*HearingConfig);
	BotPerceptionComponent->RequestStimuliListenerUpdate();

	UE_LOG(
		LogTemp,
		Display,
		TEXT("ArchonFactoryCanary: BotAIControllerConfigured bot=%d team=%d role=%s sightRadius=%.0f hearingRadius=%.0f nativePerception=true tuningRevision=%s"),
		BotIndex,
		TeamId,
		bBreacher ? TEXT("breacher") : TEXT("fighter"),
		SightConfig->SightRadius,
		HearingConfig->HearingRange,
		*StrategyTuning.Revision);
}

void AArchonBotAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	if (BotPerceptionComponent)
	{
		BotPerceptionComponent->RequestStimuliListenerUpdate();
	}
}

AArchonCanaryFpsCharacter* AArchonBotAIController::FindNearestNativePerceivedEnemy(
	float MaxRange,
	FVector& OutLastKnownLocation,
	FName& OutSenseTag) const
{
	OutLastKnownLocation = FVector::ZeroVector;
	OutSenseTag = NAME_None;

	const APawn* OwnPawn = GetPawn();
	const AArchonCanaryFpsCharacter* Bot = Cast<AArchonCanaryFpsCharacter>(OwnPawn);
	const UAIPerceptionComponent* Perception = GetAIPerceptionComponent();
	if (!Bot || !Perception || MaxRange <= 0.0f)
	{
		return nullptr;
	}

	TArray<AActor*> PerceivedActors;
	Perception->GetCurrentlyPerceivedActors(nullptr, PerceivedActors);

	AArchonCanaryFpsCharacter* Nearest = nullptr;
	float NearestDistance = TNumericLimits<float>::Max();
	FVector NearestLastKnown = FVector::ZeroVector;
	FName NearestSense = NAME_None;

	const FVector BotLocation = Bot->GetActorLocation();
	for (AActor* Actor : PerceivedActors)
	{
		AArchonCanaryFpsCharacter* Candidate = Cast<AArchonCanaryFpsCharacter>(Actor);
		if (!Candidate || Candidate == Bot || !IsEnemyCharacter(*Candidate, TeamId))
		{
			continue;
		}

		const float Distance = FVector::Dist2D(BotLocation, Candidate->GetActorLocation());
		if (Distance > MaxRange || Distance >= NearestDistance)
		{
			continue;
		}

		const FActorPerceptionInfo* Info = Perception->GetActorInfo(*Candidate);
		if (!Info || !Info->HasAnyCurrentStimulus())
		{
			continue;
		}

		FVector StimulusLocation = Candidate->GetActorLocation();
		const FName SenseTag = ResolveActiveSenseTag(*Info, StimulusLocation);
		if (!FAISystem::IsValidLocation(StimulusLocation))
		{
			StimulusLocation = Candidate->GetActorLocation();
		}

		NearestDistance = Distance;
		Nearest = Candidate;
		NearestLastKnown = StimulusLocation;
		NearestSense = SenseTag;
	}

	if (Nearest)
	{
		OutLastKnownLocation = NearestLastKnown;
		OutSenseTag = NearestSense;
	}
	return Nearest;
}

ETeamAttitude::Type AArchonBotAIController::GetTeamAttitudeTowards(const AActor& Other) const
{
	if (const AArchonCanaryFpsCharacter* OtherCharacter = Cast<AArchonCanaryFpsCharacter>(&Other))
	{
		const UArchonCombatHealthComponent* Health = OtherCharacter->GetHealth();
		if (!Health)
		{
			return ETeamAttitude::Neutral;
		}
		return Health->GetTeamId() == TeamId ? ETeamAttitude::Friendly : ETeamAttitude::Hostile;
	}

	const IGenericTeamAgentInterface* OtherTeamAgent = Cast<const IGenericTeamAgentInterface>(&Other);
	return OtherTeamAgent
		? FGenericTeamId::GetAttitude(GetGenericTeamId(), OtherTeamAgent->GetGenericTeamId())
		: ETeamAttitude::Neutral;
}
