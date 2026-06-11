#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ArchonLenswrightBracewrightActor.generated.h"

class UArchonAiCombatBehaviorComponent;
class UArchonCombatHealthComponent;
class UArchonLenswrightPressureBoltCrossbow;
class UStaticMeshComponent;

UCLASS(Blueprintable)
class ARCHONFACTORYCANARY_API AArchonLenswrightBracewrightActor : public ACharacter
{
	GENERATED_BODY()

public:
	AArchonLenswrightBracewrightActor();

	UFUNCTION(BlueprintPure, Category = "Archon|Lenswright")
	UArchonCombatHealthComponent* GetHealth() const { return Health; }

	UFUNCTION(BlueprintPure, Category = "Archon|Lenswright")
	UArchonLenswrightPressureBoltCrossbow* GetCrossbow() const { return Crossbow; }

	UFUNCTION(BlueprintPure, Category = "Archon|Lenswright")
	UArchonAiCombatBehaviorComponent* GetCombatBehavior() const { return CombatBehavior; }

	UFUNCTION(BlueprintPure, Category = "Archon|Lenswright")
	int32 GetTeamId() const { return TeamId; }

	UFUNCTION(BlueprintPure, Category = "Archon|Lenswright")
	FName GetUnitId() const { return UnitId; }

	UFUNCTION(BlueprintCallable, Category = "Archon|Lenswright")
	void ConfigureBracewright(int32 InTeamId, FName InUnitId);

private:
	UPROPERTY(VisibleAnywhere, Category = "Archon|Lenswright")
	TObjectPtr<UArchonCombatHealthComponent> Health;

	UPROPERTY(VisibleAnywhere, Category = "Archon|Lenswright")
	TObjectPtr<UArchonLenswrightPressureBoltCrossbow> Crossbow;

	UPROPERTY(VisibleAnywhere, Category = "Archon|Lenswright")
	TObjectPtr<UArchonAiCombatBehaviorComponent> CombatBehavior;

	UPROPERTY(VisibleAnywhere, Category = "Archon|Lenswright")
	TObjectPtr<UStaticMeshComponent> PlaceholderMesh;

	UPROPERTY(EditAnywhere, Category = "Archon|Lenswright")
	int32 TeamId = 1;

	UPROPERTY(EditAnywhere, Category = "Archon|Lenswright")
	FName UnitId = TEXT("lenswright_bracewright");

	UPROPERTY(VisibleAnywhere, Category = "Archon|Lenswright")
	FLinearColor DebugColor = FLinearColor(0.55f, 0.30f, 0.15f, 1.0f);
};
