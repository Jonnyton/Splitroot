#if WITH_DEV_AUTOMATION_TESTS

#include "ArchonRespawnPolicyLibrary.h"
#include "Misc/AutomationTest.h"

namespace
{
	FArchonRespawnSpawnPoint MakeRespawnPoint(
		FName SpawnPointId,
		int32 TeamId,
		bool bIsForwardSpawn,
		bool bIsAvailable = true)
	{
		FArchonRespawnSpawnPoint SpawnPoint;
		SpawnPoint.SpawnPointId = SpawnPointId;
		SpawnPoint.TeamId = TeamId;
		SpawnPoint.Location = FVector::ZeroVector;
		SpawnPoint.Rotation = FRotator::ZeroRotator;
		SpawnPoint.bIsForwardSpawn = bIsForwardSpawn;
		SpawnPoint.bIsAvailable = bIsAvailable;
		return SpawnPoint;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonRespawnSelectDefaultSpawnPicksBaseSpawnTest,
	"ArchonFactory.Respawn.SelectDefaultSpawnPicksBaseSpawn",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonRespawnSelectDefaultSpawnPicksBaseSpawnTest::RunTest(const FString& Parameters)
{
	TArray<FArchonRespawnSpawnPoint> SpawnPoints;
	SpawnPoints.Add(MakeRespawnPoint(TEXT("alpha_forward"), 0, true));
	SpawnPoints.Add(MakeRespawnPoint(TEXT("bravo_base"), 0, false));

	FArchonRespawnSpawnPoint Selected;
	const bool bSelected = UArchonRespawnPolicyLibrary::SelectDefaultSpawnPoint(0, SpawnPoints, Selected);

	TestTrue(TEXT("A valid team spawn should be selected"), bSelected);
	TestEqual(TEXT("Base spawn is preferred over forward spawn"), Selected.SpawnPointId, FName(TEXT("bravo_base")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonRespawnSelectDefaultSpawnRejectsUnavailableAndEnemyTest,
	"ArchonFactory.Respawn.SelectDefaultSpawnRejectsUnavailableAndEnemy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonRespawnSelectDefaultSpawnRejectsUnavailableAndEnemyTest::RunTest(const FString& Parameters)
{
	TArray<FArchonRespawnSpawnPoint> SpawnPoints;
	SpawnPoints.Add(MakeRespawnPoint(TEXT("enemy_base"), 1, false));
	SpawnPoints.Add(MakeRespawnPoint(TEXT("own_base_unavailable"), 0, false, false));
	SpawnPoints.Add(MakeRespawnPoint(TEXT("own_forward"), 0, true));

	FArchonRespawnSpawnPoint Selected;
	const bool bSelected = UArchonRespawnPolicyLibrary::SelectDefaultSpawnPoint(0, SpawnPoints, Selected);

	TestTrue(TEXT("Available own-team forward spawn is selected when base is unavailable"), bSelected);
	TestEqual(TEXT("Enemy and unavailable spawns are ignored"), Selected.SpawnPointId, FName(TEXT("own_forward")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonRespawnCanSelectRequestedSpawnForTeamTest,
	"ArchonFactory.Respawn.CanSelectRequestedSpawnForTeam",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonRespawnCanSelectRequestedSpawnForTeamTest::RunTest(const FString& Parameters)
{
	TArray<FArchonRespawnSpawnPoint> SpawnPoints;
	SpawnPoints.Add(MakeRespawnPoint(TEXT("own_base"), 0, false));
	SpawnPoints.Add(MakeRespawnPoint(TEXT("enemy_forward"), 1, true));

	FArchonRespawnSpawnPoint Selected;
	const bool bCanSelectOwn = UArchonRespawnPolicyLibrary::CanPlayerSelectSpawnPoint(0, TEXT("own_base"), SpawnPoints, Selected);
	const bool bCanSelectEnemy = UArchonRespawnPolicyLibrary::CanPlayerSelectSpawnPoint(0, TEXT("enemy_forward"), SpawnPoints, Selected);

	TestTrue(TEXT("Own-team spawn can be selected"), bCanSelectOwn);
	TestFalse(TEXT("Enemy spawn cannot be selected"), bCanSelectEnemy);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonRespawnBeginRespawnStartsDeadStateWithFiveSecondTimerTest,
	"ArchonFactory.Respawn.BeginRespawnStartsDeadStateWithFiveSecondTimer",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonRespawnBeginRespawnStartsDeadStateWithFiveSecondTimerTest::RunTest(const FString& Parameters)
{
	FArchonRespawnState State;
	FArchonRespawnTuning Tuning;
	const FArchonRespawnSpawnPoint SpawnPoint = MakeRespawnPoint(TEXT("own_base"), 0, false);

	UArchonRespawnPolicyLibrary::BeginRespawn(State, SpawnPoint, Tuning);

	TestEqual(TEXT("Respawn begins in dead state"), State.LifeState, EArchonLifeState::Dead);
	TestEqual(TEXT("Default respawn timer is five seconds"), State.SecondsUntilRespawn, 5.0f);
	TestEqual(TEXT("Total respawn timer is recorded"), State.TotalRespawnSeconds, 5.0f);
	TestEqual(TEXT("Selected spawn is stored"), State.SelectedSpawnPointId, FName(TEXT("own_base")));
	TestEqual(TEXT("Lives lost increments"), State.LivesLostThisMatch, 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonRespawnForwardSpawnAddsPenaltySecondsTest,
	"ArchonFactory.Respawn.ForwardSpawnAddsPenaltySeconds",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonRespawnForwardSpawnAddsPenaltySecondsTest::RunTest(const FString& Parameters)
{
	FArchonRespawnState State;
	FArchonRespawnTuning Tuning;
	const FArchonRespawnSpawnPoint SpawnPoint = MakeRespawnPoint(TEXT("own_forward"), 0, true);

	UArchonRespawnPolicyLibrary::BeginRespawn(State, SpawnPoint, Tuning);

	TestEqual(TEXT("Forward spawn includes penalty"), State.TotalRespawnSeconds, 6.5f);
	TestEqual(TEXT("Forward spawn timer starts at penalized total"), State.SecondsUntilRespawn, 6.5f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonRespawnAdvanceTimerTransitionsToRespawningTest,
	"ArchonFactory.Respawn.AdvanceTimerTransitionsToRespawning",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonRespawnAdvanceTimerTransitionsToRespawningTest::RunTest(const FString& Parameters)
{
	FArchonRespawnState State;
	State.LifeState = EArchonLifeState::Dead;
	State.SecondsUntilRespawn = 0.25f;
	State.TotalRespawnSeconds = 5.0f;

	const bool bReady = UArchonRespawnPolicyLibrary::AdvanceRespawnTimer(State, 0.30f);

	TestTrue(TEXT("Timer reports respawn ready"), bReady);
	TestEqual(TEXT("Life state transitions to respawning"), State.LifeState, EArchonLifeState::Respawning);
	TestEqual(TEXT("Timer clamps at zero"), State.SecondsUntilRespawn, 0.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonRespawnMayIssueMapTableCommandByLifeStateTest,
	"ArchonFactory.Respawn.MayIssueMapTableCommandByLifeState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonRespawnMayIssueMapTableCommandByLifeStateTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("Alive players may issue map-table commands"), UArchonRespawnPolicyLibrary::MayIssueMapTableCommandInLifeState(EArchonLifeState::Alive));
	TestFalse(TEXT("Dying players may not issue map-table commands"), UArchonRespawnPolicyLibrary::MayIssueMapTableCommandInLifeState(EArchonLifeState::Dying));
	TestTrue(TEXT("Dead players may issue map-table commands"), UArchonRespawnPolicyLibrary::MayIssueMapTableCommandInLifeState(EArchonLifeState::Dead));
	TestFalse(TEXT("Respawning players may not issue map-table commands"), UArchonRespawnPolicyLibrary::MayIssueMapTableCommandInLifeState(EArchonLifeState::Respawning));
	return true;
}

#endif
