#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GenericTeamAgentInterface.h"
#include "ArchonTeamRtsTypes.h"
#include "ArchonCanaryFpsCharacter.generated.h"

class UCameraComponent;
class UMaterialInstanceDynamic;
class UNavigationInvokerComponent;
class USpringArmComponent;
class UStaticMeshComponent;
class UArchonCombatHealthComponent;
class UArchonFactionMovementComponent;
class UArchonVerdantThornsproutBow;

UCLASS()
class ARCHONFACTORYCANARY_API AArchonCanaryFpsCharacter : public ACharacter, public IGenericTeamAgentInterface
{
	GENERATED_BODY()

public:
	AArchonCanaryFpsCharacter();

	UFUNCTION(BlueprintPure, Category = "Archon|FPS")
	UCameraComponent* GetFirstPersonCamera() const { return FirstPersonCamera; }

	UFUNCTION(BlueprintPure, Category = "Archon|FPS")
	UArchonFactionMovementComponent* GetFactionMovement() const { return FactionMovement; }

	UFUNCTION(BlueprintPure, Category = "Archon|FPS")
	UArchonVerdantThornsproutBow* GetVerdantBow() const { return VerdantBow; }

	UFUNCTION(BlueprintPure, Category = "Archon|FPS")
	UArchonCombatHealthComponent* GetHealth() const { return Health; }

	virtual void SetGenericTeamId(const FGenericTeamId& NewTeamId) override;
	virtual FGenericTeamId GetGenericTeamId() const override;
	virtual ETeamAttitude::Type GetTeamAttitudeTowards(const AActor& Other) const override;

	// Weapons are server-authoritative; network clients route trigger
	// pulls through these RPCs (the pawn is connection-owned, so they
	// are callable from the owning client).
	UFUNCTION(Server, Reliable, Category = "Archon|FPS")
	void ServerFireBow(FVector Origin, FVector Direction);

	UFUNCTION(Server, Reliable, Category = "Archon|FPS")
	void ServerReloadBow();

	// RTS orders are server-authoritative too; the server rebuilds the
	// intent against its own map table (shared Archon command surface).
	UFUNCTION(Server, Reliable, Category = "Archon|FPS")
	void ServerSubmitRtsOrder(EArchonRtsOrderKind OrderKind);

	// First/third person view toggle (V). Cosmetic and client-local.
	UFUNCTION(BlueprintCallable, Category = "Archon|FPS")
	bool ToggleCameraView();

	// Team orb tint: team identity must read at a glance from any view
	// (Jonathan 2026-06-10: "I can't see any of the units" — the
	// characters had NO body mesh at all until this).
	UFUNCTION(BlueprintCallable, Category = "Archon|FPS")
	void SetBodyTeamColor(const FLinearColor& TeamColor);

	// Ragdoll for the death linger; restores the pose on respawn
	// (Jonathan 2026-06-10: the lingering dead should fall over).
	UFUNCTION(BlueprintCallable, Category = "Archon|FPS")
	void SetBodyDead(bool bDead);

	UFUNCTION(BlueprintPure, Category = "Archon|FPS")
	bool IsFirstPersonView() const { return bFirstPersonView; }

private:
	UPROPERTY(VisibleAnywhere, Category = "Archon|FPS")
	TObjectPtr<UCameraComponent> FirstPersonCamera;

	UPROPERTY(VisibleAnywhere, Category = "Archon|FPS")
	TObjectPtr<UArchonFactionMovementComponent> FactionMovement;

	UPROPERTY(VisibleAnywhere, Category = "Archon|FPS")
	TObjectPtr<UArchonVerdantThornsproutBow> VerdantBow;

	UPROPERTY(VisibleAnywhere, Category = "Archon|FPS")
	TObjectPtr<UArchonCombatHealthComponent> Health;

	UPROPERTY(VisibleAnywhere, Category = "Archon|FPS")
	TObjectPtr<USpringArmComponent> ThirdPersonBoom;

	UPROPERTY(VisibleAnywhere, Category = "Archon|FPS")
	TObjectPtr<UCameraComponent> ThirdPersonCamera;

	UPROPERTY(VisibleAnywhere, Category = "Archon|FPS")
	TObjectPtr<UStaticMeshComponent> BodyMesh;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> BodyMaterial;

	UPROPERTY(VisibleAnywhere, Category = "Archon|FPS")
	TObjectPtr<UNavigationInvokerComponent> NavInvoker;

	bool bFirstPersonView = true;
};
