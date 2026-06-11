#include "ArchonMapTableSelectionPolicyLibrary.h"

FArchonDragBox UArchonMapTableSelectionPolicyLibrary::NormalizeDragBox(
	const FVector2D& StartCorner,
	const FVector2D& EndCorner)
{
	FArchonDragBox Box;
	Box.Min.X = FMath::Min(StartCorner.X, EndCorner.X);
	Box.Min.Y = FMath::Min(StartCorner.Y, EndCorner.Y);
	Box.Max.X = FMath::Max(StartCorner.X, EndCorner.X);
	Box.Max.Y = FMath::Max(StartCorner.Y, EndCorner.Y);
	return Box;
}

bool UArchonMapTableSelectionPolicyLibrary::BoxContainsPoint(const FArchonDragBox& Box, const FVector2D& Point)
{
	return Point.X >= Box.Min.X && Point.X <= Box.Max.X
		&& Point.Y >= Box.Min.Y && Point.Y <= Box.Max.Y;
}

bool UArchonMapTableSelectionPolicyLibrary::IsDragBoxDegenerate(const FArchonDragBox& Box)
{
	const double Width = Box.Max.X - Box.Min.X;
	const double Height = Box.Max.Y - Box.Min.Y;
	return Width * Height <= 1e-6;
}

void UArchonMapTableSelectionPolicyLibrary::ComputeSelection(
	const FArchonDragBox& Box,
	int32 IssuingTeamId,
	const TArray<FArchonSelectableSquadCandidate>& Candidates,
	TArray<FName>& OutSelectedSquadIds)
{
	OutSelectedSquadIds.Reset();
	if (IsDragBoxDegenerate(Box))
	{
		return;
	}
	for (const FArchonSelectableSquadCandidate& Candidate : Candidates)
	{
		if (Candidate.TeamId != IssuingTeamId)
		{
			continue;
		}
		if (!Candidate.bIsAlive)
		{
			continue;
		}
		if (BoxContainsPoint(Box, Candidate.TableSpacePosition))
		{
			OutSelectedSquadIds.Add(Candidate.SquadId);
		}
	}
}

void UArchonMapTableSelectionPolicyLibrary::ResolveClickSelection(
	const FVector2D& ClickPoint,
	float ClickRadius,
	int32 IssuingTeamId,
	const TArray<FArchonSelectableSquadCandidate>& Candidates,
	TArray<FName>& OutSelectedSquadIds)
{
	OutSelectedSquadIds.Reset();
	const float ClickRadiusSquared = ClickRadius * ClickRadius;

	const FArchonSelectableSquadCandidate* Best = nullptr;
	float BestDistanceSquared = TNumericLimits<float>::Max();

	for (const FArchonSelectableSquadCandidate& Candidate : Candidates)
	{
		if (Candidate.TeamId != IssuingTeamId)
		{
			continue;
		}
		if (!Candidate.bIsAlive)
		{
			continue;
		}
		const float DistanceSquared = FVector2D::DistSquared(Candidate.TableSpacePosition, ClickPoint);
		if (DistanceSquared > ClickRadiusSquared)
		{
			continue;
		}
		if (DistanceSquared < BestDistanceSquared)
		{
			Best = &Candidate;
			BestDistanceSquared = DistanceSquared;
		}
		else if (FMath::IsNearlyEqual(DistanceSquared, BestDistanceSquared, KINDA_SMALL_NUMBER))
		{
			// Stable tie-break: lexicographically smaller squad id wins.
			if (Best == nullptr || Candidate.SquadId.LexicalLess(Best->SquadId))
			{
				Best = &Candidate;
				BestDistanceSquared = DistanceSquared;
			}
		}
	}

	if (Best != nullptr)
	{
		OutSelectedSquadIds.Add(Best->SquadId);
	}
}

FVector UArchonMapTableSelectionPolicyLibrary::TableSpaceToWorld(
	const FVector2D& TableSpacePoint,
	const FVector& WorldOrigin,
	const FVector& WorldExtents)
{
	return WorldOrigin + FVector(TableSpacePoint.X * WorldExtents.X, TableSpacePoint.Y * WorldExtents.Y, 0.0f);
}

FVector2D UArchonMapTableSelectionPolicyLibrary::WorldToTableSpace(
	const FVector& WorldPoint,
	const FVector& WorldOrigin,
	const FVector& WorldExtents)
{
	const FVector Local = WorldPoint - WorldOrigin;
	const float X = WorldExtents.X != 0.0f ? Local.X / WorldExtents.X : 0.0f;
	const float Y = WorldExtents.Y != 0.0f ? Local.Y / WorldExtents.Y : 0.0f;
	return FVector2D(X, Y);
}
