#include "ArchonBotStrategyTuningLibrary.h"

#include "Dom/JsonObject.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
	const FName FighterRole(TEXT("fighter"));
	const FName BreacherRole(TEXT("breacher"));

	TSharedPtr<FJsonObject> TryGetObject(const TSharedPtr<FJsonObject>& Root, const TCHAR* FieldName)
	{
		if (!Root.IsValid())
		{
			return nullptr;
		}

		const TSharedPtr<FJsonObject>* Object = nullptr;
		if (Root->TryGetObjectField(FieldName, Object) && Object && Object->IsValid())
		{
			return *Object;
		}
		return nullptr;
	}

	void TryReadString(const TSharedPtr<FJsonObject>& Object, const TCHAR* FieldName, FString& OutValue)
	{
		if (!Object.IsValid())
		{
			return;
		}

		FString Value;
		if (Object->TryGetStringField(FieldName, Value) && !Value.IsEmpty())
		{
			OutValue = Value;
		}
	}

	void TryReadFloat(
		const TSharedPtr<FJsonObject>& Object,
		const TCHAR* FieldName,
		float& OutValue,
		float MinValue,
		float MaxValue)
	{
		if (!Object.IsValid())
		{
			return;
		}

		double Number = 0.0;
		if (Object->TryGetNumberField(FieldName, Number))
		{
			OutValue = FMath::Clamp(static_cast<float>(Number), MinValue, MaxValue);
		}
	}

	void TryReadGroupedFloat(
		const TSharedPtr<FJsonObject>& Root,
		const TSharedPtr<FJsonObject>& Group,
		const TCHAR* FieldName,
		float& OutValue,
		float MinValue,
		float MaxValue)
	{
		TryReadFloat(Root, FieldName, OutValue, MinValue, MaxValue);
		TryReadFloat(Group, FieldName, OutValue, MinValue, MaxValue);
	}

	void TryReadInt(
		const TSharedPtr<FJsonObject>& Object,
		const TCHAR* FieldName,
		int32& OutValue,
		int32 MinValue,
		int32 MaxValue)
	{
		if (!Object.IsValid())
		{
			return;
		}

		double Number = 0.0;
		if (Object->TryGetNumberField(FieldName, Number))
		{
			OutValue = FMath::Clamp(FMath::RoundToInt(Number), MinValue, MaxValue);
		}
	}

	void TryReadGroupedInt(
		const TSharedPtr<FJsonObject>& Root,
		const TSharedPtr<FJsonObject>& Group,
		const TCHAR* FieldName,
		int32& OutValue,
		int32 MinValue,
		int32 MaxValue)
	{
		TryReadInt(Root, FieldName, OutValue, MinValue, MaxValue);
		TryReadInt(Group, FieldName, OutValue, MinValue, MaxValue);
	}

	bool TryReadRoleCycle(const TSharedPtr<FJsonObject>& Object, TArray<FName>& OutRoleCycle)
	{
		if (!Object.IsValid())
		{
			return false;
		}

		const TArray<TSharedPtr<FJsonValue>>* RoleValues = nullptr;
		if (!Object->TryGetArrayField(TEXT("role_cycle"), RoleValues))
		{
			return false;
		}

		TArray<FName> ParsedRoles;
		for (const TSharedPtr<FJsonValue>& Value : *RoleValues)
		{
			FString Role;
			if (!Value.IsValid() || !Value->TryGetString(Role))
			{
				continue;
			}
			Role.TrimStartAndEndInline();
			Role.ToLowerInline();
			if (Role == TEXT("fighter") || Role == TEXT("breacher"))
			{
				ParsedRoles.Add(FName(*Role));
			}
		}

		if (ParsedRoles.Num() == 0)
		{
			return false;
		}

		OutRoleCycle = ParsedRoles;
		return true;
	}
}

FArchonBotStrategyTuning::FArchonBotStrategyTuning()
{
	Schema = TEXT("tinyassets.game_factory.bot_strategy_tuning.v1");
	Revision = TEXT("fallback_cpp_defaults");
	RoleCycle.Add(FighterRole);
	RoleCycle.Add(FighterRole);
	RoleCycle.Add(BreacherRole);
}

FArchonBotStrategyTuning UArchonBotStrategyTuningLibrary::GetSplitrootBotStrategyTuning()
{
	FArchonBotStrategyTuning Tuning;
	FString Error;
	const FString TuningPath = GetSplitrootBotStrategyTuningPath();
	if (LoadBotStrategyTuningFromFile(TuningPath, Tuning, Error))
	{
		UE_LOG(
			LogTemp,
			Display,
			TEXT("ArchonFactoryCanary: BotStrategyTuningLoaded revision=%s roleCycle=%d fighterEngage=%.0f breacherEngage=%.0f breacherPursuit=%.1f breacherThreat=%.1f unstickLateral=%.0f unstickBackstep=%.0f routeStallMax=%d laneShiftEvery=%d laneShiftMax=%d path=%s"),
			*Tuning.Revision,
			Tuning.RoleCycle.Num(),
			Tuning.FighterEngageRange,
			Tuning.BreacherEngageRange,
			Tuning.BreacherPursuitMemoryWindowSeconds,
			Tuning.BreacherThreatWindowSeconds,
			Tuning.UnstickLateralDistance,
			Tuning.UnstickBackstepDistance,
			Tuning.RouteStallMaxUnstickAttempts,
			Tuning.ObjectiveStallLaneShiftAttempts,
			Tuning.ObjectiveStallMaxLaneShift,
			*TuningPath);
		return Tuning;
	}

	UE_LOG(LogTemp, Warning, TEXT("ArchonFactoryCanary: BotStrategyTuningFallback reason=%s path=%s"), *Error, *TuningPath);
	return FArchonBotStrategyTuning();
}

FName UArchonBotStrategyTuningLibrary::GetRoleForBotIndex(int32 BotIndex, const FArchonBotStrategyTuning& Tuning)
{
	if (Tuning.RoleCycle.Num() == 0)
	{
		return FighterRole;
	}

	const int32 RoleIndex = FMath::Abs(BotIndex) % Tuning.RoleCycle.Num();
	FString Role = Tuning.RoleCycle[RoleIndex].ToString();
	Role.ToLowerInline();
	return Role == TEXT("breacher") ? BreacherRole : FighterRole;
}

bool UArchonBotStrategyTuningLibrary::IsBreacherBot(int32 BotIndex, const FArchonBotStrategyTuning& Tuning)
{
	return GetRoleForBotIndex(BotIndex, Tuning) == BreacherRole;
}

FString UArchonBotStrategyTuningLibrary::GetSplitrootBotStrategyTuningPath()
{
	return FPaths::ConvertRelativePathToFull(FPaths::Combine(
		FPaths::ProjectDir(),
		TEXT("games"),
		TEXT("splitroot"),
		TEXT("FactoryContracts"),
		TEXT("bot_strategy_tuning.json")));
}

bool UArchonBotStrategyTuningLibrary::LoadBotStrategyTuningFromFile(
	const FString& TuningPath,
	FArchonBotStrategyTuning& OutTuning,
	FString& OutError)
{
	FString JsonText;
	if (!FFileHelper::LoadFileToString(JsonText, *TuningPath))
	{
		OutError = TEXT("file_not_readable");
		return false;
	}

	return ParseBotStrategyTuningJson(JsonText, OutTuning, OutError);
}

bool UArchonBotStrategyTuningLibrary::ParseBotStrategyTuningJson(
	const FString& JsonText,
	FArchonBotStrategyTuning& OutTuning,
	FString& OutError)
{
	OutTuning = FArchonBotStrategyTuning();
	OutError.Reset();

	TSharedPtr<FJsonObject> Root;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonText);
	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
	{
		OutError = TEXT("json_parse_failed");
		return false;
	}

	TryReadString(Root, TEXT("schema"), OutTuning.Schema);
	TryReadString(Root, TEXT("revision"), OutTuning.Revision);

	const TSharedPtr<FJsonObject> Roles = TryGetObject(Root, TEXT("roles"));
	const TSharedPtr<FJsonObject> Perception = TryGetObject(Root, TEXT("perception"));
	const TSharedPtr<FJsonObject> ObjectivePressure = TryGetObject(Root, TEXT("objective_pressure"));
	const TSharedPtr<FJsonObject> Movement = TryGetObject(Root, TEXT("movement"));
	const TSharedPtr<FJsonObject> Respawn = TryGetObject(Root, TEXT("respawn"));

	if (!TryReadRoleCycle(Roles, OutTuning.RoleCycle))
	{
		TryReadRoleCycle(Root, OutTuning.RoleCycle);
	}

	TryReadGroupedFloat(Root, Roles, TEXT("fighter_engage_range"), OutTuning.FighterEngageRange, 0.0f, 100000.0f);
	TryReadGroupedFloat(Root, Roles, TEXT("breacher_engage_range"), OutTuning.BreacherEngageRange, 0.0f, 100000.0f);
	TryReadGroupedFloat(Root, Roles, TEXT("breacher_pursuit_memory_window_seconds"), OutTuning.BreacherPursuitMemoryWindowSeconds, 0.0f, 300.0f);
	TryReadGroupedFloat(Root, Roles, TEXT("breacher_threat_window_seconds"), OutTuning.BreacherThreatWindowSeconds, 0.0f, 300.0f);

	TryReadGroupedFloat(Root, Perception, TEXT("vision_half_angle_degrees"), OutTuning.VisionHalfAngleDegrees, 1.0f, 179.0f);
	TryReadGroupedFloat(Root, Perception, TEXT("hearing_radius"), OutTuning.HearingRadius, 0.0f, 100000.0f);
	TryReadGroupedFloat(Root, Perception, TEXT("eyes_on_radius"), OutTuning.EyesOnRadius, 0.0f, 100000.0f);
	TryReadGroupedFloat(Root, Perception, TEXT("pursuit_memory_window_seconds"), OutTuning.PursuitMemoryWindowSeconds, 0.0f, 300.0f);
	TryReadGroupedFloat(Root, Perception, TEXT("threat_window_seconds"), OutTuning.ThreatWindowSeconds, 0.0f, 300.0f);

	TryReadGroupedFloat(Root, ObjectivePressure, TEXT("tower_attack_range"), OutTuning.TowerAttackRange, 0.0f, 100000.0f);
	TryReadGroupedFloat(Root, ObjectivePressure, TEXT("core_attack_range"), OutTuning.CoreAttackRange, 0.0f, 100000.0f);
	TryReadGroupedFloat(Root, ObjectivePressure, TEXT("tower_standoff_distance"), OutTuning.TowerStandOffDistance, 0.0f, 100000.0f);
	TryReadGroupedFloat(Root, ObjectivePressure, TEXT("core_standoff_distance"), OutTuning.CoreStandOffDistance, 0.0f, 100000.0f);
	TryReadGroupedFloat(Root, ObjectivePressure, TEXT("objective_lane_spacing"), OutTuning.ObjectiveLaneSpacing, 0.0f, 100000.0f);

	TryReadGroupedFloat(Root, Movement, TEXT("stuck_sample_interval_seconds"), OutTuning.StuckSampleIntervalSeconds, 0.01f, 60.0f);
	TryReadGroupedFloat(Root, Movement, TEXT("stuck_minimum_progress"), OutTuning.StuckMinimumProgress, 0.0f, 100000.0f);
	TryReadGroupedFloat(Root, Movement, TEXT("stuck_trigger_seconds"), OutTuning.StuckTriggerSeconds, 0.01f, 60.0f);
	TryReadGroupedFloat(Root, Movement, TEXT("unstick_duration_seconds"), OutTuning.UnstickDurationSeconds, 0.01f, 60.0f);
	TryReadGroupedFloat(Root, Movement, TEXT("unstick_lateral_distance"), OutTuning.UnstickLateralDistance, 0.0f, 100000.0f);
	TryReadGroupedFloat(Root, Movement, TEXT("unstick_backstep_distance"), OutTuning.UnstickBackstepDistance, 0.0f, 100000.0f);
	TryReadGroupedInt(Root, Movement, TEXT("route_stall_max_unstick_attempts"), OutTuning.RouteStallMaxUnstickAttempts, 0, 1000);
	TryReadGroupedInt(Root, Movement, TEXT("objective_stall_lane_shift_attempts"), OutTuning.ObjectiveStallLaneShiftAttempts, 0, 1000);
	TryReadGroupedInt(Root, Movement, TEXT("objective_stall_max_lane_shift"), OutTuning.ObjectiveStallMaxLaneShift, 0, 32);

	TryReadGroupedFloat(Root, Respawn, TEXT("respawn_seconds"), OutTuning.RespawnSeconds, 0.0f, 300.0f);

	OutError = TEXT("ok");
	return true;
}
