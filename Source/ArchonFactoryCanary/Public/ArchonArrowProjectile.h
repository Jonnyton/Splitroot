#pragma once

#include "CoreMinimal.h"
#include "ArchonCombatTypes.h"
#include "GameFramework/Actor.h"
#include "ArchonArrowProjectile.generated.h"

class UProjectileMovementComponent;
class UPrimitiveComponent;
class UStaticMeshComponent;

UCLASS()
class ARCHONFACTORYCANARY_API AArchonArrowProjectile : public AActor
{
	GENERATED_BODY()

public:
	AArchonArrowProjectile();

	UFUNCTION(BlueprintCallable, Category = "Archon|Weapon")
	void ConfigureShot(const FArchonShotPayload& InShot, const FArchonWeaponDamageProfile& InWeaponProfile);

	UFUNCTION(BlueprintCallable, Category = "Archon|Weapon")
	void HandleHit(AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	UFUNCTION(BlueprintPure, Category = "Archon|Weapon")
	FArchonShotPayload GetShot() const { return Shot; }

	UFUNCTION(BlueprintPure, Category = "Archon|Weapon")
	FArchonWeaponDamageProfile GetWeaponProfile() const { return WeaponProfile; }

protected:
	UStaticMeshComponent* GetProjectileMesh() const { return ArrowMesh; }
	UProjectileMovementComponent* GetProjectileMovement() const { return Movement; }

private:
	UFUNCTION()
	void HandleComponentHit(
		UPrimitiveComponent* HitComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		FVector NormalImpulse,
		const FHitResult& Hit);

	UFUNCTION()
	void HandleComponentOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	UPROPERTY(VisibleAnywhere, Category = "Archon|Weapon")
	TObjectPtr<UStaticMeshComponent> ArrowMesh;

	UPROPERTY(VisibleAnywhere, Category = "Archon|Weapon")
	TObjectPtr<UStaticMeshComponent> ShaftMesh;

	UPROPERTY(VisibleAnywhere, Category = "Archon|Weapon")
	TObjectPtr<UProjectileMovementComponent> Movement;

	UPROPERTY()
	FArchonShotPayload Shot;

	UPROPERTY()
	FArchonWeaponDamageProfile WeaponProfile;
};
