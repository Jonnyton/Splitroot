#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "ArchonBotAIController.generated.h"

class AArchonCanaryFpsCharacter;
class UAIPerceptionComponent;
class UAISenseConfig_Damage;
class UAISenseConfig_Hearing;
class UAISenseConfig_Sight;
struct FArchonBotStrategyTuning;

UCLASS()
class ARCHONFACTORYCANARY_API AArchonBotAIController : public AAIController
{
	GENERATED_BODY()

public:
	AArchonBotAIController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintCallable, Category = "Archon|Bot")
	void ConfigureBotTeam(int32 InTeamId, int32 InBotIndex);

	void ConfigureBotTeamWithTuning(int32 InTeamId, int32 InBotIndex, const FArchonBotStrategyTuning& StrategyTuning);

	UFUNCTION(BlueprintPure, Category = "Archon|Bot")
	int32 GetArchonTeamId() const { return TeamId; }

	UFUNCTION(BlueprintPure, Category = "Archon|Bot")
	int32 GetBotIndex() const { return BotIndex; }

	AArchonCanaryFpsCharacter* FindNearestNativePerceivedEnemy(
		float MaxRange,
		FVector& OutLastKnownLocation,
		FName& OutSenseTag) const;

	virtual ETeamAttitude::Type GetTeamAttitudeTowards(const AActor& Other) const override;

protected:
	virtual void OnPossess(APawn* InPawn) override;

private:
	UPROPERTY(VisibleAnywhere, Category = "Archon|Bot")
	TObjectPtr<UAIPerceptionComponent> BotPerceptionComponent;

	UPROPERTY()
	TObjectPtr<UAISenseConfig_Sight> SightConfig;

	UPROPERTY()
	TObjectPtr<UAISenseConfig_Hearing> HearingConfig;

	UPROPERTY()
	TObjectPtr<UAISenseConfig_Damage> DamageConfig;

	int32 TeamId = INDEX_NONE;
	int32 BotIndex = INDEX_NONE;
};
