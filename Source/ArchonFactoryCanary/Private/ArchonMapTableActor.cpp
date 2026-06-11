#include "ArchonMapTableActor.h"
#include "ArchonMapTableMiniatureComponent.h"
#include "ArchonMapTablePolicyLibrary.h"
#include "ArchonTeamRtsStateComponent.h"
#include "ArchonTeamVisibilityStateComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "UObject/ConstructorHelpers.h"

AArchonMapTableActor::AArchonMapTableActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	TableMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TableMesh"));
	TableMesh->SetupAttachment(SceneRoot);
	TableMesh->SetCollisionProfileName(TEXT("BlockAll"));

	// Temp-asset rule (Jonathan 2026-06-10): the map table is an ACTUAL
	// table — Poly Haven wooden_table_02 (CC0), fetched via API and
	// imported through the headless Python channel. Cube fallback keeps
	// proofs alive if stand-in content is ever missing.
	// NOTE (2026-06-11): the original wooden_table_02 package saved
	// wrong-classed (skeletal marker, no StaticMesh export). The forced
	// static re-import landed as wooden_table_021 because the dead
	// package still occupied the name — verified: 021 carries the
	// StaticMesh + BodySetup exports, 02 carries none. Point at the
	// real mesh; cleanup (delete dead package, rename 021 -> 02) needs
	// an editor-side rename pass in a future build-lock window.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> StandInTable(TEXT("/Game/StandIns/MapTable/wooden_table_021.wooden_table_021"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (StandInTable.Succeeded())
	{
		TableMesh->SetStaticMesh(StandInTable.Object);
		// A ~1m real-world table scaled into a command table.
		TableMesh->SetRelativeScale3D(FVector(2.2f, 2.2f, 2.2f));
		TableMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 0.0f));
		UE_LOG(
			LogTemp,
			Display,
			TEXT("ArchonFactoryCanary: MapTableMeshLoaded loaded=true asset=/Game/StandIns/MapTable/wooden_table_021.wooden_table_021"));
	}
	else if (CubeMesh.Succeeded())
	{
		TableMesh->SetStaticMesh(CubeMesh.Object);
		TableMesh->SetRelativeScale3D(FVector(2.4f, 1.4f, 0.18f));
		TableMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 36.0f));
		UE_LOG(LogTemp, Warning, TEXT("ArchonFactoryCanary: MapTableMeshLoaded loaded=false fallback=/Engine/BasicShapes/Cube.Cube"));
	}

	HeaderText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("HeaderText"));
	HeaderText->SetupAttachment(SceneRoot);
	HeaderText->SetRelativeLocation(FVector(-110.0f, -80.0f, 92.0f));
	HeaderText->SetRelativeRotation(FRotator(0.0f, 0.0f, 0.0f));
	HeaderText->SetHorizontalAlignment(EHTA_Left);
	HeaderText->SetTextRenderColor(FColor(115, 225, 255));
	HeaderText->SetWorldSize(22.0f);
	HeaderText->SetText(FText::FromString(TEXT("ARCHON MAP TABLE")));

	StatusText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("StatusText"));
	StatusText->SetupAttachment(SceneRoot);
	StatusText->SetRelativeLocation(FVector(-110.0f, -80.0f, 66.0f));
	StatusText->SetRelativeRotation(FRotator(0.0f, 0.0f, 0.0f));
	StatusText->SetHorizontalAlignment(EHTA_Left);
	StatusText->SetTextRenderColor(FColor::White);
	StatusText->SetWorldSize(15.0f);
	RuntimeStatusText = TEXT("Tab: open RTS surface\nE: interact at table");
	StatusText->SetText(FText::FromString(RuntimeStatusText));

	OwnedTeamState = CreateDefaultSubobject<UArchonTeamRtsStateComponent>(TEXT("TeamRtsState"));
	TeamState = OwnedTeamState;

	OwnedTeamVisibilityState = CreateDefaultSubobject<UArchonTeamVisibilityStateComponent>(TEXT("TeamVisibilityState"));
	TeamVisibilityState = OwnedTeamVisibilityState;

	// The live diorama on the felt (WC3 hill: the table IS the RTS).
	// Sits just above the 2.2-scaled wooden table top (~75uu * 2.2).
	Miniature = CreateDefaultSubobject<UArchonMapTableMiniatureComponent>(TEXT("MapTableMiniature"));
	Miniature->SetupAttachment(SceneRoot);
	Miniature->SetRelativeLocation(FVector(0.0f, 0.0f, 170.0f));
}

void AArchonMapTableActor::ConfigureMapTable(int32 InTeamId, EArchonSessionRoute InSessionRoute, bool bInOwnsRtsExecutionExpansion)
{
	TeamId = InTeamId;
	SessionRoute = InSessionRoute;
	bOwnsRtsExecutionExpansion = bInOwnsRtsExecutionExpansion;
	if (TeamState)
	{
		TeamState->ConfigureTeamState(TeamId);
	}
	if (TeamVisibilityState)
	{
		TeamVisibilityState->ConfigureVisibilityState(TeamId, TeamVisibilityState->GetVisibilityCells());
	}

	if (Miniature)
	{
		Miniature->ConfigureMiniature(TeamId, TeamVisibilityState);
	}

	SetRuntimeStatusText(FString::Printf(TEXT("Team %d map table ready\nTab: open RTS surface\nE: interact at table\nB: field reinforcements"), TeamId));
}

void AArchonMapTableActor::AttachTeamState(UArchonTeamRtsStateComponent* InTeamState)
{
	TeamState = InTeamState;
}

void AArchonMapTableActor::AttachTeamVisibilityState(UArchonTeamVisibilityStateComponent* InTeamVisibilityState)
{
	TeamVisibilityState = InTeamVisibilityState;
}

void AArchonMapTableActor::SetRuntimeStatusText(const FString& InStatusText)
{
	RuntimeStatusText = InStatusText;
	if (StatusText)
	{
		StatusText->SetText(FText::FromString(RuntimeStatusText));
	}
}

int32 AArchonMapTableActor::GetCurrentCommandSequence() const
{
	return TeamState ? TeamState->GetCurrentCommandSequence() : 0;
}

FArchonMapTableDecision AArchonMapTableActor::EvaluateCommand(
	EArchonMapTableCommandKind CommandKind,
	const FArchonMapTableCommandContext& Context) const
{
	return UArchonMapTablePolicyLibrary::EvaluateMapTableCommand(
		SessionRoute,
		bOwnsRtsExecutionExpansion,
		CommandKind,
		Context);
}

bool AArchonMapTableActor::SubmitCommand(
	EArchonMapTableCommandKind CommandKind,
	const FArchonMapTableCommandContext& Context,
	const FArchonRtsCommandIntent& Intent,
	FArchonRtsAuthorityDecision& OutDecision) const
{
	if (!TeamState)
	{
		OutDecision = FArchonRtsAuthorityDecision();
		OutDecision.Reason = TEXT("missing_team_state");
		return false;
	}

	if (Intent.TeamId != TeamId)
	{
		OutDecision = FArchonRtsAuthorityDecision();
		OutDecision.Reason = TEXT("wrong_map_table_team");
		return false;
	}

	const FArchonMapTableDecision MapTableDecision = EvaluateCommand(CommandKind, Context);
	return TeamState->SubmitMapTableCommand(MapTableDecision, Intent, OutDecision);
}
