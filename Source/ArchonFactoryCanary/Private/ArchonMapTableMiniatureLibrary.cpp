#include "ArchonMapTableMiniatureLibrary.h"

FVector UArchonMapTableMiniatureLibrary::WorldToTableLocal(
	const FVector& WorldPoint,
	const FVector& WorldOrigin,
	const FVector& WorldExtents,
	float TableHalfX,
	float TableHalfY)
{
	const double SafeExtentX = FMath::Max(1.0, WorldExtents.X);
	const double SafeExtentY = FMath::Max(1.0, WorldExtents.Y);
	const double NormalizedX = FMath::Clamp((WorldPoint.X - WorldOrigin.X) / SafeExtentX, -1.0, 1.0);
	const double NormalizedY = FMath::Clamp((WorldPoint.Y - WorldOrigin.Y) / SafeExtentY, -1.0, 1.0);
	return FVector(NormalizedX * TableHalfX, NormalizedY * TableHalfY, 0.0);
}

bool UArchonMapTableMiniatureLibrary::IsWorldPointLit(
	const TArray<FArchonVisibilityGridCell>& Cells,
	const FVector& WorldPoint)
{
	if (Cells.Num() == 0)
	{
		return false;
	}
	const FArchonVisibilityGridCell* Nearest = nullptr;
	double NearestDistSq = TNumericLimits<double>::Max();
	for (const FArchonVisibilityGridCell& Cell : Cells)
	{
		const double DistSq = FVector::DistSquared2D(Cell.WorldCenter, WorldPoint);
		if (DistSq < NearestDistSq)
		{
			NearestDistSq = DistSq;
			Nearest = &Cell;
		}
	}
	return Nearest && Nearest->State == EArchonTeamVisibilityState::Lit;
}
