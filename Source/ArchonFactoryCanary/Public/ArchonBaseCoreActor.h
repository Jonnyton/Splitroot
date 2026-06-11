#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ArchonValleyLayoutTypes.h"
#include "ArchonBaseCoreActor.generated.h"

class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class UArchonCombatHealthComponent;
class UArchonWorldHealthBarComponent;
struct FArchonDamageApplicationResult;

/**
 * The team objective. When a core dies, its team loses the match
 * (Renegade base-destruction pattern; v0 spec "destroy the enemy base
 * core"). Visual is a faction-tinted pillar that chars toward black as
 * it takes damage, so core pressure reads at a glance before real art.
 */
UCLASS(Blueprintable)
class ARCHONFACTORYCANARY_API AArchonBaseCoreActor : public AActor
{
	GENERATED_BODY()

public:
	AArchonBaseCoreActor();

	void ConfigureCore(const FArchonValleyPlacement& Placement, float MaxHealth);

	UFUNCTION(BlueprintPure, Category = "Archon|Match")
	UArchonCombatHealthComponent* GetHealth() const { return Health; }

	UFUNCTION(BlueprintPure, Category = "Archon|Match")
	int32 GetTeamId() const { return TeamId; }

	UFUNCTION(BlueprintPure, Category = "Archon|Match")
	FName GetPieceId() const { return PieceId; }

private:
	void HandleHealthChanged(const FArchonDamageApplicationResult& DamageResult);
	void RefreshDamageTint();

	UPROPERTY(VisibleAnywhere, Category = "Archon|Match")
	TObjectPtr<UStaticMeshComponent> CoreMesh;

	UPROPERTY(VisibleAnywhere, Category = "Archon|Match")
	TObjectPtr<UArchonCombatHealthComponent> Health;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> CoreMaterial;

	UPROPERTY(VisibleAnywhere, Category = "Archon|Match")
	TObjectPtr<UArchonWorldHealthBarComponent> HealthBar;

	UPROPERTY(VisibleAnywhere, Category = "Archon|Match")
	FName PieceId = NAME_None;

	UPROPERTY(VisibleAnywhere, Category = "Archon|Match")
	int32 TeamId = INDEX_NONE;

	UPROPERTY(VisibleAnywhere, Category = "Archon|Match")
	FLinearColor HealthyColor = FLinearColor::White;
};
