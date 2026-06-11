#include "ArchonRespawnStateComponent.h"
#include "ArchonRespawnPolicyLibrary.h"
#include "Net/UnrealNetwork.h"

UArchonRespawnStateComponent::UArchonRespawnStateComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	SetIsReplicatedByDefault(true);
}

void UArchonRespawnStateComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UArchonRespawnStateComponent, PlayerId);
	DOREPLIFETIME(UArchonRespawnStateComponent, PlayerTeamId);
	DOREPLIFETIME(UArchonRespawnStateComponent, State);
	DOREPLIFETIME(UArchonRespawnStateComponent, Tuning);
	DOREPLIFETIME(UArchonRespawnStateComponent, SpawnPoints);
}

void UArchonRespawnStateComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	if (IsRegistered())
	{
		Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	}

	const EArchonLifeState PreviousLifeState = State.LifeState;
	if (!UArchonRespawnPolicyLibrary::AdvanceRespawnTimer(State, DeltaTime))
	{
		return;
	}

	if (PreviousLifeState != State.LifeState)
	{
		OnLifeStateChanged.Broadcast(State.LifeState);
	}

	FArchonRespawnSpawnPoint SelectedSpawnPoint;
	if (GetSelectedSpawnPoint(SelectedSpawnPoint))
	{
		OnRespawnReady.Broadcast(SelectedSpawnPoint);
	}
}

void UArchonRespawnStateComponent::ConfigureRespawn(
	int32 InPlayerId,
	int32 InPlayerTeamId,
	const FArchonRespawnTuning& InTuning)
{
	PlayerId = InPlayerId;
	PlayerTeamId = InPlayerTeamId;
	Tuning = InTuning;
	State = FArchonRespawnState();
}

void UArchonRespawnStateComponent::RegisterSpawnPoint(const FArchonRespawnSpawnPoint& SpawnPoint)
{
	for (FArchonRespawnSpawnPoint& ExistingSpawnPoint : SpawnPoints)
	{
		if (ExistingSpawnPoint.SpawnPointId == SpawnPoint.SpawnPointId)
		{
			ExistingSpawnPoint = SpawnPoint;
			return;
		}
	}

	SpawnPoints.Add(SpawnPoint);
}

bool UArchonRespawnStateComponent::HandlePlayerDeath(int32 KillerTeamId, int32 KillerPlayerId)
{
	if (!HasRespawnAuthority() || State.LifeState == EArchonLifeState::Dead || State.LifeState == EArchonLifeState::Respawning)
	{
		return false;
	}

	FArchonRespawnSpawnPoint SelectedSpawnPoint;
	if (!UArchonRespawnPolicyLibrary::SelectDefaultSpawnPoint(PlayerTeamId, SpawnPoints, SelectedSpawnPoint))
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("ArchonFactoryCanary: RespawnDeathHandled handled=false reason=no_spawn_points player=%d team=%d"),
			PlayerId,
			PlayerTeamId);
		return false;
	}

	const EArchonLifeState PreviousLifeState = State.LifeState;
	UArchonRespawnPolicyLibrary::BeginRespawn(State, SelectedSpawnPoint, Tuning);
	if (PreviousLifeState != State.LifeState)
	{
		OnLifeStateChanged.Broadcast(State.LifeState);
	}

	UE_LOG(
		LogTemp,
		Display,
		TEXT("ArchonFactoryCanary: RespawnDeathHandled handled=true player=%d team=%d killerTeam=%d killerPlayer=%d selected=%s seconds=%.1f"),
		PlayerId,
		PlayerTeamId,
		KillerTeamId,
		KillerPlayerId,
		*State.SelectedSpawnPointId.ToString(),
		State.SecondsUntilRespawn);
	return true;
}

bool UArchonRespawnStateComponent::RequestSpawnPointChoice(FName SpawnPointId)
{
	if (!HasRespawnAuthority() || State.LifeState != EArchonLifeState::Dead)
	{
		return false;
	}

	FArchonRespawnSpawnPoint RequestedSpawnPoint;
	if (!UArchonRespawnPolicyLibrary::CanPlayerSelectSpawnPoint(PlayerTeamId, SpawnPointId, SpawnPoints, RequestedSpawnPoint))
	{
		return false;
	}

	const float PreviousTotalSeconds = State.TotalRespawnSeconds;
	const float NewTotalSeconds = GetRespawnSecondsForSpawnPoint(RequestedSpawnPoint);
	if (NewTotalSeconds > PreviousTotalSeconds)
	{
		State.SecondsUntilRespawn += NewTotalSeconds - PreviousTotalSeconds;
	}

	State.TotalRespawnSeconds = NewTotalSeconds;
	State.SelectedSpawnPointId = RequestedSpawnPoint.SpawnPointId;
	return true;
}

bool UArchonRespawnStateComponent::MarkRespawnComplete()
{
	if (!HasRespawnAuthority() || State.LifeState == EArchonLifeState::Alive)
	{
		return false;
	}

	SetLifeState(EArchonLifeState::Alive);
	State.SecondsUntilRespawn = 0.0f;
	return true;
}

void UArchonRespawnStateComponent::SetLifeStateForProof(EArchonLifeState InLifeState)
{
	SetLifeState(InLifeState);
}

bool UArchonRespawnStateComponent::MayIssueMapTableCommand() const
{
	return UArchonRespawnPolicyLibrary::MayIssueMapTableCommandInLifeState(State.LifeState);
}

TArray<FArchonRespawnSpawnPoint> UArchonRespawnStateComponent::GetAvailableSpawnPoints() const
{
	TArray<FArchonRespawnSpawnPoint> AvailableSpawnPoints;
	for (const FArchonRespawnSpawnPoint& SpawnPoint : SpawnPoints)
	{
		if (SpawnPoint.TeamId == PlayerTeamId && SpawnPoint.bIsAvailable)
		{
			AvailableSpawnPoints.Add(SpawnPoint);
		}
	}
	return AvailableSpawnPoints;
}

bool UArchonRespawnStateComponent::GetSelectedSpawnPoint(FArchonRespawnSpawnPoint& OutSpawnPoint) const
{
	if (State.SelectedSpawnPointId.IsNone())
	{
		OutSpawnPoint = FArchonRespawnSpawnPoint();
		return false;
	}

	return UArchonRespawnPolicyLibrary::CanPlayerSelectSpawnPoint(
		PlayerTeamId,
		State.SelectedSpawnPointId,
		SpawnPoints,
		OutSpawnPoint);
}

bool UArchonRespawnStateComponent::HasRespawnAuthority() const
{
	const AActor* Owner = GetOwner();
	return !Owner || Owner->HasAuthority();
}

void UArchonRespawnStateComponent::SetLifeState(EArchonLifeState NewLifeState)
{
	if (State.LifeState == NewLifeState)
	{
		return;
	}

	State.LifeState = NewLifeState;
	OnLifeStateChanged.Broadcast(State.LifeState);
}

float UArchonRespawnStateComponent::GetRespawnSecondsForSpawnPoint(const FArchonRespawnSpawnPoint& SpawnPoint) const
{
	return FMath::Max(0.0f, Tuning.DefaultRespawnSeconds) +
		(SpawnPoint.bIsForwardSpawn ? FMath::Max(0.0f, Tuning.ForwardSpawnPenaltySeconds) : 0.0f);
}
