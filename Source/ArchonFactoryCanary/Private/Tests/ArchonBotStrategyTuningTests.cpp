#include "ArchonBotStrategyTuningLibrary.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonBotStrategyTuningDefaultsTest,
	"ArchonFactory.Bot.StrategyTuningDefaultsMatchCurrentCanary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FArchonBotStrategyTuningDefaultsTest::RunTest(const FString& Parameters)
{
	const FArchonBotStrategyTuning Tuning;

	TestEqual(TEXT("Default role cycle keeps every third bot as breacher"), Tuning.RoleCycle.Num(), 3);
	TestEqual(TEXT("Bot 0 is fighter"), UArchonBotStrategyTuningLibrary::GetRoleForBotIndex(0, Tuning), FName(TEXT("fighter")));
	TestEqual(TEXT("Bot 2 is breacher"), UArchonBotStrategyTuningLibrary::GetRoleForBotIndex(2, Tuning), FName(TEXT("breacher")));
	TestTrue(TEXT("Default fighter range matches current canary"), FMath::IsNearlyEqual(Tuning.FighterEngageRange, 1400.0f));
	TestTrue(TEXT("Default breacher range matches current canary"), FMath::IsNearlyEqual(Tuning.BreacherEngageRange, 500.0f));
	TestTrue(TEXT("Default breacher pursuit memory mirrors current canary"), FMath::IsNearlyEqual(Tuning.BreacherPursuitMemoryWindowSeconds, 8.0f));
	TestTrue(TEXT("Default breacher threat window mirrors current canary"), FMath::IsNearlyEqual(Tuning.BreacherThreatWindowSeconds, 10.0f));
	TestTrue(TEXT("Default unstick lateral matches current canary"), FMath::IsNearlyEqual(Tuning.UnstickLateralDistance, 650.0f));
	TestEqual(TEXT("Default route stall threshold matches current canary"), Tuning.RouteStallMaxUnstickAttempts, 8);
	TestEqual(TEXT("Default objective lane shift threshold matches current canary"), Tuning.ObjectiveStallLaneShiftAttempts, 6);
	TestEqual(TEXT("Default objective lane shift cap matches current canary"), Tuning.ObjectiveStallMaxLaneShift, 3);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonBotStrategyTuningJsonParseTest,
	"ArchonFactory.Bot.StrategyTuningJsonFeedsBotRolesAndMovement",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FArchonBotStrategyTuningJsonParseTest::RunTest(const FString& Parameters)
{
	const FString JsonText = TEXT(R"JSON(
{
  "schema": "tinyassets.game_factory.bot_strategy_tuning.v1",
  "revision": "test-live-tuning",
  "roles": {
    "role_cycle": ["breacher", "fighter"],
    "fighter_engage_range": 1200.0,
    "breacher_engage_range": 420.0,
    "breacher_pursuit_memory_window_seconds": 2.5,
    "breacher_threat_window_seconds": 3.5
  },
  "perception": {
    "vision_half_angle_degrees": 80.0,
    "hearing_radius": 900.0,
    "eyes_on_radius": 2600.0,
    "pursuit_memory_window_seconds": 6.0,
    "threat_window_seconds": 9.0
  },
  "objective_pressure": {
    "tower_attack_range": 2300.0,
    "core_attack_range": 2700.0,
    "tower_standoff_distance": 1600.0,
    "core_standoff_distance": 2000.0,
    "objective_lane_spacing": 525.0
  },
  "movement": {
    "stuck_sample_interval_seconds": 0.5,
    "stuck_minimum_progress": 70.0,
    "stuck_trigger_seconds": 1.25,
    "unstick_duration_seconds": 1.4,
    "unstick_lateral_distance": 800.0,
    "unstick_backstep_distance": 325.0,
    "route_stall_max_unstick_attempts": 4,
    "objective_stall_lane_shift_attempts": 3,
    "objective_stall_max_lane_shift": 2
  },
  "respawn": {
    "respawn_seconds": 4.0
  }
}
)JSON");

	FArchonBotStrategyTuning Tuning;
	FString Error;
	TestTrue(TEXT("Strategy tuning parses"), UArchonBotStrategyTuningLibrary::ParseBotStrategyTuningJson(JsonText, Tuning, Error));
	TestEqual(TEXT("Revision parsed"), Tuning.Revision, FString(TEXT("test-live-tuning")));
	TestEqual(TEXT("Bot 0 follows parsed breacher role"), UArchonBotStrategyTuningLibrary::GetRoleForBotIndex(0, Tuning), FName(TEXT("breacher")));
	TestEqual(TEXT("Bot 1 follows parsed fighter role"), UArchonBotStrategyTuningLibrary::GetRoleForBotIndex(1, Tuning), FName(TEXT("fighter")));
	TestTrue(TEXT("Parsed fighter range"), FMath::IsNearlyEqual(Tuning.FighterEngageRange, 1200.0f));
	TestTrue(TEXT("Parsed breacher range"), FMath::IsNearlyEqual(Tuning.BreacherEngageRange, 420.0f));
	TestTrue(TEXT("Parsed breacher pursuit memory"), FMath::IsNearlyEqual(Tuning.BreacherPursuitMemoryWindowSeconds, 2.5f));
	TestTrue(TEXT("Parsed breacher threat window"), FMath::IsNearlyEqual(Tuning.BreacherThreatWindowSeconds, 3.5f));
	TestTrue(TEXT("Parsed hearing radius"), FMath::IsNearlyEqual(Tuning.HearingRadius, 900.0f));
	TestTrue(TEXT("Parsed tower standoff"), FMath::IsNearlyEqual(Tuning.TowerStandOffDistance, 1600.0f));
	TestTrue(TEXT("Parsed unstick lateral"), FMath::IsNearlyEqual(Tuning.UnstickLateralDistance, 800.0f));
	TestEqual(TEXT("Parsed route stall threshold"), Tuning.RouteStallMaxUnstickAttempts, 4);
	TestEqual(TEXT("Parsed objective lane shift threshold"), Tuning.ObjectiveStallLaneShiftAttempts, 3);
	TestEqual(TEXT("Parsed objective lane shift cap"), Tuning.ObjectiveStallMaxLaneShift, 2);
	TestTrue(TEXT("Parsed respawn"), FMath::IsNearlyEqual(Tuning.RespawnSeconds, 4.0f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonBotStrategyTuningInvalidJsonTest,
	"ArchonFactory.Bot.StrategyTuningRejectsInvalidJson",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FArchonBotStrategyTuningInvalidJsonTest::RunTest(const FString& Parameters)
{
	FArchonBotStrategyTuning Tuning;
	FString Error;
	TestFalse(TEXT("Invalid JSON fails closed"), UArchonBotStrategyTuningLibrary::ParseBotStrategyTuningJson(TEXT("{"), Tuning, Error));
	TestEqual(TEXT("Invalid JSON error"), Error, FString(TEXT("json_parse_failed")));
	return true;
}
