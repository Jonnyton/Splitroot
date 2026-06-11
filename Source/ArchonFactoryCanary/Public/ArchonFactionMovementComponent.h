#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ArchonFactionTypes.h"
#include "ArchonFactionMovementComponent.generated.h"

class ACharacter;

DECLARE_MULTICAST_DELEGATE_OneParam(FArchonFactionMovementLaunchedNative, EArchonFactionMovementVerb);

UCLASS(ClassGroup=(Archon), Blueprintable, meta=(BlueprintSpawnableComponent))
class ARCHONFACTORYCANARY_API UArchonFactionMovementComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UArchonFactionMovementComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category = "Archon|Locomotion")
	void ConfigureFaction(EArchonFaction InFaction);

	UFUNCTION(BlueprintCallable, Category = "Archon|Locomotion")
	void NotifySprintHeld(bool bHeld);

	UFUNCTION(BlueprintCallable, Category = "Archon|Locomotion")
	void NotifyJumpPressed();

	UFUNCTION(BlueprintPure, Category = "Archon|Locomotion")
	EArchonFaction GetFaction() const { return Faction; }

	UFUNCTION(BlueprintPure, Category = "Archon|Locomotion")
	FArchonFactionMovementCooldown GetCooldown() const { return Cooldown; }

	UFUNCTION(BlueprintPure, Category = "Archon|Locomotion")
	bool IsCooldownReady() const;

	UFUNCTION(BlueprintPure, Category = "Archon|Locomotion")
	int32 GetLaunchCount() const { return LaunchCount; }

	UFUNCTION(BlueprintPure, Category = "Archon|Locomotion")
	int32 GetCooldownBlockedCount() const { return CooldownBlockedCount; }

	UFUNCTION(BlueprintPure, Category = "Archon|Locomotion")
	FArchonFactionMovementTuning GetTuning() const { return Tuning; }

	UFUNCTION(BlueprintCallable, Category = "Archon|Locomotion")
	void SetTuning(const FArchonFactionMovementTuning& InTuning) { Tuning = InTuning; }

	UFUNCTION(BlueprintPure, Category = "Archon|Locomotion")
	bool WillVaultThisFrame() const { return bWillVaultThisFrame; }

	FArchonFactionMovementLaunchedNative OnLaunched;

private:
	ACharacter* GetOwningCharacter() const;
	bool IsOwnerGrounded() const;
	bool ShouldVaultOnPendingInput() const;
	FVector ComputeForwardImpulseDirection() const;

	UPROPERTY(EditAnywhere, Category = "Archon|Locomotion")
	EArchonFaction Faction = EArchonFaction::VerdantChoir;

	UPROPERTY(EditAnywhere, Category = "Archon|Locomotion")
	FArchonFactionMovementTuning Tuning;

	UPROPERTY()
	FArchonFactionMovementCooldown Cooldown;

	bool bSprintHeld = false;
	float SprintHeldSeconds = 0.0f;
	bool bJumpPressedThisFrame = false;
	bool bWillVaultThisFrame = false;

	int32 LaunchCount = 0;
	int32 CooldownBlockedCount = 0;
};
