#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ArchonLenswrightSundialOpticActor.generated.h"

class UArchonAiCombatBehaviorComponent;
class UArchonCombatHealthComponent;
class UStaticMeshComponent;

UCLASS(Blueprintable)
class ARCHONFACTORYCANARY_API AArchonLenswrightSundialOpticActor : public ACharacter
{
	GENERATED_BODY()

public:
	AArchonLenswrightSundialOpticActor();

	UFUNCTION(BlueprintPure, Category = "Archon|Lenswright")
	UArchonCombatHealthComponent* GetHealth() const { return Health; }

	UFUNCTION(BlueprintPure, Category = "Archon|Lenswright")
	UArchonAiCombatBehaviorComponent* GetCombatBehavior() const { return CombatBehavior; }

	UFUNCTION(BlueprintPure, Category = "Archon|Lenswright")
	float GetVisionRadius() const { return VisionRadius; }

	UFUNCTION(BlueprintCallable, Category = "Archon|Lenswright")
	void ConfigureSundial(int32 InTeamId, FName InUnitId);

private:
	UPROPERTY(VisibleAnywhere, Category = "Archon|Lenswright")
	TObjectPtr<UArchonCombatHealthComponent> Health;

	UPROPERTY(VisibleAnywhere, Category = "Archon|Lenswright")
	TObjectPtr<UArchonAiCombatBehaviorComponent> CombatBehavior;

	UPROPERTY(VisibleAnywhere, Category = "Archon|Lenswright")
	TObjectPtr<UStaticMeshComponent> PlaceholderMesh;

	UPROPERTY(EditAnywhere, Category = "Archon|Lenswright")
	int32 TeamId = 1;

	UPROPERTY(EditAnywhere, Category = "Archon|Lenswright")
	FName UnitId = TEXT("lenswright_sundial_optic");

	UPROPERTY(EditAnywhere, Category = "Archon|Lenswright")
	float VisionRadius = 4500.0f;

	UPROPERTY(VisibleAnywhere, Category = "Archon|Lenswright")
	FLinearColor DebugColor = FLinearColor(0.70f, 0.55f, 0.25f, 1.0f);
};
