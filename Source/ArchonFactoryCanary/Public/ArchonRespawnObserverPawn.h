#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "ArchonRespawnObserverPawn.generated.h"

class UCameraComponent;
class UFloatingPawnMovement;

UCLASS(Blueprintable)
class ARCHONFACTORYCANARY_API AArchonRespawnObserverPawn : public APawn
{
	GENERATED_BODY()

public:
	AArchonRespawnObserverPawn();

	UFUNCTION(BlueprintCallable, Category = "Archon|Respawn")
	void ConfigureObserverFromDeathLocation(const FVector& DeathLocation);

	UFUNCTION(BlueprintPure, Category = "Archon|Respawn")
	UCameraComponent* GetObserverCamera() const { return ObserverCamera; }

private:
	UPROPERTY(VisibleAnywhere, Category = "Archon|Respawn")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, Category = "Archon|Respawn")
	TObjectPtr<UCameraComponent> ObserverCamera;

	UPROPERTY(VisibleAnywhere, Category = "Archon|Respawn")
	TObjectPtr<UFloatingPawnMovement> FloatingMovement;
};
