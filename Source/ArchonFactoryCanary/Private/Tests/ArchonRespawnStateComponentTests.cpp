#if WITH_DEV_AUTOMATION_TESTS

#include "ArchonRespawnStateComponent.h"
#include "Misc/AutomationTest.h"

namespace
{
	FArchonRespawnSpawnPoint MakeComponentSpawnPoint(
		FName SpawnPointId,
		int32 TeamId = 0,
		bool bIsForwardSpawn = false,
		const FVector& Location = FVector::ZeroVector)
	{
		FArchonRespawnSpawnPoint SpawnPoint;
		SpawnPoint.SpawnPointId = SpawnPointId;
		SpawnPoint.TeamId = TeamId;
		SpawnPoint.Location = Location;
		SpawnPoint.Rotation = FRotator::ZeroRotator;
		SpawnPoint.bIsForwardSpawn = bIsForwardSpawn;
		SpawnPoint.bIsAvailable = true;
		return SpawnPoint;
	}

	UArchonRespawnStateComponent* MakeRespawnStateComponent()
	{
		AActor* Owner = NewObject<AActor>();
		UArchonRespawnStateComponent* Respawn = NewObject<UArchonRespawnStateComponent>(Owner);
		Owner->AddOwnedComponent(Respawn);
		FArchonRespawnTuning Tuning;
		Respawn->ConfigureRespawn(0, 0, Tuning);
		Respawn->RegisterSpawnPoint(MakeComponentSpawnPoint(TEXT("base_spawn")));
		return Respawn;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonRespawnStateConfigureStartsAliveTest,
	"ArchonFactory.RespawnState.ConfigureStartsAlive",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonRespawnStateConfigureStartsAliveTest::RunTest(const FString& Parameters)
{
	UArchonRespawnStateComponent* Respawn = MakeRespawnStateComponent();

	TestEqual(TEXT("Configured respawn state starts alive"), Respawn->GetLifeState(), EArchonLifeState::Alive);
	TestTrue(TEXT("Alive state can issue map-table commands"), Respawn->MayIssueMapTableCommand());
	TestEqual(TEXT("Player id configured"), Respawn->GetPlayerId(), 0);
	TestEqual(TEXT("Team id configured"), Respawn->GetPlayerTeamId(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonRespawnStateRegisterSpawnPointIsIdempotentTest,
	"ArchonFactory.RespawnState.RegisterSpawnPointIsIdempotent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonRespawnStateRegisterSpawnPointIsIdempotentTest::RunTest(const FString& Parameters)
{
	UArchonRespawnStateComponent* Respawn = MakeRespawnStateComponent();
	Respawn->RegisterSpawnPoint(MakeComponentSpawnPoint(TEXT("base_spawn"), 0, false, FVector(100.0f, 0.0f, 0.0f)));

	const TArray<FArchonRespawnSpawnPoint> SpawnPoints = Respawn->GetAvailableSpawnPoints();

	TestEqual(TEXT("Duplicate spawn id replaces existing point"), SpawnPoints.Num(), 1);
	if (SpawnPoints.Num() > 0)
	{
		TestEqual(TEXT("Updated location is retained"), SpawnPoints[0].Location, FVector(100.0f, 0.0f, 0.0f));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonRespawnStateHandleDeathStartsTimerAndBroadcastsTest,
	"ArchonFactory.RespawnState.HandleDeathStartsTimerAndBroadcasts",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonRespawnStateHandleDeathStartsTimerAndBroadcastsTest::RunTest(const FString& Parameters)
{
	UArchonRespawnStateComponent* Respawn = MakeRespawnStateComponent();
	int32 LifeStateBroadcasts = 0;
	EArchonLifeState LastLifeState = EArchonLifeState::Alive;
	Respawn->OnLifeStateChanged.AddLambda([&LifeStateBroadcasts, &LastLifeState](EArchonLifeState NewLifeState)
	{
		++LifeStateBroadcasts;
		LastLifeState = NewLifeState;
	});

	const bool bHandled = Respawn->HandlePlayerDeath(1, 9);

	TestTrue(TEXT("Death starts respawn"), bHandled);
	TestEqual(TEXT("Respawn state is dead after death"), Respawn->GetLifeState(), EArchonLifeState::Dead);
	TestEqual(TEXT("Death selects default spawn"), Respawn->GetState().SelectedSpawnPointId, FName(TEXT("base_spawn")));
	TestEqual(TEXT("Timer starts at default seconds"), Respawn->GetState().SecondsUntilRespawn, 5.0f);
	TestEqual(TEXT("Life-state delegate broadcasts once"), LifeStateBroadcasts, 1);
	TestEqual(TEXT("Broadcast reports dead"), LastLifeState, EArchonLifeState::Dead);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonRespawnStateTickBroadcastsRespawnReadyTest,
	"ArchonFactory.RespawnState.TickBroadcastsRespawnReady",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonRespawnStateTickBroadcastsRespawnReadyTest::RunTest(const FString& Parameters)
{
	UArchonRespawnStateComponent* Respawn = MakeRespawnStateComponent();
	int32 ReadyBroadcasts = 0;
	FName ReadySpawnId = NAME_None;
	Respawn->OnRespawnReady.AddLambda([&ReadyBroadcasts, &ReadySpawnId](const FArchonRespawnSpawnPoint& SpawnPoint)
	{
		++ReadyBroadcasts;
		ReadySpawnId = SpawnPoint.SpawnPointId;
	});

	Respawn->HandlePlayerDeath(1, 9);
	Respawn->TickComponent(5.05f, LEVELTICK_All, nullptr);

	TestEqual(TEXT("Timer completion transitions to respawning"), Respawn->GetLifeState(), EArchonLifeState::Respawning);
	TestEqual(TEXT("Respawn-ready delegate broadcasts once"), ReadyBroadcasts, 1);
	TestEqual(TEXT("Ready spawn matches selected spawn"), ReadySpawnId, FName(TEXT("base_spawn")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonRespawnStateMarkRespawnCompleteReturnsAliveTest,
	"ArchonFactory.RespawnState.MarkRespawnCompleteReturnsAlive",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonRespawnStateMarkRespawnCompleteReturnsAliveTest::RunTest(const FString& Parameters)
{
	UArchonRespawnStateComponent* Respawn = MakeRespawnStateComponent();
	Respawn->HandlePlayerDeath(1, 9);
	Respawn->TickComponent(5.05f, LEVELTICK_All, nullptr);

	const bool bMarked = Respawn->MarkRespawnComplete();

	TestTrue(TEXT("Respawn completion is accepted from respawning state"), bMarked);
	TestEqual(TEXT("Respawn completion returns player to alive"), Respawn->GetLifeState(), EArchonLifeState::Alive);
	TestTrue(TEXT("Alive after respawn can issue map-table commands"), Respawn->MayIssueMapTableCommand());
	return true;
}

#endif
