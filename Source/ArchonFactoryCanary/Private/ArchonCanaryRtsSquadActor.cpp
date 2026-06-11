#include "ArchonCanaryRtsSquadActor.h"
#include "ArchonTeamRtsStateComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "UObject/ConstructorHelpers.h"

AArchonCanaryRtsSquadActor::AArchonCanaryRtsSquadActor()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	SquadMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SquadMesh"));
	SquadMesh->SetupAttachment(SceneRoot);
	SquadMesh->SetRelativeScale3D(FVector(0.75f, 0.75f, 0.75f));
	SquadMesh->SetCollisionProfileName(TEXT("BlockAll"));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		SquadMesh->SetStaticMesh(CubeMesh.Object);
	}

	StatusText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("StatusText"));
	StatusText->SetupAttachment(SceneRoot);
	StatusText->SetRelativeLocation(FVector(-80.0f, -55.0f, 92.0f));
	StatusText->SetHorizontalAlignment(EHTA_Left);
	StatusText->SetTextRenderColor(FColor(120, 255, 135));
	StatusText->SetWorldSize(18.0f);

	ActiveDestination = GetActorLocation();
	SetRuntimeStatusText(TEXT("CANARY SQUAD READY"));
}

void AArchonCanaryRtsSquadActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	AdvanceTowardDestination(DeltaSeconds);
}

void AArchonCanaryRtsSquadActor::ConfigureSquad(int32 InTeamId, FName InSquadId)
{
	TeamId = InTeamId;
	SquadId = InSquadId;
	SetRuntimeStatusText(FString::Printf(TEXT("SQUAD READY\n%s"), *SquadId.ToString()));
}

void AArchonCanaryRtsSquadActor::AttachTeamState(UArchonTeamRtsStateComponent* InTeamState)
{
	if (TeamState)
	{
		TeamState->OnCommandAccepted.RemoveAll(this);
	}

	TeamState = InTeamState;
	if (TeamState)
	{
		TeamState->OnCommandAccepted.AddUObject(this, &AArchonCanaryRtsSquadActor::HandleAcceptedCommand);
	}
}

bool AArchonCanaryRtsSquadActor::ApplyAcceptedCommand(
	const FArchonRtsCommandIntent& Intent,
	const FArchonRtsAuthorityDecision& Decision)
{
	if (!Decision.bAccepted || !Decision.bMutatesTeamState)
	{
		return false;
	}

	if (Intent.TeamId != TeamId || Intent.SubjectId != SquadId)
	{
		return false;
	}

	if (Decision.FinalSequence <= LastAppliedCommandSequence)
	{
		return false;
	}

	LastAppliedCommandSequence = Decision.FinalSequence;

	if (Intent.OrderKind == EArchonRtsOrderKind::AttackTarget)
	{
		OrderState = EArchonCanaryRtsSquadOrderState::Attacking;
		ActiveDestination = Intent.bTargetLocationValid ? Intent.TargetLocation : AttackDestination;
		SetRuntimeStatusText(FString::Printf(TEXT("SQUAD ATTACKING\nSeq %d -> %s"), LastAppliedCommandSequence, *Intent.TargetId.ToString()));
	}
	else
	{
		OrderState = EArchonCanaryRtsSquadOrderState::Moving;
		ActiveDestination = Intent.bTargetLocationValid ? Intent.TargetLocation : RallyDestination;
		SetRuntimeStatusText(FString::Printf(TEXT("SQUAD MOVING\nSeq %d -> %s"), LastAppliedCommandSequence, *Intent.TargetId.ToString()));
	}

	UE_LOG(
		LogTemp,
		Display,
		TEXT("ArchonFactoryCanary: RuntimeRtsSquadCommandAccepted squad=%s order=%d sequence=%d destination=%s"),
		*SquadId.ToString(),
		static_cast<int32>(Intent.OrderKind),
		LastAppliedCommandSequence,
		*ActiveDestination.ToCompactString());

	return true;
}

void AArchonCanaryRtsSquadActor::HandleAcceptedCommand(
	const FArchonRtsCommandIntent& Intent,
	const FArchonRtsAuthorityDecision& Decision)
{
	ApplyAcceptedCommand(Intent, Decision);
}

void AArchonCanaryRtsSquadActor::SetRuntimeStatusText(const FString& InStatusText)
{
	RuntimeStatusText = InStatusText;
	if (StatusText)
	{
		StatusText->SetText(FText::FromString(RuntimeStatusText));
	}
}

void AArchonCanaryRtsSquadActor::AdvanceTowardDestination(float DeltaSeconds)
{
	if (OrderState == EArchonCanaryRtsSquadOrderState::Idle)
	{
		return;
	}

	const FVector CurrentLocation = GetActorLocation();
	if (FVector::DistSquared(CurrentLocation, ActiveDestination) <= FMath::Square(8.0f))
	{
		if (OrderState == EArchonCanaryRtsSquadOrderState::Moving)
		{
			OrderState = EArchonCanaryRtsSquadOrderState::Idle;
			SetRuntimeStatusText(FString::Printf(TEXT("SQUAD HOLDING\nSeq %d"), LastAppliedCommandSequence));
		}
		return;
	}

	SetActorLocation(FMath::VInterpConstantTo(CurrentLocation, ActiveDestination, DeltaSeconds, MovementSpeed));
}
