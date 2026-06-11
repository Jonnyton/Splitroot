#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "ArchonWorldHealthBarComponent.generated.h"

class UStaticMeshComponent;
class UMaterialInstanceDynamic;

/**
 * World-space health bar for structures (towers, cores): a dark back
 * plate and a colored fill built from engine basic cubes — zero asset
 * dependencies, safe in headless smokes. Yaws toward the local
 * viewer's camera (WC3 billboard readability). Decisions (fill scale,
 * band color) live in UArchonHealthBarPolicyLibrary.
 */
UCLASS(ClassGroup=(Archon), meta=(BlueprintSpawnableComponent))
class ARCHONFACTORYCANARY_API UArchonWorldHealthBarComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	UArchonWorldHealthBarComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category = "Archon|HealthBar")
	void SetHealthFraction(float InHealthFraction);

	UFUNCTION(BlueprintPure, Category = "Archon|HealthBar")
	float GetHealthFraction() const { return HealthFraction; }

	// Bar footprint in world units (X = width when facing the viewer).
	UFUNCTION(BlueprintCallable, Category = "Archon|HealthBar")
	void ConfigureBarSize(float InWidthUnits, float InThicknessUnits);

private:
	void RefreshBar();

	UPROPERTY(VisibleAnywhere, Category = "Archon|HealthBar")
	TObjectPtr<UStaticMeshComponent> BackPlate;

	UPROPERTY(VisibleAnywhere, Category = "Archon|HealthBar")
	TObjectPtr<UStaticMeshComponent> Fill;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> BackMaterial;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> FillMaterial;

	float HealthFraction = 1.0f;
	float WidthUnits = 220.0f;
	float ThicknessUnits = 22.0f;
};
