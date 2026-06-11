#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ArchonFpsInputProfile.h"
#include "ArchonMapTableInteractionTypes.h"
#include "ArchonRespawnTypes.h"
#include "ArchonTeamRtsTypes.h"
#include "ArchonPlayerInputBridgeComponent.generated.h"

class AArchonMapTableActor;
class AArchonMatchStateActor;
class ACharacter;
class APlayerController;
class SWidget;
class UArchonMapTableInteractorComponent;
class UArchonMapTableWidget;
class UArchonRespawnStateComponent;

UCLASS(ClassGroup=(Archon), Blueprintable, meta=(BlueprintSpawnableComponent))
class ARCHONFACTORYCANARY_API UArchonPlayerInputBridgeComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UArchonPlayerInputBridgeComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category = "Archon|Input")
	void ConfigureBridge(
		APlayerController* InPlayerController,
		AArchonMapTableActor* InMapTable,
		const FArchonMapTableInteractorConfig& InInteractorConfig);

	UFUNCTION(BlueprintCallable, Category = "Archon|Input")
	bool PreviewRuntimeMapTable();

	UFUNCTION(BlueprintCallable, Category = "Archon|Input")
	bool SubmitRuntimeRtsOrder(EArchonRtsOrderKind OrderKind);

	UFUNCTION(BlueprintCallable, Category = "Archon|Input")
	bool RequestTechBuildingPurchase();

	UFUNCTION(BlueprintCallable, Category = "Archon|Input")
	FName CycleSelectedTechBuilding(int32 Direction);

	UFUNCTION(BlueprintCallable, Category = "Archon|Input")
	bool SelectTechBuilding(FName TechId);

	UFUNCTION(BlueprintCallable, Category = "Archon|Input")
	bool CloseRuntimeMapTable();

	UFUNCTION(BlueprintCallable, Category = "Archon|Input")
	bool RunRuntimeRtsProofSequence();

	UFUNCTION(BlueprintCallable, Category = "Archon|Input")
	bool RunRuntimeMapTableWidgetProofSequence();

	UFUNCTION(BlueprintCallable, Category = "Archon|Input")
	bool SubmitRuntimeMapTableWidgetMoveOrderAt(const FVector2D& TableSpacePoint, FName TargetId);

	UFUNCTION(BlueprintCallable, Category = "Archon|Input")
	bool SubmitRuntimeMapTableWidgetRallyPointAt(const FVector2D& TableSpacePoint, FName TargetId);

	UFUNCTION(BlueprintCallable, Category = "Archon|Input")
	void SetRespawnStateComponent(UArchonRespawnStateComponent* InRespawnState);

	UFUNCTION(BlueprintPure, Category = "Archon|Input")
	bool IsDeadStateCommandAllowed() const;

	UFUNCTION(BlueprintCallable, Category = "Archon|Input")
	bool ApplyRuntimeFactionMovementInput(
		ACharacter* Character,
		bool bSprintHeld,
		bool bJumpPressed,
		bool bJumpReleased);

	UFUNCTION(BlueprintCallable, Category = "Archon|Input")
	bool ApplyRuntimeWeaponInput(
		ACharacter* Character,
		bool bFirePressed,
		bool bReloadPressed);

	UFUNCTION(BlueprintCallable, Category = "Archon|Input")
	bool RunRuntimeWeaponProofSequence(ACharacter* Character);

	// E-interact convention: world interactables (map table first) show a
	// "[E] ..." prompt in range; E opens their interface. Range policy in
	// UArchonInteractPromptPolicyLibrary.
	UFUNCTION(BlueprintCallable, Category = "Archon|Input")
	void RefreshInteractPrompt();

	UFUNCTION(BlueprintCallable, Category = "Archon|Input")
	bool TryInteract();

	UFUNCTION(BlueprintPure, Category = "Archon|Input")
	bool IsInteractPromptVisible() const { return bInteractPromptVisible; }

	// In-game system menu (Escape): RESUME / controls reference / mouse
	// look-speed slider / quit paths. The menu gates all FPS input while
	// open; look speed persists to GameUserSettings.ini.
	UFUNCTION(BlueprintCallable, Category = "Archon|Input")
	bool TogglePauseMenu();

	UFUNCTION(BlueprintPure, Category = "Archon|Input")
	bool IsPauseMenuOpen() const { return bPauseMenuOpen; }

	UFUNCTION(BlueprintPure, Category = "Archon|Input")
	float GetMouseLookScale() const { return MouseLookScale; }

	UFUNCTION(BlueprintCallable, Category = "Archon|Input")
	void SetMouseLookScale(float NewScale);

	UFUNCTION(BlueprintCallable, Category = "Archon|Input")
	void QuitToMainMenu();

	UFUNCTION(BlueprintCallable, Category = "Archon|Input")
	void QuitGame();

	static constexpr float MinMouseLookScale = 0.01f;
	static constexpr float MaxMouseLookScale = 0.25f;

	UFUNCTION(BlueprintPure, Category = "Archon|Input")
	bool IsRuntimeBridgeInstalled() const { return bRuntimeBridgeInstalled; }

	UFUNCTION(BlueprintPure, Category = "Archon|Input")
	bool IsMapTableSurfaceOpen() const { return bMapTableSurfaceOpen; }

	UFUNCTION(BlueprintPure, Category = "Archon|Input")
	int32 GetSubmittedOrderCount() const { return SubmittedOrderCount; }

	UFUNCTION(BlueprintPure, Category = "Archon|Input")
	bool HasRuntimeRtsProofSequenceRun() const { return bRuntimeRtsProofSequenceRan; }

	UFUNCTION(BlueprintPure, Category = "Archon|Input")
	bool HasRuntimeMapTableWidgetProofSequenceRun() const { return bRuntimeMapTableWidgetProofSequenceRan; }

	UFUNCTION(BlueprintPure, Category = "Archon|Input")
	bool HasRuntimeWeaponProofSequenceRun() const { return bRuntimeWeaponProofSequenceRan; }

	UFUNCTION(BlueprintPure, Category = "Archon|Input")
	bool IsRuntimeMapTableWidgetMounted() const { return bRuntimeMapTableWidgetMounted; }

	UFUNCTION(BlueprintPure, Category = "Archon|Input")
	int32 GetRuntimeWidgetSubmittedOrderCount() const { return RuntimeWidgetSubmittedOrderCount; }

	UFUNCTION(BlueprintPure, Category = "Archon|Input")
	int32 GetCommandsIssuedDuringDeath() const { return CommandsIssuedDuringDeath; }

	UFUNCTION(BlueprintPure, Category = "Archon|Input")
	FName GetSelectedTechBuildingId() const;

	UFUNCTION(BlueprintPure, Category = "Archon|Input")
	UArchonMapTableWidget* GetRuntimeMapTableWidget() const { return RuntimeMapTableWidget; }

	UFUNCTION(BlueprintPure, Category = "Archon|Input")
	FArchonMapTableInteractionResult GetLastInteractionResult() const { return LastInteractionResult; }

private:
	APlayerController* ResolvePlayerController() const;
	UArchonMapTableInteractorComponent* ResolveInteractor();
	void ApplyStandardFpsLook(APlayerController& Controller);
	void ApplyStandardFpsMovement(APlayerController& Controller);
	void ApplyStandardFpsMobility(APlayerController& Controller);
	void HandleMapTableInput(APlayerController& Controller);
	void HandleFpsWeaponInput(APlayerController& Controller);
	void RequestReinforcementPurchase();
	FArchonRtsCommandIntent MakeRuntimeIntent(EArchonRtsOrderKind OrderKind) const;
	bool OpenRuntimeMapTableWidget();
	void CloseRuntimeMapTableWidget();
	bool OpenPauseMenu();
	bool ClosePauseMenu();
	void ShowInteractPrompt();
	void HideInteractPrompt();
	void ShowMapTableOverlay();
	void HideMapTableOverlay();
	void LogMapTableInputResult(const TCHAR* KeyName, bool bSurfaceOpenBefore, bool bHandled, const FString& Result) const;
	AArchonMatchStateActor* ResolveMatchState() const;
	FString FormatRtsResourceLine() const;
	FString FormatRtsSelectionLine() const;
	FString FormatRtsCommandLine() const;
	FString FormatRtsVisibilityLine() const;
	FString FormatRtsMatchClockLine() const;
	FString FormatRtsProductionLine() const;
	FString FormatRtsSiteControlLine() const;
	FString FormatRtsSelectedStatsLine() const;
	FString FormatRtsControlGroupLine() const;
	FString FormatRtsLastActionLine() const;
	void RequestMapTableVisualEvidence(const TCHAR* EventName) const;
	void RefreshCrosshair(APlayerController& Controller);
	void NotifyPlaytestEvent(const TCHAR* EventName) const;
	void SeedRuntimeMapTableWidgetCandidates();
	bool SelectRuntimeMapTableWidgetCanarySquad();
	bool SubmitRuntimeMapTableWidgetMoveOrder();
	void RecordCommandSubmittedDuringDeath(bool bSubmitted, EArchonLifeState LifeStateBeforeSubmit);
	FName GetSelectedTechBuildingTargetId() const;

	UPROPERTY()
	TObjectPtr<APlayerController> PlayerController;

	UPROPERTY()
	TObjectPtr<AArchonMapTableActor> MapTable;

	UPROPERTY()
	TObjectPtr<UArchonMapTableInteractorComponent> Interactor;

	UPROPERTY()
	FArchonMapTableInteractorConfig InteractorConfig;

	UPROPERTY()
	FArchonMapTableInteractionResult LastInteractionResult;

	UPROPERTY()
	TObjectPtr<UArchonMapTableWidget> RuntimeMapTableWidget;

	UPROPERTY()
	TObjectPtr<UArchonRespawnStateComponent> RespawnState;

	TSharedPtr<SWidget> PauseMenuWidget;
	TSharedPtr<SWidget> InteractPromptWidget;
	TSharedPtr<SWidget> MapTableOverlayWidget;
	TSharedPtr<SWidget> CrosshairWidget;

	bool bRuntimeBridgeInstalled = false;
	bool bPauseMenuOpen = false;
	bool bInteractPromptVisible = false;
	bool bMapTableSurfaceOpen = false;
	bool bAllowMapTableDuringDeath = true;
	bool bRuntimeRtsProofSequenceRan = false;
	bool bRuntimeMapTableWidgetMounted = false;
	bool bRuntimeMapTableWidgetProofSequenceRan = false;
	bool bRuntimeWeaponProofSequenceRan = false;
	int32 SubmittedOrderCount = 0;
	int32 RuntimeWidgetSubmittedOrderCount = 0;
	int32 CommandsIssuedDuringDeath = 0;
	int32 SelectedTechBuildingIndex = 0;
	float DefaultWalkSpeed = 600.0f;
	float SprintWalkSpeed = 900.0f;
	float MouseLookScale = 0.07f;
};
