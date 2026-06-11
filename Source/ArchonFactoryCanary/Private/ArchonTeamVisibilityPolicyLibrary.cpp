#include "ArchonTeamVisibilityPolicyLibrary.h"

bool UArchonTeamVisibilityPolicyLibrary::IsPointLitBySource(
	const FVector& Point,
	const FArchonVisibilitySource& Source)
{
	if (Source.TeamId == INDEX_NONE || Source.Radius <= 0.0f)
	{
		return false;
	}

	return FVector::DistSquared2D(Point, Source.Location) <= FMath::Square(Source.Radius);
}

EArchonTeamVisibilityState UArchonTeamVisibilityPolicyLibrary::ResolveCellVisibility(
	const FVector& CellWorldCenter,
	EArchonTeamVisibilityState PreviousState,
	const TArray<FArchonVisibilitySource>& FriendlySources)
{
	for (const FArchonVisibilitySource& Source : FriendlySources)
	{
		if (IsPointLitBySource(CellWorldCenter, Source))
		{
			return EArchonTeamVisibilityState::Lit;
		}
	}

	return PreviousState == EArchonTeamVisibilityState::Black
		? EArchonTeamVisibilityState::Black
		: EArchonTeamVisibilityState::Fog;
}

void UArchonTeamVisibilityPolicyLibrary::RecomputeVisibilityGrid(
	const TArray<FArchonVisibilityGridCell>& PreviousCells,
	const TArray<FArchonVisibilitySource>& FriendlySources,
	TArray<FArchonVisibilityGridCell>& OutCells,
	TArray<FIntPoint>& OutNewlyLitCells)
{
	OutCells.Reset(PreviousCells.Num());
	OutNewlyLitCells.Reset();

	for (const FArchonVisibilityGridCell& PreviousCell : PreviousCells)
	{
		FArchonVisibilityGridCell NextCell = PreviousCell;
		NextCell.State = ResolveCellVisibility(PreviousCell.WorldCenter, PreviousCell.State, FriendlySources);

		if (PreviousCell.State != EArchonTeamVisibilityState::Lit && NextCell.State == EArchonTeamVisibilityState::Lit)
		{
			OutNewlyLitCells.Add(NextCell.Cell);
		}

		OutCells.Add(NextCell);
	}
}

FArchonBuildingSnapshot UArchonTeamVisibilityPolicyLibrary::ResolveBuildingSnapshot(
	const FArchonBuildingSnapshot& PreviousSnapshot,
	const FArchonBuildingVisionReport& CurrentReport)
{
	if (!CurrentReport.bCurrentlyVisible)
	{
		return PreviousSnapshot;
	}

	FArchonBuildingSnapshot Snapshot;
	Snapshot.BuildingId = CurrentReport.BuildingId;
	Snapshot.ObservedTeamId = CurrentReport.ObservedTeamId;
	Snapshot.LastKnownLocation = CurrentReport.Location;
	Snapshot.LastKnownState = CurrentReport.ObservedState;
	Snapshot.LastObservedSequence = CurrentReport.ObservationSequence;
	Snapshot.bHasSnapshot = true;
	return Snapshot;
}

bool UArchonTeamVisibilityPolicyLibrary::ShouldReplicateActorBasedOnVisibility(
	int32 ViewingTeamId,
	int32 ActorTeamId,
	EArchonTeamVisibilityState ActorCellState)
{
	// Own-team actors always replicate to their owning team. (The team
	// owns its own state; vision is not a gate on seeing your own units.)
	if (ViewingTeamId != INDEX_NONE && ActorTeamId == ViewingTeamId)
	{
		return true;
	}

	// Cross-team actors replicate only when the viewing team has current
	// vision on the actor's cell. Fog (last-seen) and Black (unexplored)
	// both withhold the actor — standard WC3/SC1 fog discipline.
	return ActorCellState == EArchonTeamVisibilityState::Lit;
}
