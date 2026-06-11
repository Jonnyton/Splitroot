#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ArchonWeaponTypes.h"
#include "ArchonRangedWeaponComponent.generated.h"

class AArchonArrowProjectile;

DECLARE_MULTICAST_DELEGATE_OneParam(FArchonWeaponFiredNative, const FArchonShotPayload&);
DECLARE_MULTICAST_DELEGATE(FArchonWeaponReloadStartedNative);
DECLARE_MULTICAST_DELEGATE(FArchonWeaponReloadCompletedNative);

UCLASS(ClassGroup=(Archon), Blueprintable, meta=(BlueprintSpawnableComponent))
class ARCHONFACTORYCANARY_API UArchonRangedWeaponComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UArchonRangedWeaponComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintCallable, Category = "Archon|Weapon")
	void ConfigureWeapon(int32 InOwnerTeamId, int32 InOwnerPlayerId, const FArchonWeaponStats& InStats);

	UFUNCTION(BlueprintCallable, Category = "Archon|Weapon")
	bool TryFire(const FVector& Origin, const FVector& Direction);

	UFUNCTION(BlueprintCallable, Category = "Archon|Weapon")
	bool TryReload();

	UFUNCTION(BlueprintPure, Category = "Archon|Weapon")
	FArchonWeaponState GetState() const { return State; }

	UFUNCTION(BlueprintPure, Category = "Archon|Weapon")
	FArchonWeaponStats GetStats() const { return Stats; }

	UFUNCTION(BlueprintPure, Category = "Archon|Weapon")
	int32 GetOwnerTeamId() const { return OwnerTeamId; }

	UFUNCTION(BlueprintPure, Category = "Archon|Weapon")
	int32 GetOwnerPlayerId() const { return OwnerPlayerId; }

	UFUNCTION(BlueprintPure, Category = "Archon|Weapon")
	int32 GetShotsFired() const { return ShotsFired; }

	UFUNCTION(BlueprintPure, Category = "Archon|Weapon")
	int32 GetReloadCount() const { return ReloadCount; }

	UFUNCTION(BlueprintPure, Category = "Archon|Weapon")
	bool IsReady() const { return State.FireState == EArchonWeaponFireState::Ready && State.CurrentAmmo > 0; }

	FArchonWeaponFiredNative OnWeaponFired;
	FArchonWeaponReloadStartedNative OnReloadStarted;
	FArchonWeaponReloadCompletedNative OnReloadCompleted;

protected:
	virtual void SpawnProjectile(const FArchonShotPayload& Shot);

	UPROPERTY(EditDefaultsOnly, Category = "Archon|Weapon")
	TSubclassOf<AArchonArrowProjectile> ProjectileClass;

	UPROPERTY(EditDefaultsOnly, Category = "Archon|Weapon")
	bool bAutoReloadOnEmpty = true;

private:
	FArchonShotPayload MakeShotPayload(const FVector& Origin, const FVector& Direction) const;
	static EArchonDamageType GetDamageTypeForWeaponClass(EArchonWeaponClass WeaponClass);
	bool HasWeaponAuthority() const;

	UPROPERTY(Replicated)
	int32 OwnerTeamId = INDEX_NONE;

	UPROPERTY(Replicated)
	int32 OwnerPlayerId = INDEX_NONE;

	UPROPERTY(Replicated)
	FArchonWeaponStats Stats;

	UPROPERTY(Replicated)
	FArchonWeaponState State;

	UPROPERTY(Replicated)
	int32 ShotsFired = 0;

	UPROPERTY(Replicated)
	int32 ReloadCount = 0;
};
