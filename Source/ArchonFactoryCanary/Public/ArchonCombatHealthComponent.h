#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ArchonCombatTypes.h"
#include "ArchonCombatHealthComponent.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FArchonHealthChangedNative, const FArchonDamageApplicationResult&);
DECLARE_MULTICAST_DELEGATE_OneParam(FArchonHealthDeathNative, const FArchonDamageApplicationResult&);

UCLASS(ClassGroup=(Archon), Blueprintable, meta=(BlueprintSpawnableComponent))
class ARCHONFACTORYCANARY_API UArchonCombatHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UArchonCombatHealthComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintCallable, Category = "Archon|Combat")
	void ConfigureHealth(int32 InTeamId, float InMaxHealth, float InArmorModifier);

	UFUNCTION(BlueprintCallable, Category = "Archon|Combat")
	FArchonDamageApplicationResult ApplyHit(const FArchonHitResult& HitResult);

	UFUNCTION(BlueprintCallable, Category = "Archon|Combat")
	FArchonDamageApplicationResult ApplyDirectDamage(
		float DamageAmount,
		EArchonDamageType DamageType,
		int32 InstigatorTeamId,
		int32 InstigatorPlayerId);

	UFUNCTION(BlueprintCallable, Category = "Archon|Combat")
	void HealToFull();

	UFUNCTION(BlueprintCallable, Category = "Archon|Combat")
	void ResetProofState();

	UFUNCTION(BlueprintPure, Category = "Archon|Combat")
	int32 GetTeamId() const { return TeamId; }

	UFUNCTION(BlueprintCallable, Category = "Archon|Combat")
	void SetTeamId(int32 InTeamId) { TeamId = InTeamId; }

	UFUNCTION(BlueprintPure, Category = "Archon|Combat")
	float GetCurrentHealth() const { return CurrentHealth; }

	UFUNCTION(BlueprintPure, Category = "Archon|Combat")
	float GetMaxHealth() const { return MaxHealth; }

	UFUNCTION(BlueprintPure, Category = "Archon|Combat")
	float GetHealthFraction() const { return MaxHealth > 0.0f ? FMath::Clamp(CurrentHealth / MaxHealth, 0.0f, 1.0f) : 0.0f; }

	UFUNCTION(BlueprintPure, Category = "Archon|Combat")
	bool IsAlive() const { return CurrentHealth > 0.0f; }

	UFUNCTION(BlueprintPure, Category = "Archon|Combat")
	float GetArmorModifier() const { return ArmorModifier; }

	UFUNCTION(BlueprintPure, Category = "Archon|Combat")
	int32 GetTotalHitsTaken() const { return TotalHitsTaken; }

	UFUNCTION(BlueprintPure, Category = "Archon|Combat")
	int32 GetTotalDeaths() const { return TotalDeaths; }

	UFUNCTION(BlueprintPure, Category = "Archon|Combat")
	FArchonDamageApplicationResult GetLastDamageApplication() const { return LastDamageApplication; }

	// Threat-response seam (2026-06-11): where the last ACCEPTED hit was
	// fired from. ZeroVector until first hit / for direct damage.
	UFUNCTION(BlueprintPure, Category = "Archon|Combat")
	FVector GetLastAcceptedShotOrigin() const { return LastAcceptedShotOrigin; }

	FArchonHealthChangedNative OnHealthChanged;
	FArchonHealthDeathNative OnDeath;

private:
	UPROPERTY(Replicated)
	int32 TeamId = INDEX_NONE;

	UPROPERTY(Replicated)
	float MaxHealth = 150.0f;

	UPROPERTY(Replicated)
	float CurrentHealth = 150.0f;

	UPROPERTY(Replicated)
	float ArmorModifier = 1.0f;

	UPROPERTY(Replicated)
	int32 TotalHitsTaken = 0;

	UPROPERTY(Replicated)
	int32 TotalDeaths = 0;

	UPROPERTY(Replicated)
	FArchonDamageApplicationResult LastDamageApplication;

	UPROPERTY(Replicated)
	FVector LastAcceptedShotOrigin = FVector::ZeroVector;
};
