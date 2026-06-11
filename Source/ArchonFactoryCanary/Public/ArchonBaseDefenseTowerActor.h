#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ArchonValleyLayoutTypes.h"
#include "ArchonBaseDefenseTowerActor.generated.h"

class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class UArchonCombatHealthComponent;
class UArchonWorldHealthBarComponent;
class AArchonMatchStateActor;
struct FArchonDamageApplicationResult;

/**
 * Renegade-style base auto-defense (decision 5, Jonathan 2026-06-10):
 * solves the rush problem the Renegade way, but ships tuned
 * almost-worthless (DamagePerShot ~2 vs 150hp bodies) so early matches
 * stay about players, not towers. It must still eventually kill a
 * loiterer. Killable (its own health) — the liveness-flag ratchet
 * (decision: powerplant-loss doubles prices) arrives later.
 */
UCLASS(Blueprintable)
class ARCHONFACTORYCANARY_API AArchonBaseDefenseTowerActor : public AActor
{
	GENERATED_BODY()

public:
	AArchonBaseDefenseTowerActor();

	virtual void Tick(float DeltaSeconds) override;

	void ConfigureTower(const FArchonValleyPlacement& Placement);

	UFUNCTION(BlueprintPure, Category = "Archon|Match")
	UArchonCombatHealthComponent* GetHealth() const { return Health; }

	UFUNCTION(BlueprintPure, Category = "Archon|Match")
	int32 GetTeamId() const { return TeamId; }

	UFUNCTION(BlueprintPure, Category = "Archon|Match")
	int32 GetShotsFired() const { return ShotsFired; }

private:
	void HandleHealthChanged(const FArchonDamageApplicationResult& DamageResult);

	UPROPERTY(VisibleAnywhere, Category = "Archon|Match")
	TObjectPtr<UStaticMeshComponent> TowerMesh;

	UPROPERTY(VisibleAnywhere, Category = "Archon|Match")
	TObjectPtr<UArchonCombatHealthComponent> Health;

	UPROPERTY(VisibleAnywhere, Category = "Archon|Match")
	TObjectPtr<UArchonWorldHealthBarComponent> HealthBar;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> TowerMaterial;

	// Lazily found on first accepted hit; reports siege damage for points.
	UPROPERTY()
	TObjectPtr<AArchonMatchStateActor> MatchState;

	UPROPERTY(VisibleAnywhere, Category = "Archon|Match")
	int32 TeamId = INDEX_NONE;

	FLinearColor HealthyColor = FLinearColor::White;
	float FireCooldownRemaining = 0.0f;
	int32 ShotsFired = 0;
	bool bUsingStandInMesh = false;

	// Decision-5 tuning: near-zero damage, slow cycle, modest reach.
	// (2.0 read as literally zero in play — 5.0 is perceptible while
	// still ~30 shots to kill, Jonathan 2026-06-10.)
	float DamagePerShot = 5.0f;
	float FireCycleSeconds = 1.5f;
	float TargetRange = 2800.0f;
	float TowerMaxHealth = 600.0f;
};
