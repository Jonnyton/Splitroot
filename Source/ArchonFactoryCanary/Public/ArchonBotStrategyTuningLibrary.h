#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ArchonBotStrategyTuningLibrary.generated.h"

USTRUCT(BlueprintType)
struct FArchonBotStrategyTuning
{
	GENERATED_BODY()

	FArchonBotStrategyTuning();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Archon|Bot")
	FString Schema;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Archon|Bot")
	FString Revision;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Archon|Bot")
	TArray<FName> RoleCycle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Archon|Bot")
	float FighterEngageRange = 1400.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Archon|Bot")
	float BreacherEngageRange = 500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Archon|Bot")
	float BreacherPursuitMemoryWindowSeconds = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Archon|Bot")
	float BreacherThreatWindowSeconds = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Archon|Bot")
	float VisionHalfAngleDegrees = 70.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Archon|Bot")
	float HearingRadius = 800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Archon|Bot")
	float EyesOnRadius = 2500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Archon|Bot")
	float PursuitMemoryWindowSeconds = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Archon|Bot")
	float ThreatWindowSeconds = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Archon|Bot")
	float TowerAttackRange = 2400.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Archon|Bot")
	float CoreAttackRange = 2800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Archon|Bot")
	float TowerStandOffDistance = 1700.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Archon|Bot")
	float CoreStandOffDistance = 2100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Archon|Bot")
	float ObjectiveLaneSpacing = 450.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Archon|Bot")
	float RespawnSeconds = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Archon|Bot")
	float StuckSampleIntervalSeconds = 0.75f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Archon|Bot")
	float StuckMinimumProgress = 80.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Archon|Bot")
	float StuckTriggerSeconds = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Archon|Bot")
	float UnstickDurationSeconds = 1.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Archon|Bot")
	float UnstickLateralDistance = 650.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Archon|Bot")
	float UnstickBackstepDistance = 250.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Archon|Bot")
	int32 RouteStallMaxUnstickAttempts = 8;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Archon|Bot")
	int32 ObjectiveStallLaneShiftAttempts = 6;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Archon|Bot")
	int32 ObjectiveStallMaxLaneShift = 3;
};

/**
 * Runtime reader for games/splitroot/FactoryContracts/bot_strategy_tuning.json.
 * Defaults mirror the current compiled bot behavior so the file can safely
 * disappear during canary work without changing match behavior.
 */
UCLASS()
class ARCHONFACTORYCANARY_API UArchonBotStrategyTuningLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Archon|Bot")
	static FArchonBotStrategyTuning GetSplitrootBotStrategyTuning();

	UFUNCTION(BlueprintPure, Category = "Archon|Bot")
	static FName GetRoleForBotIndex(int32 BotIndex, const FArchonBotStrategyTuning& Tuning);

	UFUNCTION(BlueprintPure, Category = "Archon|Bot")
	static bool IsBreacherBot(int32 BotIndex, const FArchonBotStrategyTuning& Tuning);

	static FString GetSplitrootBotStrategyTuningPath();
	static bool LoadBotStrategyTuningFromFile(const FString& TuningPath, FArchonBotStrategyTuning& OutTuning, FString& OutError);
	static bool ParseBotStrategyTuningJson(const FString& JsonText, FArchonBotStrategyTuning& OutTuning, FString& OutError);
};
