#include "ArchonTeamVisibilityStateComponent.h"
#include "ArchonTeamVisibilityPolicyLibrary.h"
#include "Net/UnrealNetwork.h"

UArchonTeamVisibilityStateComponent::UArchonTeamVisibilityStateComponent()
{
	SetIsReplicatedByDefault(true);
}

void UArchonTeamVisibilityStateComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UArchonTeamVisibilityStateComponent, TeamId);
	DOREPLIFETIME(UArchonTeamVisibilityStateComponent, VisibilityCells);
	DOREPLIFETIME(UArchonTeamVisibilityStateComponent, FriendlySources);
	DOREPLIFETIME(UArchonTeamVisibilityStateComponent, LastNewlyLitCells);
	DOREPLIFETIME(UArchonTeamVisibilityStateComponent, BuildingSnapshots);
}

void UArchonTeamVisibilityStateComponent::ConfigureVisibilityState(
	int32 InTeamId,
	const TArray<FArchonVisibilityGridCell>& InitialCells)
{
	TeamId = InTeamId;
	VisibilityCells = InitialCells;
	FriendlySources.Reset();
	LastNewlyLitCells.Reset();
	BuildingSnapshots.Reset();
}

void UArchonTeamVisibilityStateComponent::SetFriendlySources(const TArray<FArchonVisibilitySource>& InSources)
{
	FriendlySources.Reset(InSources.Num());
	for (const FArchonVisibilitySource& Source : InSources)
	{
		AddFriendlySource(Source);
	}
}

bool UArchonTeamVisibilityStateComponent::AddFriendlySource(const FArchonVisibilitySource& Source)
{
	if (!IsFriendlySource(Source))
	{
		return false;
	}

	FriendlySources.Add(Source);
	return true;
}

void UArchonTeamVisibilityStateComponent::RecomputeVisibility()
{
	TArray<FArchonVisibilityGridCell> NextCells;
	UArchonTeamVisibilityPolicyLibrary::RecomputeVisibilityGrid(
		VisibilityCells,
		FriendlySources,
		NextCells,
		LastNewlyLitCells);

	VisibilityCells = NextCells;
}

void UArchonTeamVisibilityStateComponent::ResetProofState()
{
	FriendlySources.Reset();
	LastNewlyLitCells.Reset();
	BuildingSnapshots.Reset();
}

bool UArchonTeamVisibilityStateComponent::ApplyBuildingVisionReport(
	const FArchonBuildingVisionReport& Report,
	FArchonBuildingSnapshot& OutSnapshot)
{
	if (Report.BuildingId.IsNone())
	{
		OutSnapshot = FArchonBuildingSnapshot();
		return false;
	}

	const FArchonBuildingSnapshot* ExistingSnapshot = FindBuildingSnapshot(Report.BuildingId);
	const FArchonBuildingSnapshot PreviousSnapshot = ExistingSnapshot ? *ExistingSnapshot : FArchonBuildingSnapshot();
	const FArchonBuildingSnapshot NextSnapshot =
		UArchonTeamVisibilityPolicyLibrary::ResolveBuildingSnapshot(PreviousSnapshot, Report);

	if (!NextSnapshot.bHasSnapshot)
	{
		OutSnapshot = FArchonBuildingSnapshot();
		return false;
	}

	if (FArchonBuildingSnapshot* MutableSnapshot = FindMutableBuildingSnapshot(Report.BuildingId))
	{
		*MutableSnapshot = NextSnapshot;
	}
	else
	{
		BuildingSnapshots.Add(NextSnapshot);
	}

	OutSnapshot = NextSnapshot;
	return true;
}

bool UArchonTeamVisibilityStateComponent::GetBuildingSnapshot(
	FName BuildingId,
	FArchonBuildingSnapshot& OutSnapshot) const
{
	if (const FArchonBuildingSnapshot* Snapshot = FindBuildingSnapshot(BuildingId))
	{
		OutSnapshot = *Snapshot;
		return Snapshot->bHasSnapshot;
	}

	OutSnapshot = FArchonBuildingSnapshot();
	return false;
}

EArchonTeamVisibilityState UArchonTeamVisibilityStateComponent::GetCellState(FIntPoint Cell) const
{
	for (const FArchonVisibilityGridCell& GridCell : VisibilityCells)
	{
		if (GridCell.Cell == Cell)
		{
			return GridCell.State;
		}
	}

	return EArchonTeamVisibilityState::Black;
}

int32 UArchonTeamVisibilityStateComponent::CountCellsInState(EArchonTeamVisibilityState State) const
{
	int32 Count = 0;
	for (const FArchonVisibilityGridCell& GridCell : VisibilityCells)
	{
		if (GridCell.State == State)
		{
			++Count;
		}
	}

	return Count;
}

bool UArchonTeamVisibilityStateComponent::IsFriendlySource(const FArchonVisibilitySource& Source) const
{
	return TeamId != INDEX_NONE
		&& Source.TeamId == TeamId
		&& Source.Radius > 0.0f;
}

FArchonBuildingSnapshot* UArchonTeamVisibilityStateComponent::FindMutableBuildingSnapshot(FName BuildingId)
{
	return BuildingSnapshots.FindByPredicate(
		[BuildingId](const FArchonBuildingSnapshot& Snapshot)
		{
			return Snapshot.BuildingId == BuildingId;
		});
}

const FArchonBuildingSnapshot* UArchonTeamVisibilityStateComponent::FindBuildingSnapshot(FName BuildingId) const
{
	return BuildingSnapshots.FindByPredicate(
		[BuildingId](const FArchonBuildingSnapshot& Snapshot)
		{
			return Snapshot.BuildingId == BuildingId;
		});
}
