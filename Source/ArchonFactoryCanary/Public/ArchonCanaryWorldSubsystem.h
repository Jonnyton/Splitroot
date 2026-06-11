#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TimerManager.h"
#include "ArchonValleyLayoutTypes.h"
#include "ArchonCanaryWorldSubsystem.generated.h"

class AArchonCanaryFpsCharacter;
class AArchonMapTableActor;
class AArchonCanaryRtsSquadActor;
class AArchonLenswrightBracewrightActor;
class AArchonLenswrightSundialOpticActor;
class AArchonRespawnObserverPawn;
class APlayerController;
class UArchonPlayerInputBridgeComponent;
class UArchonRespawnStateComponent;
class AArchonVerdantOutpostActor;
class AArchonSplitrootTreeCentralActor;
class AArchonLenswrightOutpostGhostActor;
class AArchonCoverStoneRootVaultActor;
class AArchonBaseCoreActor;
class AArchonMatchStateActor;
class AArchonTechBuildingActor;
class UArchonMainMenuController;
enum class EArchonRtsOrderKind : uint8;
struct FArchonDamageApplicationResult;
struct FArchonRespawnSpawnPoint;
struct FArchonBotStrategyTuning;

class SWidget;

UCLASS()
class ARCHONFACTORYCANARY_API UArchonCanaryWorldSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintPure, Category = "Archon|Proof")
	AArchonMapTableActor* GetRuntimeMapTable() const { return RuntimeMapTable; }

	UFUNCTION(BlueprintPure, Category = "Archon|Proof")
	UArchonPlayerInputBridgeComponent* GetRuntimeInputBridge() const { return RuntimeInputBridge; }

	UFUNCTION(BlueprintPure, Category = "Archon|Proof")
	AArchonCanaryRtsSquadActor* GetRuntimeRtsSquad() const { return RuntimeRtsSquad; }

	UFUNCTION(BlueprintPure, Category = "Archon|Valley")
	bool IsValleyActive() const { return bValleyActive; }

	UFUNCTION(BlueprintPure, Category = "Archon|Valley")
	FVector GetValleyPlayerSpawnLocation() const { return ValleyPlayerSpawnLocation; }

	UFUNCTION(BlueprintPure, Category = "Archon|Valley")
	FRotator GetValleyPlayerSpawnRotation() const { return ValleyPlayerSpawnRotation; }

	// Network clients rebuild the deterministic cosmetic valley locally
	// (triggered by AArchonValleyNetBeacon); replicated gameplay actors
	// (base cores, match state) arrive from the server.
	void SpawnSplitrootValleyForClient();

	// Server-side entry for client-originated RTS orders (pawn RPC →
	// here → the server's own input bridge / map table).
	bool SubmitNetworkedRtsOrder(EArchonRtsOrderKind OrderKind);

	// Bot-match spectating (decision 9, Jonathan 2026-06-10): start in
	// the shared overhead match view; N cycles player viewpoints; M snaps
	// to the friendly bot nearest the map table; F takes the viewed player
	// over (their brain sleeps) or releases them (brain resumes).
	bool IsBotMatchActive() const { return BotPlayers.Num() > 0; }
	void CycleBotView(int32 Direction);
	void ReturnToSharedView();
	void FocusMapTableNearestBot();
	void ToggleBotTakeOver();

	// Event-driven playtest capture (Jonathan 2026-06-10): periodic shots
	// miss whole action sequences, so every meaningful beat (map table,
	// orders, pause menu, host/join, fire, death/respawn) takes an
	// immediate rate-limited shot with the event in the filename.
	void NotifyPlaytestEvent(const TCHAR* EventName);

private:
	void SpawnRuntimeRtsSquad();
	void InitializeRuntimeTeamVisibility();
	void InstallRuntimePlayerBridge();
	void InstallRuntimeRespawnState(APlayerController& PlayerController);
	void RegisterRuntimeRespawnSpawnPoints();
	void BindRuntimeFpsDeath(AArchonCanaryFpsCharacter* FpsCharacter);
	void HandleRuntimeFpsDeath(const FArchonDamageApplicationResult& DamageResult);
	void HandleRuntimeRespawnReady(const FArchonRespawnSpawnPoint& SpawnPoint);
	void RunRuntimeRtsProofSequenceIfRequested();
	void RunRuntimeTechShopProofSequenceIfRequested();
	void ExecuteRuntimeTechShopProofSequence();
	void RunRuntimeRallyProofSequenceIfRequested();
	void ExecuteRuntimeRallyProofSequence();
	void RunRuntimeWeaponProofSequenceIfRequested();
	void RunRuntimeCombatProofSequenceIfRequested();
	void RunRuntimeRespawnProofSequenceIfRequested();
	void SpawnRuntimeLenswrightCombatActors();
	void SpawnBlockoutActorsIfRequested();
	void SpawnSplitrootValleyIfRequested();
	void SpawnSplitrootValleyInternal(bool bCosmeticOnly);
	void SpawnValleyLightingIfNeeded();
	void SpawnMatchLoopIfRequested();
	void SpawnBotMatchIfRequested();
	AArchonCanaryFpsCharacter* SpawnFieldedBot(
		int32 Team,
		AArchonBaseCoreActor* OwnCore,
		AArchonBaseCoreActor* EnemyCore,
		const TArray<FArchonValleyPlacement>& RouteSites,
		const FArchonBotStrategyTuning& StrategyTuning);
	void HandleReinforcementPurchased(int32 TeamId, int32 SquadSize);
	void HandleTechBuildingPurchased(int32 TeamId, FName TechId);
	FVector ResolveTechBuildingSlot(int32 TeamId, FName TechId) const;
	void SpawnMainMenuIfRequested();
	void ApplyMainMenuInputMode();
	void RunFirst60SecondsProofIfRequested();
	void SpawnSecond60Enemies();
	void RunSecond60SecondsProofIfRequested();
	void RunClientFireProofIfRequested();
	void ExecuteClientFireProofBridgeShot();
	void ExecuteClientFireProofCoreShot();
	void RunClientOrderProofIfRequested();
	void ExecuteClientRtsOrderProof();
	void RunPauseMenuProofIfRequested();
	void ExecutePauseMenuProof();
	void RunInteractProofIfRequested();
	APawn* ResolveSpectatorPawn();
	void ShowSpectatorBannerIfNeeded();
	void HandleControlledBotDeath(const FArchonDamageApplicationResult& DamageResult);
	void ShowBuildVersionStampIfNeeded();
	void ShowMatchHudIfNeeded();
	AArchonMatchStateActor* ResolveMatchState();
	void StartPlaytestRecorderIfHumanSession();
	void TakePlaytestRecorderShot();
	void SchedulePlayTestScreenshotsIfRequested();
	void FramePlayTestCameraForScreenshot();
	void TakePlayTestScreenshot();
	void CapturePlayTestScreenshot();
	void IssuePlayTestQuit();

	UPROPERTY()
	TObjectPtr<AArchonMapTableActor> RuntimeMapTable;

	UPROPERTY()
	TObjectPtr<AArchonCanaryFpsCharacter> RuntimeFpsCharacter;

	UPROPERTY()
	TObjectPtr<AArchonCanaryRtsSquadActor> RuntimeRtsSquad;

	UPROPERTY()
	TObjectPtr<UArchonPlayerInputBridgeComponent> RuntimeInputBridge;

	UPROPERTY()
	TObjectPtr<UArchonRespawnStateComponent> RuntimeRespawnState;

	UPROPERTY()
	TObjectPtr<AArchonRespawnObserverPawn> RuntimeRespawnObserver;

	UPROPERTY()
	TObjectPtr<AArchonLenswrightBracewrightActor> RuntimeLenswrightBracewright;

	UPROPERTY()
	TObjectPtr<AArchonLenswrightSundialOpticActor> RuntimeLenswrightSundial;

	FTimerHandle RuntimeBridgeInstallTimer;
	FTimerHandle RuntimeTechShopProofTimer;
	FTimerHandle RuntimeRallyProofTimer;
	FTimerHandle PlayTestScreenshotTimer;
	FTimerHandle PlayTestCaptureTimer;
	FTimerHandle PlayTestQuitTimer;
	FTimerHandle MatchProofQuitTimer;
	FTimerHandle ClientFireProofBridgeShotTimer;
	FTimerHandle ClientFireProofCoreShotTimer;
	FTimerHandle ClientOrderProofTimer;
	FTimerHandle PlaytestRecorderTimer;
	FString PlaytestRecorderDir;
	int32 PlaytestRecorderShotIndex = 0;
	TMap<FName, double> LastPlaytestEventShotSeconds;
	int32 RuntimeBridgeInstallAttempts = 0;
	int32 PlayTestScreenshotsTaken = 0;
	bool bRuntimeRtsProofSequenceRan = false;
	bool bRuntimeTechShopProofScheduled = false;
	bool bRuntimeTechShopProofSequenceRan = false;
	bool bRuntimeRallyProofScheduled = false;
	bool bRuntimeRallyProofSequenceRan = false;
	bool bRuntimeWeaponProofSequenceRan = false;
	bool bRuntimeCombatProofSequenceRan = false;
	bool bRuntimeRespawnProofSequenceRan = false;
	bool bFirst60ProofSequenceRan = false;
	bool bSecond60ProofSequenceRan = false;
	bool bClientFireProofScheduled = false;
	bool bClientOrderProofScheduled = false;
	bool bPauseMenuProofScheduled = false;
	bool bInteractProofRan = false;
	bool bBlockoutSpawned = false;
	bool bPlayTestScreenshotScheduled = false;
	bool bValleyActive = false;
	FVector ValleyMapTableLocation = FVector::ZeroVector;
	FVector ValleyPlayerSpawnLocation = FVector::ZeroVector;
	FRotator ValleyPlayerSpawnRotation = FRotator::ZeroRotator;
	FVector ValleySquadSpawnLocation = FVector::ZeroVector;

	UPROPERTY()
	TObjectPtr<AArchonVerdantOutpostActor> BlockoutVerdantOutpost;

	UPROPERTY()
	TObjectPtr<AArchonSplitrootTreeCentralActor> BlockoutSplitrootTree;

	UPROPERTY()
	TObjectPtr<AArchonLenswrightOutpostGhostActor> BlockoutLenswrightGhost;

	UPROPERTY()
	TArray<TObjectPtr<AArchonCoverStoneRootVaultActor>> BlockoutCoverStones;

	UPROPERTY()
	TArray<TObjectPtr<AArchonLenswrightBracewrightActor>> Second60Bracewrights;

	UPROPERTY()
	TObjectPtr<AArchonLenswrightSundialOpticActor> Second60Sundial;

	UPROPERTY()
	TArray<TObjectPtr<AActor>> ValleyActors;

	UPROPERTY()
	TArray<TObjectPtr<AArchonBaseCoreActor>> ValleyBaseCores;

	UPROPERTY()
	TArray<TObjectPtr<AArchonTechBuildingActor>> ValleyTechBuildings;

	UPROPERTY()
	TArray<TObjectPtr<AArchonCanaryFpsCharacter>> BotPlayers;

	// Income loop: monotonic bot index (role mix + respawn rings key off
	// it) and per-team field counts for the reinforcement cap.
	int32 NextBotIndex = 0;
	int32 FieldedBotsPerTeam[2] = { 0, 0 };

	UPROPERTY()
	TObjectPtr<APawn> SpectatorPawn;

	UPROPERTY()
	TObjectPtr<AArchonCanaryFpsCharacter> ControlledBot;

	TSharedPtr<SWidget> SpectatorBannerWidget;
	TSharedPtr<SWidget> BuildVersionWidget;
	TSharedPtr<SWidget> MatchHudWidget;
	TWeakObjectPtr<AArchonMatchStateActor> WeakMatchState;
	int32 ViewedBotIndex = INDEX_NONE;

	UPROPERTY()
	TObjectPtr<AArchonMatchStateActor> MatchState;

	UPROPERTY()
	TObjectPtr<UArchonMainMenuController> MainMenu;

	FTimerHandle MainMenuProofTimer;
	FTimerHandle MainMenuCursorTimer;

	// The menu panel lives on the game viewport, which SURVIVES travel —
	// without explicit removal it stays painted over the hosted match
	// (player-test finding 2026-06-10).
	TSharedPtr<SWidget> MainMenuPanelWidget;
};
