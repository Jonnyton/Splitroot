#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ArchonTechBuildingActor.generated.h"

class AArchonMatchStateActor;
class UArchonCombatHealthComponent;
class UArchonWorldHealthBarComponent;
class UMaterialInstanceDynamic;
class UStaticMeshComponent;
struct FArchonDamageApplicationResult;

/**
 * First physical RTS tech structure. Its alive state owns one team tech tag;
 * death removes that tag so the FPS shop locks/inflates again.
 */
UCLASS(Blueprintable)
class ARCHONFACTORYCANARY_API AArchonTechBuildingActor : public AActor
{
	GENERATED_BODY()

public:
	AArchonTechBuildingActor();

	void ConfigureTechBuilding(int32 InTeamId, FName InTechId, AArchonMatchStateActor* InMatchState, FLinearColor InTeamColor);

	UFUNCTION(BlueprintPure, Category = "Archon|Tech")
	UArchonCombatHealthComponent* GetHealth() const { return Health; }

	UFUNCTION(BlueprintPure, Category = "Archon|Tech")
	int32 GetTeamId() const { return TeamId; }

	UFUNCTION(BlueprintPure, Category = "Archon|Tech")
	FName GetTechId() const { return TechId; }

private:
	void HandleHealthChanged(const FArchonDamageApplicationResult& DamageResult);
	void RefreshDamageTint();

	UPROPERTY(VisibleAnywhere, Category = "Archon|Tech")
	TObjectPtr<UStaticMeshComponent> BuildingMesh;

	UPROPERTY(VisibleAnywhere, Category = "Archon|Tech")
	TObjectPtr<UArchonCombatHealthComponent> Health;

	UPROPERTY(VisibleAnywhere, Category = "Archon|Tech")
	TObjectPtr<UArchonWorldHealthBarComponent> HealthBar;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> BuildingMaterial;

	UPROPERTY()
	TObjectPtr<AArchonMatchStateActor> MatchState;

	UPROPERTY(VisibleAnywhere, Category = "Archon|Tech")
	int32 TeamId = INDEX_NONE;

	UPROPERTY(VisibleAnywhere, Category = "Archon|Tech")
	FName TechId = NAME_None;

	UPROPERTY(VisibleAnywhere, Category = "Archon|Tech")
	FLinearColor TeamColor = FLinearColor::White;

	bool bRemovedTechAfterDeath = false;
	float MaxHealth = 450.0f;
};
