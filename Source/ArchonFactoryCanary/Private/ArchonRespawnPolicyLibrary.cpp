#include "ArchonRespawnPolicyLibrary.h"

namespace
{
	float GetRespawnSecondsForSpawnPoint(
		const FArchonRespawnSpawnPoint& SpawnPoint,
		const FArchonRespawnTuning& Tuning)
	{
		return FMath::Max(0.0f, Tuning.DefaultRespawnSeconds) +
			(SpawnPoint.bIsForwardSpawn ? FMath::Max(0.0f, Tuning.ForwardSpawnPenaltySeconds) : 0.0f);
	}
}

bool UArchonRespawnPolicyLibrary::SelectDefaultSpawnPoint(
	int32 PlayerTeamId,
	const TArray<FArchonRespawnSpawnPoint>& SpawnPoints,
	FArchonRespawnSpawnPoint& OutSpawnPoint)
{
	TArray<FArchonRespawnSpawnPoint> Candidates;
	for (const FArchonRespawnSpawnPoint& SpawnPoint : SpawnPoints)
	{
		if (SpawnPoint.TeamId == PlayerTeamId && SpawnPoint.bIsAvailable)
		{
			Candidates.Add(SpawnPoint);
		}
	}

	if (Candidates.IsEmpty())
	{
		OutSpawnPoint = FArchonRespawnSpawnPoint();
		return false;
	}

	Candidates.Sort([](const FArchonRespawnSpawnPoint& Left, const FArchonRespawnSpawnPoint& Right)
	{
		if (Left.bIsForwardSpawn != Right.bIsForwardSpawn)
		{
			return !Left.bIsForwardSpawn;
		}

		return Left.SpawnPointId.ToString() < Right.SpawnPointId.ToString();
	});

	OutSpawnPoint = Candidates[0];
	return true;
}

bool UArchonRespawnPolicyLibrary::CanPlayerSelectSpawnPoint(
	int32 PlayerTeamId,
	FName SpawnPointId,
	const TArray<FArchonRespawnSpawnPoint>& SpawnPoints,
	FArchonRespawnSpawnPoint& OutSpawnPoint)
{
	for (const FArchonRespawnSpawnPoint& SpawnPoint : SpawnPoints)
	{
		if (SpawnPoint.SpawnPointId == SpawnPointId &&
			SpawnPoint.TeamId == PlayerTeamId &&
			SpawnPoint.bIsAvailable)
		{
			OutSpawnPoint = SpawnPoint;
			return true;
		}
	}

	OutSpawnPoint = FArchonRespawnSpawnPoint();
	return false;
}

void UArchonRespawnPolicyLibrary::BeginRespawn(
	FArchonRespawnState& State,
	const FArchonRespawnSpawnPoint& SelectedSpawnPoint,
	const FArchonRespawnTuning& Tuning)
{
	const float TotalRespawnSeconds = GetRespawnSecondsForSpawnPoint(SelectedSpawnPoint, Tuning);
	State.LifeState = EArchonLifeState::Dead;
	State.SecondsUntilRespawn = TotalRespawnSeconds;
	State.TotalRespawnSeconds = TotalRespawnSeconds;
	State.SelectedSpawnPointId = SelectedSpawnPoint.SpawnPointId;
	++State.LivesLostThisMatch;
}

bool UArchonRespawnPolicyLibrary::AdvanceRespawnTimer(FArchonRespawnState& State, float DeltaSeconds)
{
	if (State.LifeState != EArchonLifeState::Dead)
	{
		return false;
	}

	State.SecondsUntilRespawn = FMath::Max(0.0f, State.SecondsUntilRespawn - FMath::Max(0.0f, DeltaSeconds));
	if (State.SecondsUntilRespawn > 0.0f)
	{
		return false;
	}

	State.LifeState = EArchonLifeState::Respawning;
	return true;
}

bool UArchonRespawnPolicyLibrary::MayIssueMapTableCommandInLifeState(EArchonLifeState LifeState)
{
	return LifeState == EArchonLifeState::Alive || LifeState == EArchonLifeState::Dead;
}
