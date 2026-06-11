#include "ArchonMapTableWidget.h"
#include "ArchonMapTableActor.h"
#include "ArchonTeamVisibilityStateComponent.h"

void UArchonMapTableWidget::ConfigureMapTableWidget(
	AArchonMapTableActor* InMapTable,
	int32 InPlayerId,
	int32 InTeamId,
	const FVector& InWorldOrigin,
	const FVector& InWorldExtents)
{
	MapTable = InMapTable;
	PlayerId = InPlayerId;
	TeamId = InTeamId;
	WorldOrigin = InWorldOrigin;
	WorldExtents = InWorldExtents;
	SelectedSquadIds.Reset();
	bHasSubmittedOrder = false;
	LastOrderTargetLocation = FVector::ZeroVector;
}

void UArchonMapTableWidget::SetSelectableSquadCandidates(
	const TArray<FArchonSelectableSquadCandidate>& InCandidates)
{
	SelectableSquadCandidates = InCandidates;
}

bool UArchonMapTableWidget::CommitDragBox(const FVector2D& StartCorner, const FVector2D& EndCorner)
{
	const FArchonDragBox Box = UArchonMapTableSelectionPolicyLibrary::NormalizeDragBox(StartCorner, EndCorner);
	UArchonMapTableSelectionPolicyLibrary::ComputeSelection(
		Box,
		TeamId,
		SelectableSquadCandidates,
		SelectedSquadIds);

	return SelectedSquadIds.Num() > 0;
}

bool UArchonMapTableWidget::CommitClickSelection(const FVector2D& ClickPoint, float ClickRadius)
{
	UArchonMapTableSelectionPolicyLibrary::ResolveClickSelection(
		ClickPoint,
		ClickRadius,
		TeamId,
		SelectableSquadCandidates,
		SelectedSquadIds);

	return SelectedSquadIds.Num() > 0;
}

bool UArchonMapTableWidget::HandleRightClickOrder(
	const FVector2D& TableSpacePoint,
	FName TargetId,
	FArchonRtsAuthorityDecision& OutDecision)
{
	if (!MapTable || SelectedSquadIds.Num() == 0)
	{
		OutDecision = FArchonRtsAuthorityDecision();
		OutDecision.Reason = TEXT("map_table_widget_not_ready");
		return false;
	}

	LastOrderTargetLocation = UArchonMapTableSelectionPolicyLibrary::TableSpaceToWorld(
		TableSpacePoint,
		WorldOrigin,
		WorldExtents);

	FArchonRtsCommandIntent Intent;
	Intent.TeamId = TeamId;
	Intent.IssuingPlayerId = PlayerId;
	Intent.OrderKind = EArchonRtsOrderKind::MoveSquad;
	Intent.SubjectId = ResolvePrimarySelectedSquadId();
	Intent.TargetId = TargetId;
	Intent.TargetLocation = LastOrderTargetLocation;
	Intent.bTargetLocationValid = true;

	bHasSubmittedOrder = MapTable->SubmitCommand(
		EArchonMapTableCommandKind::ExecuteOrder,
		FArchonMapTableCommandContext(),
		Intent,
		OutDecision);

	if (bHasSubmittedOrder)
	{
		UE_LOG(
			LogTemp,
			Display,
			TEXT("ArchonFactoryCanary: MapTableWidgetOrder submitted=true selected=%s target=%s"),
			*Intent.SubjectId.ToString(),
			*TargetId.ToString());
	}

	return bHasSubmittedOrder;
}

bool UArchonMapTableWidget::HandleRallyPointOrder(
	const FVector2D& TableSpacePoint,
	FName TargetId,
	FArchonRtsAuthorityDecision& OutDecision)
{
	if (!MapTable)
	{
		OutDecision = FArchonRtsAuthorityDecision();
		OutDecision.Reason = TEXT("map_table_widget_not_ready");
		return false;
	}

	LastOrderTargetLocation = UArchonMapTableSelectionPolicyLibrary::TableSpaceToWorld(
		TableSpacePoint,
		WorldOrigin,
		WorldExtents);

	FArchonRtsCommandIntent Intent;
	Intent.TeamId = TeamId;
	Intent.IssuingPlayerId = PlayerId;
	Intent.OrderKind = EArchonRtsOrderKind::SetRallyPoint;
	Intent.SubjectId = TEXT("team_rally");
	Intent.TargetId = TargetId;
	Intent.TargetLocation = LastOrderTargetLocation;
	Intent.bTargetLocationValid = true;

	bHasSubmittedOrder = MapTable->SubmitCommand(
		EArchonMapTableCommandKind::ExecuteOrder,
		FArchonMapTableCommandContext(),
		Intent,
		OutDecision);

	if (bHasSubmittedOrder)
	{
		UE_LOG(
			LogTemp,
			Display,
			TEXT("ArchonFactoryCanary: MapTableRallyPoint submitted=true target=%s location=%s"),
			*TargetId.ToString(),
			*Intent.TargetLocation.ToCompactString());
	}

	return bHasSubmittedOrder;
}

FString UArchonMapTableWidget::FormatVisibilitySummary() const
{
	const UArchonTeamVisibilityStateComponent* VisibilityState =
		MapTable ? MapTable->GetTeamVisibilityState() : nullptr;
	if (!VisibilityState)
	{
		return TEXT("VISION unavailable");
	}

	return FString::Printf(
		TEXT("VISION lit=%d fog=%d black=%d newlyLit=%d"),
		VisibilityState->CountCellsInState(EArchonTeamVisibilityState::Lit),
		VisibilityState->CountCellsInState(EArchonTeamVisibilityState::Fog),
		VisibilityState->CountCellsInState(EArchonTeamVisibilityState::Black),
		VisibilityState->GetLastNewlyLitCellCount());
}

FName UArchonMapTableWidget::ResolvePrimarySelectedSquadId() const
{
	return SelectedSquadIds.Num() > 0 ? SelectedSquadIds[0] : NAME_None;
}
