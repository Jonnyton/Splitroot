#if WITH_DEV_AUTOMATION_TESTS

#include "ArchonCombatHealthComponent.h"
#include "Misc/AutomationTest.h"

namespace
{
	UArchonCombatHealthComponent* MakeHealthComponent(float MaxHealth = 150.0f)
	{
		AActor* Owner = NewObject<AActor>();
		UArchonCombatHealthComponent* Health = NewObject<UArchonCombatHealthComponent>(Owner);
		Owner->AddOwnedComponent(Health);
		Health->ConfigureHealth(0, MaxHealth, 1.0f);
		return Health;
	}

	FArchonHitResult MakeAcceptedHit(float Damage)
	{
		FArchonHitResult Hit;
		Hit.bShouldHit = true;
		Hit.FinalDamage = Damage;
		Hit.HitType = EArchonHitType::Body;
		Hit.DamageType = EArchonDamageType::VerdantLivingArrow;
		Hit.InstigatorTeamId = 1;
		Hit.InstigatorPlayerId = 9;
		Hit.Reason = TEXT("accepted_combat_shot");
		return Hit;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonCombatConfigureHealthSetsStateTest,
	"ArchonFactory.CombatHealth.ConfigureHealthSetsState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonCombatConfigureHealthSetsStateTest::RunTest(const FString& Parameters)
{
	UArchonCombatHealthComponent* Health = MakeHealthComponent(80.0f);
	Health->ConfigureHealth(2, 120.0f, 0.75f);

	TestEqual(TEXT("Team id configured"), Health->GetTeamId(), 2);
	TestEqual(TEXT("Max health configured"), Health->GetMaxHealth(), 120.0f);
	TestEqual(TEXT("Current health restored to max"), Health->GetCurrentHealth(), 120.0f);
	TestEqual(TEXT("Armor modifier configured"), Health->GetArmorModifier(), 0.75f);
	TestEqual(TEXT("Hit counter resets"), Health->GetTotalHitsTaken(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonCombatApplyHitAcceptedReducesHealthTest,
	"ArchonFactory.CombatHealth.ApplyHitAcceptedReducesHealth",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonCombatApplyHitAcceptedReducesHealthTest::RunTest(const FString& Parameters)
{
	UArchonCombatHealthComponent* Health = MakeHealthComponent();
	const FArchonDamageApplicationResult Result = Health->ApplyHit(MakeAcceptedHit(35.0f));

	TestTrue(TEXT("Accepted hit applies"), Result.bAccepted);
	TestEqual(TEXT("Previous health recorded"), Result.PreviousHealth, 150.0f);
	TestEqual(TEXT("New health recorded"), Result.NewHealth, 115.0f);
	TestEqual(TEXT("Component current health mutates"), Health->GetCurrentHealth(), 115.0f);
	TestEqual(TEXT("Hit counter increments"), Health->GetTotalHitsTaken(), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonCombatRejectedHitDoesNotMutateTest,
	"ArchonFactory.CombatHealth.RejectedHitDoesNotMutate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonCombatRejectedHitDoesNotMutateTest::RunTest(const FString& Parameters)
{
	UArchonCombatHealthComponent* Health = MakeHealthComponent();
	FArchonHitResult Hit = MakeAcceptedHit(35.0f);
	Hit.bShouldHit = false;
	Hit.Reason = TEXT("friendly_fire_disabled");

	const FArchonDamageApplicationResult Result = Health->ApplyHit(Hit);
	TestFalse(TEXT("Rejected hit does not apply"), Result.bAccepted);
	TestEqual(TEXT("Health remains full"), Health->GetCurrentHealth(), 150.0f);
	TestEqual(TEXT("Hits remain zero"), Health->GetTotalHitsTaken(), 0);
	TestEqual(TEXT("Rejection reason propagates"), Result.Reason, FName(TEXT("friendly_fire_disabled")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonCombatDirectDamageCanKillTest,
	"ArchonFactory.CombatHealth.DirectDamageCanKill",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonCombatDirectDamageCanKillTest::RunTest(const FString& Parameters)
{
	UArchonCombatHealthComponent* Health = MakeHealthComponent(80.0f);
	const FArchonDamageApplicationResult Result = Health->ApplyDirectDamage(100.0f, EArchonDamageType::Environmental, INDEX_NONE, INDEX_NONE);

	TestTrue(TEXT("Direct damage accepted"), Result.bAccepted);
	TestTrue(TEXT("Direct damage caused death"), Result.bCausedDeath);
	TestEqual(TEXT("Health clamps at zero"), Health->GetCurrentHealth(), 0.0f);
	TestEqual(TEXT("Death counter increments"), Health->GetTotalDeaths(), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonCombatDeadTargetsRejectFurtherDamageTest,
	"ArchonFactory.CombatHealth.DeadTargetsRejectFurtherDamage",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonCombatDeadTargetsRejectFurtherDamageTest::RunTest(const FString& Parameters)
{
	UArchonCombatHealthComponent* Health = MakeHealthComponent(80.0f);
	Health->ApplyDirectDamage(100.0f, EArchonDamageType::Environmental, INDEX_NONE, INDEX_NONE);
	const FArchonDamageApplicationResult Result = Health->ApplyDirectDamage(10.0f, EArchonDamageType::Environmental, INDEX_NONE, INDEX_NONE);

	TestFalse(TEXT("Dead targets reject subsequent damage"), Result.bAccepted);
	TestEqual(TEXT("Already dead reason is stable"), Result.Reason, FName(TEXT("already_dead")));
	TestEqual(TEXT("Death counter does not double-count"), Health->GetTotalDeaths(), 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonCombatHealToFullRestoresHealthTest,
	"ArchonFactory.CombatHealth.HealToFullRestoresHealth",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonCombatHealToFullRestoresHealthTest::RunTest(const FString& Parameters)
{
	UArchonCombatHealthComponent* Health = MakeHealthComponent();
	Health->ApplyDirectDamage(35.0f, EArchonDamageType::Environmental, INDEX_NONE, INDEX_NONE);
	Health->HealToFull();

	TestEqual(TEXT("Heal restores current health to max"), Health->GetCurrentHealth(), Health->GetMaxHealth());
	TestTrue(TEXT("Health is alive after heal"), Health->IsAlive());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonCombatResetProofStateClearsCountersTest,
	"ArchonFactory.CombatHealth.ResetProofStateClearsCounters",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonCombatResetProofStateClearsCountersTest::RunTest(const FString& Parameters)
{
	UArchonCombatHealthComponent* Health = MakeHealthComponent();
	Health->ApplyDirectDamage(35.0f, EArchonDamageType::Environmental, INDEX_NONE, INDEX_NONE);
	Health->ResetProofState();

	TestEqual(TEXT("Reset restores health"), Health->GetCurrentHealth(), Health->GetMaxHealth());
	TestEqual(TEXT("Reset clears hit counter"), Health->GetTotalHitsTaken(), 0);
	TestEqual(TEXT("Reset clears death counter"), Health->GetTotalDeaths(), 0);
	TestFalse(TEXT("Reset clears last accepted damage"), Health->GetLastDamageApplication().bAccepted);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonCombatDeathDelegateBroadcastsOnceTest,
	"ArchonFactory.CombatHealth.DeathDelegateBroadcastsOnce",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonCombatDeathDelegateBroadcastsOnceTest::RunTest(const FString& Parameters)
{
	UArchonCombatHealthComponent* Health = MakeHealthComponent(80.0f);
	int32 DeathBroadcasts = 0;
	Health->OnDeath.AddLambda([&DeathBroadcasts](const FArchonDamageApplicationResult&)
	{
		++DeathBroadcasts;
	});

	Health->ApplyDirectDamage(100.0f, EArchonDamageType::Environmental, INDEX_NONE, INDEX_NONE);
	Health->ApplyDirectDamage(10.0f, EArchonDamageType::Environmental, INDEX_NONE, INDEX_NONE);

	TestEqual(TEXT("Death broadcasts once for one death"), DeathBroadcasts, 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonCombatHealthChangedBroadcastsOnDamageTest,
	"ArchonFactory.CombatHealth.HealthChangedBroadcastsOnDamage",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonCombatHealthChangedBroadcastsOnDamageTest::RunTest(const FString& Parameters)
{
	UArchonCombatHealthComponent* Health = MakeHealthComponent();
	int32 HealthBroadcasts = 0;
	Health->OnHealthChanged.AddLambda([&HealthBroadcasts](const FArchonDamageApplicationResult&)
	{
		++HealthBroadcasts;
	});

	Health->ApplyDirectDamage(35.0f, EArchonDamageType::Environmental, INDEX_NONE, INDEX_NONE);

	TestEqual(TEXT("Health changed broadcasts on accepted damage"), HealthBroadcasts, 1);
	return true;
}

#endif
