#include "ArchonPlayerInputBridgeComponent.h"
#include "ArchonCanaryFpsCharacter.h"
#include "ArchonCanaryWorldSubsystem.h"
#include "ArchonInteractPromptPolicyLibrary.h"
#include "ArchonFactionMovementComponent.h"
#include "ArchonItemShopPolicyLibrary.h"
#include "ArchonMapTableActor.h"
#include "ArchonMapTableInteractorComponent.h"
#include "ArchonMatchStateActor.h"
#include "EngineUtils.h"
#include "ArchonMapTableWidget.h"
#include "ArchonPauseMenuPanel.h"
#include "ArchonRespawnStateComponent.h"
#include "ArchonVerdantThornsproutBow.h"
#include "Camera/CameraComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "HAL/PlatformFileManager.h"
#include "InputCoreTypes.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/DateTime.h"
#include "Misc/Paths.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SGridPanel.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"
#include "UnrealClient.h"

namespace
{
	const TArray<FName>& GetBuildableTechIds()
	{
		static const TArray<FName> TechIds = {
			FName(TEXT("armory")),
			FName(TEXT("beast_den")),
			FName(TEXT("grove"))
		};
		return TechIds;
	}

	FName ResolveTechTargetId(FName TechId)
	{
		if (TechId == TEXT("beast_den"))
		{
			return FName(TEXT("kinwild_beast_den_slot"));
		}
		if (TechId == TEXT("grove"))
		{
			return FName(TEXT("verdant_grove_slot"));
		}
		return FName(TEXT("verdant_armory_slot"));
	}

	int32 ResolveTechBuildCost(FName TechId)
	{
		return TechId.IsNone() ? 0 : 5;
	}
}

UArchonPlayerInputBridgeComponent::UArchonPlayerInputBridgeComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UArchonPlayerInputBridgeComponent::BeginPlay()
{
	Super::BeginPlay();
	PlayerController = ResolvePlayerController();

	// User setting: look speed persists across sessions (pause-menu slider).
	float SavedScale = 0.0f;
	if (GConfig && GConfig->GetFloat(TEXT("ArchonControls"), TEXT("MouseLookScale"), SavedScale, GGameUserSettingsIni) && SavedScale > 0.0f)
	{
		MouseLookScale = FMath::Clamp(SavedScale, MinMouseLookScale, MaxMouseLookScale);
		UE_LOG(LogTemp, Display, TEXT("ArchonFactoryCanary: MouseLookScaleLoaded scale=%.3f"), MouseLookScale);
	}
}

void UArchonPlayerInputBridgeComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// The viewport outlives this world across travel; pull our widgets
	// off it or they stay painted over the next map (same trap as the
	// main menu panel, found in the 2026-06-10 player test).
	if (PauseMenuWidget.IsValid() && GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(PauseMenuWidget.ToSharedRef());
	}
	PauseMenuWidget.Reset();
	bPauseMenuOpen = false;

	if (InteractPromptWidget.IsValid() && GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(InteractPromptWidget.ToSharedRef());
	}
	InteractPromptWidget.Reset();
	bInteractPromptVisible = false;

	if (MapTableOverlayWidget.IsValid() && GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(MapTableOverlayWidget.ToSharedRef());
	}
	MapTableOverlayWidget.Reset();

	if (CrosshairWidget.IsValid() && GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(CrosshairWidget.ToSharedRef());
	}
	CrosshairWidget.Reset();

	Super::EndPlay(EndPlayReason);
}

void UArchonPlayerInputBridgeComponent::ConfigureBridge(
	APlayerController* InPlayerController,
	AArchonMapTableActor* InMapTable,
	const FArchonMapTableInteractorConfig& InInteractorConfig)
{
	PlayerController = InPlayerController ? InPlayerController : ResolvePlayerController();
	MapTable = InMapTable;
	InteractorConfig = InInteractorConfig;
	bRuntimeBridgeInstalled = PlayerController != nullptr && MapTable != nullptr && ResolveInteractor() != nullptr;
	RuntimeMapTableWidget = nullptr;
	bRuntimeMapTableWidgetMounted = false;
	bRuntimeMapTableWidgetProofSequenceRan = false;
	RuntimeWidgetSubmittedOrderCount = 0;
	CommandsIssuedDuringDeath = 0;

	if (bRuntimeBridgeInstalled)
	{
		UE_LOG(
			LogTemp,
			Display,
			TEXT("ArchonFactoryCanary: RuntimeInputBridgeInstalled controller=%s mapTable=%s"),
			*PlayerController->GetName(),
			*MapTable->GetName());
	}
}

void UArchonPlayerInputBridgeComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	APlayerController* Controller = ResolvePlayerController();
	if (!Controller)
	{
		return;
	}

	if (Controller->WasInputKeyJustPressed(EKeys::Escape))
	{
		TogglePauseMenu();
	}

	// The system menu owns input while open: no look, no movement, no
	// weapon fire, no map-table keys — only the menu's own widgets.
	if (bPauseMenuOpen)
	{
		return;
	}

	RefreshInteractPrompt();
	RefreshCrosshair(*Controller);

	// Bot-match spectating: N cycles player views, M stages the view at
	// the bot nearest the map table, F takes over / releases the viewed
	// player.
	UWorld* BridgeWorld = GetWorld();
	UArchonCanaryWorldSubsystem* Canary = BridgeWorld ? BridgeWorld->GetSubsystem<UArchonCanaryWorldSubsystem>() : nullptr;
	if (Canary && Canary->IsBotMatchActive() && !bMapTableSurfaceOpen)
	{
		if (Controller->WasInputKeyJustPressed(EKeys::N))
		{
			Canary->CycleBotView(1);
		}
		if (Controller->WasInputKeyJustPressed(EKeys::M))
		{
			Canary->FocusMapTableNearestBot();
		}
		if (Controller->WasInputKeyJustPressed(EKeys::F))
		{
			Canary->ToggleBotTakeOver();
		}
	}

	if (Controller->WasInputKeyJustPressed(EKeys::V) && !bMapTableSurfaceOpen)
	{
		if (AArchonCanaryFpsCharacter* CanaryCharacter = Cast<AArchonCanaryFpsCharacter>(Controller->GetPawn()))
		{
			CanaryCharacter->ToggleCameraView();
			NotifyPlaytestEvent(TEXT("camera_toggle"));
		}
	}

	// Using an interface locks the body (Jonathan 2026-06-10): while the
	// command surface is open the character cannot move or look — the
	// mouse belongs to the RTS. Tab/E/LMB table input stays live below.
	if (!bMapTableSurfaceOpen)
	{
		ApplyStandardFpsLook(*Controller);
		ApplyStandardFpsMovement(*Controller);
		ApplyStandardFpsMobility(*Controller);
	}
	HandleMapTableInput(*Controller);
	HandleFpsWeaponInput(*Controller);
}

void UArchonPlayerInputBridgeComponent::RefreshInteractPrompt()
{
	APlayerController* Controller = ResolvePlayerController();
	APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;

	bool bShouldShow = false;
	if (Pawn && MapTable)
	{
		bShouldShow = UArchonInteractPromptPolicyLibrary::EvaluateInteractPrompt(
			Pawn->GetActorLocation(),
			MapTable->GetActorLocation(),
			UArchonInteractPromptPolicyLibrary::DefaultInteractRadius,
			bMapTableSurfaceOpen).bShowPrompt;
	}

	if (bShouldShow == bInteractPromptVisible)
	{
		return;
	}

	if (bShouldShow)
	{
		ShowInteractPrompt();
	}
	else
	{
		HideInteractPrompt();
	}
}

bool UArchonPlayerInputBridgeComponent::TryInteract()
{
	APlayerController* Controller = ResolvePlayerController();
	APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
	if (!Pawn || !MapTable)
	{
		LastInteractionResult = FArchonMapTableInteractionResult();
		LastInteractionResult.Reason = Pawn ? TEXT("no_map_table") : TEXT("no_pawn");
		return false;
	}

	const FArchonInteractPromptDecision Decision = UArchonInteractPromptPolicyLibrary::EvaluateInteractPrompt(
		Pawn->GetActorLocation(),
		MapTable->GetActorLocation(),
		UArchonInteractPromptPolicyLibrary::DefaultInteractRadius,
		bMapTableSurfaceOpen);
	if (!Decision.bInRange)
	{
		LastInteractionResult = FArchonMapTableInteractionResult();
		LastInteractionResult.Reason = TEXT("out_of_range");
		UE_LOG(LogTemp, Display, TEXT("ArchonFactoryCanary: InteractPressed opened=false reason=out_of_range"));
		return false;
	}

	UE_LOG(LogTemp, Display, TEXT("ArchonFactoryCanary: InteractPressed opened=true target=map_table"));
	return PreviewRuntimeMapTable();
}

void UArchonPlayerInputBridgeComponent::LogMapTableInputResult(
	const TCHAR* KeyName,
	bool bSurfaceOpenBefore,
	bool bHandled,
	const FString& Result) const
{
	APlayerController* ActiveController = ResolvePlayerController();
	APawn* Pawn = ActiveController ? ActiveController->GetPawn() : nullptr;
	const FString PawnName = Pawn ? Pawn->GetName() : TEXT("none");
	const float Distance2D = (Pawn && MapTable)
		? FVector::Dist2D(Pawn->GetActorLocation(), MapTable->GetActorLocation())
		: -1.0f;

	UE_LOG(
		LogTemp,
		Display,
		TEXT("ArchonFactoryCanary: MapTableInput key=%s surfaceOpenBefore=%s surfaceOpenAfter=%s handled=%s result=%s distance2D=%.1f pawn=%s widgetMounted=%s overlayVisible=%s"),
		KeyName ? KeyName : TEXT("unknown"),
		bSurfaceOpenBefore ? TEXT("true") : TEXT("false"),
		bMapTableSurfaceOpen ? TEXT("true") : TEXT("false"),
		bHandled ? TEXT("true") : TEXT("false"),
		*Result,
		Distance2D,
		*PawnName,
		bRuntimeMapTableWidgetMounted ? TEXT("true") : TEXT("false"),
		MapTableOverlayWidget.IsValid() ? TEXT("true") : TEXT("false"));
}

AArchonMatchStateActor* UArchonPlayerInputBridgeComponent::ResolveMatchState() const
{
	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<AArchonMatchStateActor> It(World); It; ++It)
		{
			return *It;
		}
	}
	return nullptr;
}

FName UArchonPlayerInputBridgeComponent::GetSelectedTechBuildingId() const
{
	const TArray<FName>& TechIds = GetBuildableTechIds();
	return TechIds.IsValidIndex(SelectedTechBuildingIndex)
		? TechIds[SelectedTechBuildingIndex]
		: TechIds[0];
}

bool UArchonPlayerInputBridgeComponent::SelectTechBuilding(FName TechId)
{
	const TArray<FName>& TechIds = GetBuildableTechIds();
	for (int32 Index = 0; Index < TechIds.Num(); ++Index)
	{
		if (TechIds[Index] == TechId)
		{
			SelectedTechBuildingIndex = Index;
			UE_LOG(
				LogTemp,
				Display,
				TEXT("ArchonFactoryCanary: SelectedTechBuilding tech=%s target=%s index=%d"),
				*GetSelectedTechBuildingId().ToString(),
				*GetSelectedTechBuildingTargetId().ToString(),
				SelectedTechBuildingIndex);
			return true;
		}
	}

	return false;
}

FName UArchonPlayerInputBridgeComponent::CycleSelectedTechBuilding(int32 Direction)
{
	const TArray<FName>& TechIds = GetBuildableTechIds();
	if (TechIds.Num() <= 0)
	{
		return NAME_None;
	}

	const int32 Step = Direction == 0 ? 1 : Direction;
	SelectedTechBuildingIndex = (SelectedTechBuildingIndex + Step) % TechIds.Num();
	if (SelectedTechBuildingIndex < 0)
	{
		SelectedTechBuildingIndex += TechIds.Num();
	}

	UE_LOG(
		LogTemp,
		Display,
		TEXT("ArchonFactoryCanary: SelectedTechBuilding tech=%s target=%s index=%d"),
		*GetSelectedTechBuildingId().ToString(),
		*GetSelectedTechBuildingTargetId().ToString(),
		SelectedTechBuildingIndex);
	return GetSelectedTechBuildingId();
}

FName UArchonPlayerInputBridgeComponent::GetSelectedTechBuildingTargetId() const
{
	return ResolveTechTargetId(GetSelectedTechBuildingId());
}

FString UArchonPlayerInputBridgeComponent::FormatRtsResourceLine() const
{
	const AArchonMatchStateActor* MatchState = ResolveMatchState();
	if (!MatchState)
	{
		return TEXT("SUPPLY --   ENEMY --   SCORE --   BASE STATE UNKNOWN");
	}

	const FArchonMatchNetSnapshot Snapshot = MatchState->GetNetSnapshot();
	const bool bTeamB = InteractorConfig.TeamId == 1;
	const int32 OwnSupply = bTeamB ? Snapshot.SupplyB : Snapshot.SupplyA;
	const int32 EnemySupply = bTeamB ? Snapshot.SupplyA : Snapshot.SupplyB;
	const int32 OwnPoints = bTeamB ? Snapshot.PointsB : Snapshot.PointsA;
	const int32 EnemyPoints = bTeamB ? Snapshot.PointsA : Snapshot.PointsB;
	const FString Alert = bTeamB ? Snapshot.BaseAlertB : Snapshot.BaseAlertA;

	return FString::Printf(
		TEXT("SUPPLY %d   ENEMY %d   SCORE %d-%d   %s"),
		OwnSupply,
		EnemySupply,
		OwnPoints,
		EnemyPoints,
		Alert.IsEmpty() ? TEXT("BASE CLEAR") : *Alert);
}

FString UArchonPlayerInputBridgeComponent::FormatRtsSelectionLine() const
{
	const int32 SelectedCount = RuntimeMapTableWidget ? RuntimeMapTableWidget->GetSelectedSquadIds().Num() : 0;

	return FString::Printf(
		TEXT("CANARY SQUAD   GROUP 1   SELECTED %d   INFANTRY CONTROL GROUP   ATTACK-MOVE RALLY"),
		SelectedCount);
}

FString UArchonPlayerInputBridgeComponent::FormatRtsCommandLine() const
{
	const int32 Sequence = MapTable ? MapTable->GetCurrentCommandSequence() : 0;
	return FString::Printf(
		TEXT("ORDERS %d   WIDGET ORDERS %d   SEQUENCE %d   TECH %s"),
		SubmittedOrderCount,
		RuntimeWidgetSubmittedOrderCount,
		Sequence,
		*GetSelectedTechBuildingId().ToString());
}

FString UArchonPlayerInputBridgeComponent::FormatRtsVisibilityLine() const
{
	return RuntimeMapTableWidget
		? RuntimeMapTableWidget->FormatVisibilitySummary()
		: FString(TEXT("VISION unavailable"));
}

FString UArchonPlayerInputBridgeComponent::FormatRtsMatchClockLine() const
{
	const AArchonMatchStateActor* MatchState = ResolveMatchState();
	if (!MatchState)
	{
		return TEXT("MATCH --:--   PHASE UNKNOWN");
	}

	const FArchonMatchClock Clock = MatchState->GetMatchClock();
	const int32 ElapsedSeconds = FMath::Max(0, FMath::FloorToInt(Clock.PhaseSeconds));
	const int32 Minutes = ElapsedSeconds / 60;
	const int32 Seconds = ElapsedSeconds % 60;
	const TCHAR* PhaseLabel = TEXT("WARMUP");
	switch (Clock.Phase)
	{
	case EArchonMatchPhase::Live:
		PhaseLabel = TEXT("LIVE");
		break;
	case EArchonMatchPhase::MatchEnd:
		PhaseLabel = TEXT("MATCH END");
		break;
	case EArchonMatchPhase::Traveling:
		PhaseLabel = TEXT("TRAVELING");
		break;
	case EArchonMatchPhase::Warmup:
	default:
		break;
	}

	return FString::Printf(TEXT("MATCH %02d:%02d   %s"), Minutes, Seconds, PhaseLabel);
}

FString UArchonPlayerInputBridgeComponent::FormatRtsProductionLine() const
{
	const AArchonMatchStateActor* MatchState = ResolveMatchState();
	if (!MatchState)
	{
		return TEXT("PRODUCTION unavailable");
	}

	const int32 TeamId = MapTable ? MapTable->GetTeamId() : InteractorConfig.TeamId;
	const int32 Supply = MatchState->GetTeamSupply(TeamId);
	const FArchonMatchConfig DefaultConfig;
	const bool bCanTrain = Supply >= DefaultConfig.ReinforcementSquadSupplyCost;
	const bool bHasArmory = MatchState->GetBuiltTech(TeamId).Contains(TEXT("armory"));

	return FString::Printf(
		TEXT("TRAIN %s  COST %d  ARMORY %s  CROSSBOW %s"),
		bCanTrain ? TEXT("READY") : TEXT("WAIT"),
		DefaultConfig.ReinforcementSquadSupplyCost,
		bHasArmory ? TEXT("BUILT") : TEXT("MISSING"),
		bHasArmory ? TEXT("UNLOCKED") : TEXT("LOCKED"));
}

FString UArchonPlayerInputBridgeComponent::FormatRtsSiteControlLine() const
{
	const AArchonMatchStateActor* MatchState = ResolveMatchState();
	if (!MatchState)
	{
		return TEXT("SITES --");
	}

	const int32 TeamId = MapTable ? MapTable->GetTeamId() : InteractorConfig.TeamId;
	int32 Owned = 0;
	int32 EnemyOwned = 0;
	int32 Neutral = 0;
	for (int32 SiteIndex = 0; SiteIndex < MatchState->GetSiteCount(); ++SiteIndex)
	{
		FVector SiteLocation = FVector::ZeroVector;
		int32 OwningTeam = INDEX_NONE;
		if (!MatchState->GetSiteInfo(SiteIndex, SiteLocation, OwningTeam))
		{
			continue;
		}
		if (OwningTeam == TeamId)
		{
			++Owned;
		}
		else if (OwningTeam == INDEX_NONE)
		{
			++Neutral;
		}
		else
		{
			++EnemyOwned;
		}
	}

	return FString::Printf(TEXT("SITES OWN %d   ENEMY %d   NEUTRAL %d"), Owned, EnemyOwned, Neutral);
}

FString UArchonPlayerInputBridgeComponent::FormatRtsSelectedStatsLine() const
{
	const FString Target = (RuntimeMapTableWidget && RuntimeMapTableWidget->HasSubmittedOrder())
		? RuntimeMapTableWidget->GetLastOrderTargetLocation().ToCompactString()
		: FString(TEXT("none"));

	return FString::Printf(
		TEXT("HP 574/630   MANA 83/140   DMG 18-24   ARMOR 2   TARGET %s"),
		*Target);
}

FString UArchonPlayerInputBridgeComponent::FormatRtsControlGroupLine() const
{
	return TEXT("1 CANARY SELECTED   2 ROOT   3 BUILDER   F1 HERO");
}

FString UArchonPlayerInputBridgeComponent::FormatRtsLastActionLine() const
{
	const int32 Sequence = MapTable ? MapTable->GetCurrentCommandSequence() : 0;
	const FString Target = (RuntimeMapTableWidget && RuntimeMapTableWidget->HasSubmittedOrder())
		? RuntimeMapTableWidget->GetLastOrderTargetLocation().ToCompactString()
		: FString(TEXT("none"));

	return FString::Printf(
		TEXT("LAST ACTION seq=%d orders=%d widget=%d target=%s"),
		Sequence,
		SubmittedOrderCount,
		RuntimeWidgetSubmittedOrderCount,
		*Target);
}

void UArchonPlayerInputBridgeComponent::RequestMapTableVisualEvidence(const TCHAR* EventName) const
{
	if (!GetWorld())
	{
		return;
	}

	FString SafeEvent = EventName ? FString(EventName) : FString(TEXT("map_table"));
	SafeEvent.ReplaceInline(TEXT(" "), TEXT("_"));
	SafeEvent.ReplaceInline(TEXT("/"), TEXT("_"));
	SafeEvent.ReplaceInline(TEXT("\\"), TEXT("_"));

	const FDateTime Now = FDateTime::UtcNow();
	const FString EvidenceDir = FPaths::ProjectSavedDir() / TEXT("PlaytestCaptures") / TEXT("MapTableEvidence");
	FPlatformFileManager::Get().GetPlatformFile().CreateDirectoryTree(*EvidenceDir);
	const FString FileName = EvidenceDir / FString::Printf(
		TEXT("%s_%lld_%s.png"),
		*Now.ToString(TEXT("%Y%m%d-%H%M%S")),
		static_cast<long long>(Now.GetTicks()),
		*SafeEvent);

	FScreenshotRequest::RequestScreenshot(FileName, /*bInShowUI=*/ true, /*bAddFilenameSuffix=*/ false);
	UE_LOG(
		LogTemp,
		Display,
		TEXT("ArchonFactoryCanary: MapTableVisualEvidence requested=true event=%s file=%s clock=\"%s\" resources=\"%s\" sites=\"%s\" production=\"%s\" selection=\"%s\" stats=\"%s\" commands=\"%s\" lastAction=\"%s\" visibility=\"%s\" overlayMode=rts_command_cockpit_wc3_reference worldAlpha=0.22 hudAlpha=0.86"),
		EventName ? EventName : TEXT("map_table"),
		*FileName,
		*FormatRtsMatchClockLine(),
		*FormatRtsResourceLine(),
		*FormatRtsSiteControlLine(),
		*FormatRtsProductionLine(),
		*FormatRtsSelectionLine(),
		*FormatRtsSelectedStatsLine(),
		*FormatRtsCommandLine(),
		*FormatRtsLastActionLine(),
		*FormatRtsVisibilityLine());
}

void UArchonPlayerInputBridgeComponent::RefreshCrosshair(APlayerController& Controller)
{
	// Standard FPS aim reference (Jonathan 2026-06-10: "I don't seem to
	// have a cursor and crosshairs"). Shown only while embodied with the
	// world in FPS control.
	const bool bShouldShow =
		!bPauseMenuOpen &&
		!bMapTableSurfaceOpen &&
		Cast<AArchonCanaryFpsCharacter>(Controller.GetPawn()) != nullptr;

	if (bShouldShow && !CrosshairWidget.IsValid() && GEngine && GEngine->GameViewport)
	{
		TSharedRef<SWidget> Crosshair =
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().FillHeight(1.0f) [ SNew(SSpacer) ]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			[
				SNew(SBox)
				.WidthOverride(5.0f)
				.HeightOverride(5.0f)
				[
					SNew(SBorder)
					.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
					.BorderBackgroundColor(FSlateColor(FLinearColor(0.95f, 0.98f, 0.9f, 0.9f)))
				]
			]
			+ SVerticalBox::Slot().FillHeight(1.0f) [ SNew(SSpacer) ];
		CrosshairWidget = Crosshair;
		GEngine->GameViewport->AddViewportWidgetContent(Crosshair, /*ZOrder=*/40);
	}
	else if (!bShouldShow && CrosshairWidget.IsValid() && GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(CrosshairWidget.ToSharedRef());
		CrosshairWidget.Reset();
	}
}

void UArchonPlayerInputBridgeComponent::ShowMapTableOverlay()
{
	if (MapTableOverlayWidget.IsValid() || !GEngine || !GEngine->GameViewport)
	{
		return;
	}

	{
		TWeakObjectPtr<UArchonPlayerInputBridgeComponent> WeakThis(this);
		const auto MakeText = [](
			const FText& Text,
			int32 Size,
			const FLinearColor& Color,
			FName Typeface) -> TSharedRef<SWidget>
		{
			return SNew(STextBlock)
				.Text(Text)
				.Font(FCoreStyle::GetDefaultFontStyle(Typeface, Size))
				.ColorAndOpacity(FSlateColor(Color));
		};

		const auto MakeDivider = []() -> TSharedRef<SWidget>
		{
			return SNew(SBox)
				.HeightOverride(1.0f)
				[
					SNew(SBorder)
					.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
					.BorderBackgroundColor(FSlateColor(FLinearColor(0.74f, 0.88f, 0.66f, 0.32f)))
				];
		};

		const auto MakeMapCell = [](const FLinearColor& Color) -> TSharedRef<SWidget>
		{
			return SNew(SBox)
				.WidthOverride(38.0f)
				.HeightOverride(30.0f)
				[
					SNew(SBorder)
					.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
					.BorderBackgroundColor(FSlateColor(Color))
				];
		};

		const auto MakeStatChip = [&](
			const TCHAR* Label,
			TFunction<FText()> ValueGetter) -> TSharedRef<SWidget>
		{
			TFunction<FText()> LocalValueGetter = MoveTemp(ValueGetter);
			return SNew(SBox)
				.WidthOverride(106.0f)
				.HeightOverride(34.0f)
				[
					SNew(SBorder)
					.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
					.BorderBackgroundColor(FSlateColor(FLinearColor(0.018f, 0.028f, 0.022f, 0.66f)))
					.Padding(FMargin(6.0f, 3.0f))
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							MakeText(FText::FromString(FString(Label)), 9, FLinearColor(0.64f, 0.70f, 0.58f, 0.92f), FName(TEXT("Regular")))
						]
						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							SNew(STextBlock)
							.Text_Lambda([ValueGetter = MoveTemp(LocalValueGetter)]() -> FText
							{
								return ValueGetter ? ValueGetter() : FText::GetEmpty();
							})
							.Font(FCoreStyle::GetDefaultFontStyle(FName(TEXT("Bold")), 11))
							.ColorAndOpacity(FSlateColor(FLinearColor(0.92f, 0.98f, 0.82f, 1.0f)))
						]
					]
				];
		};

		const auto MakeControlGroupPill = [&](
			const TCHAR* Key,
			const TCHAR* Label,
			bool bActive) -> TSharedRef<SWidget>
		{
			const FLinearColor Fill = bActive
				? FLinearColor(0.12f, 0.26f, 0.10f, 0.84f)
				: FLinearColor(0.036f, 0.048f, 0.038f, 0.72f);
			const FLinearColor TextColor = bActive
				? FLinearColor(0.92f, 0.98f, 0.78f, 1.0f)
				: FLinearColor(0.62f, 0.68f, 0.58f, 0.92f);

			return SNew(SBox)
				.WidthOverride(82.0f)
				.HeightOverride(22.0f)
				[
					SNew(SBorder)
					.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
					.BorderBackgroundColor(FSlateColor(Fill))
					.Padding(FMargin(6.0f, 2.0f))
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							MakeText(FText::FromString(FString(Key)), 9, FLinearColor(0.72f, 0.96f, 0.38f, 1.0f), FName(TEXT("Bold")))
						]
						+ SHorizontalBox::Slot()
						.FillWidth(1.0f)
						.HAlign(HAlign_Right)
						[
							MakeText(FText::FromString(FString(Label)), 9, TextColor, FName(TEXT("Bold")))
						]
					]
				];
		};

		const auto MakeUnitCard = [&](
			const TCHAR* Name,
			const TCHAR* Status,
			const FLinearColor& Accent) -> TSharedRef<SWidget>
		{
			return SNew(SBox)
				.WidthOverride(96.0f)
				.HeightOverride(54.0f)
				[
					SNew(SBorder)
					.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
					.BorderBackgroundColor(FSlateColor(FLinearColor(0.035f, 0.058f, 0.046f, 0.78f)))
					.Padding(FMargin(6.0f, 4.0f))
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							MakeText(FText::FromString(FString(Name)), 11, Accent, FName(TEXT("Bold")))
						]
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.0f, 3.0f, 0.0f, 0.0f)
						[
							MakeText(FText::FromString(FString(Status)), 10, FLinearColor(0.82f, 0.90f, 0.76f, 0.95f), FName(TEXT("Regular")))
						]
					]
				];
		};

		const auto MakeCommandButton = [&](
			const TCHAR* Key,
			const TCHAR* Label,
			const TCHAR* Detail,
			const FLinearColor& Accent,
			bool bEnabled,
			TFunction<FReply()> OnClicked) -> TSharedRef<SWidget>
		{
			const FLinearColor PanelColor = bEnabled
				? FLinearColor(0.052f, 0.072f, 0.058f, 0.88f)
				: FLinearColor(0.030f, 0.036f, 0.030f, 0.58f);
			const FLinearColor MainText = bEnabled
				? FLinearColor(0.96f, 0.98f, 0.88f, 1.0f)
				: FLinearColor(0.54f, 0.56f, 0.52f, 0.75f);
			const FLinearColor SubText = bEnabled
				? FLinearColor(0.72f, 0.79f, 0.66f, 0.9f)
				: FLinearColor(0.42f, 0.45f, 0.40f, 0.65f);
			const FString SlotKey(Key);
			const FString SlotLabel(Label);

			return SNew(SBox)
				.WidthOverride(92.0f)
				.HeightOverride(56.0f)
				[
					SNew(SButton)
					.IsEnabled(bEnabled)
					.ContentPadding(0.0f)
					.ButtonColorAndOpacity(FSlateColor(FLinearColor(1.0f, 1.0f, 1.0f, bEnabled ? 0.72f : 0.30f)))
					.OnClicked_Lambda([WeakThis, bEnabled, OnClicked, SlotKey, SlotLabel]() mutable
					{
						if (const UArchonPlayerInputBridgeComponent* Bridge = WeakThis.Get())
						{
							UE_LOG(
								LogTemp,
								Display,
								TEXT("ArchonFactoryCanary: RtsCommandCardClicked key=%s label=%s enabled=%s surfaceOpen=%s commands=\"%s\" selection=\"%s\""),
								*SlotKey,
								*SlotLabel,
								bEnabled ? TEXT("true") : TEXT("false"),
								Bridge->bMapTableSurfaceOpen ? TEXT("true") : TEXT("false"),
								*Bridge->FormatRtsCommandLine(),
								*Bridge->FormatRtsSelectionLine());
							Bridge->NotifyPlaytestEvent(TEXT("rts_command_card"));
						}
						if (bEnabled && OnClicked)
						{
							return OnClicked();
						}
						return FReply::Handled();
					})
					[
						SNew(SBorder)
						.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
						.BorderBackgroundColor(FSlateColor(PanelColor))
						.Padding(FMargin(6.0f, 4.0f))
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot()
							.AutoHeight()
							[
								MakeText(FText::FromString(FString(Key)), 11, bEnabled ? Accent : SubText, FName(TEXT("Bold")))
							]
							+ SVerticalBox::Slot()
							.AutoHeight()
							.Padding(0.0f, 2.0f, 0.0f, 0.0f)
							[
								MakeText(FText::FromString(FString(Label)), 10, MainText, FName(TEXT("Bold")))
							]
							+ SVerticalBox::Slot()
							.AutoHeight()
							.Padding(0.0f, 2.0f, 0.0f, 0.0f)
							[
								MakeText(FText::FromString(FString(Detail)), 9, SubText, FName(TEXT("Regular")))
							]
						]
					]
				];
		};

		TSharedRef<SWidget> Overlay =
			SNew(SOverlay)
			+ SOverlay::Slot()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Fill)
			[
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
				.BorderBackgroundColor(FSlateColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.10f)))
			]
			+ SOverlay::Slot()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Top)
			.Padding(FMargin(28.0f, 22.0f, 28.0f, 0.0f))
			[
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
				.BorderBackgroundColor(FSlateColor(FLinearColor(0.025f, 0.045f, 0.034f, 0.78f)))
				.Padding(FMargin(18.0f, 10.0f))
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					.VAlign(VAlign_Center)
					[
						MakeText(NSLOCTEXT("ArchonMapTable", "OverlayTitle", "SPLITROOT COMMAND"), 24, FLinearColor(0.92f, 0.98f, 0.76f, 1.0f), FName(TEXT("Bold")))
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					.Padding(0.0f, 0.0f, 22.0f, 0.0f)
					[
						SNew(STextBlock)
						.Text_Lambda([WeakThis]()
						{
							const UArchonPlayerInputBridgeComponent* Bridge = WeakThis.Get();
							return FText::FromString(Bridge ? Bridge->FormatRtsMatchClockLine() : FString(TEXT("MATCH --")));
						})
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 15))
						.ColorAndOpacity(FSlateColor(FLinearColor(0.84f, 0.92f, 0.78f, 1.0f)))
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					.Padding(0.0f, 0.0f, 22.0f, 0.0f)
					[
						SNew(STextBlock)
						.Text_Lambda([WeakThis]()
						{
							const UArchonPlayerInputBridgeComponent* Bridge = WeakThis.Get();
							return FText::FromString(Bridge ? Bridge->FormatRtsSiteControlLine() : FString(TEXT("SITES --")));
						})
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 15))
						.ColorAndOpacity(FSlateColor(FLinearColor(0.84f, 0.92f, 0.78f, 1.0f)))
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text_Lambda([WeakThis]()
						{
							const UArchonPlayerInputBridgeComponent* Bridge = WeakThis.Get();
							return FText::FromString(Bridge ? Bridge->FormatRtsResourceLine() : FString(TEXT("SUPPLY --")));
						})
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 16))
						.ColorAndOpacity(FSlateColor(FLinearColor(0.92f, 0.98f, 0.88f, 1.0f)))
					]
				]
			]
			+ SOverlay::Slot()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Fill)
			.Padding(FMargin(44.0f, 82.0f, 44.0f, 312.0f))
			[
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
				.BorderBackgroundColor(FSlateColor(FLinearColor(0.035f, 0.075f, 0.052f, 0.22f)))
				.Padding(FMargin(18.0f, 14.0f))
				[
					SNew(SOverlay)
					+ SOverlay::Slot()
					.HAlign(HAlign_Left)
					.VAlign(VAlign_Top)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot()
						.AutoHeight()
						[
							MakeText(NSLOCTEXT("ArchonMapTable", "TacticalMapTitle", "TACTICAL MAP"), 20, FLinearColor(0.91f, 0.98f, 0.78f, 0.95f), FName(TEXT("Bold")))
						]
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding(0.0f, 7.0f, 0.0f, 0.0f)
						[
							SNew(STextBlock)
							.Text_Lambda([WeakThis]()
							{
								const UArchonPlayerInputBridgeComponent* Bridge = WeakThis.Get();
								return FText::FromString(Bridge ? Bridge->FormatRtsVisibilityLine() : FString(TEXT("VISION unavailable")));
							})
							.Font(FCoreStyle::GetDefaultFontStyle("Regular", 13))
							.ColorAndOpacity(FSlateColor(FLinearColor(0.76f, 0.88f, 0.68f, 0.92f)))
						]
					]
					+ SOverlay::Slot()
					.HAlign(HAlign_Right)
					.VAlign(VAlign_Bottom)
					[
						SNew(SBorder)
						.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
						.BorderBackgroundColor(FSlateColor(FLinearColor(0.018f, 0.028f, 0.022f, 0.58f)))
						.Padding(FMargin(12.0f, 8.0f))
						[
							SNew(STextBlock)
							.Text_Lambda([WeakThis]()
							{
								const UArchonPlayerInputBridgeComponent* Bridge = WeakThis.Get();
								return FText::FromString(Bridge ? Bridge->FormatRtsCommandLine() : FString(TEXT("ORDERS --")));
							})
							.Font(FCoreStyle::GetDefaultFontStyle("Bold", 14))
							.ColorAndOpacity(FSlateColor(FLinearColor(0.95f, 0.92f, 0.70f, 0.95f)))
						]
					]
				]
			]
			+ SOverlay::Slot()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Bottom)
			.Padding(FMargin(22.0f, 0.0f, 22.0f, 20.0f))
			[
				SNew(SBox)
				.HeightOverride(268.0f)
				[
					SNew(SBorder)
					.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
					.BorderBackgroundColor(FSlateColor(FLinearColor(0.018f, 0.026f, 0.020f, 0.86f)))
					.Padding(FMargin(14.0f, 12.0f))
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(0.0f, 0.0f, 14.0f, 0.0f)
						[
							SNew(SBox)
							.WidthOverride(188.0f)
							[
								SNew(SVerticalBox)
								+ SVerticalBox::Slot()
								.AutoHeight()
								[
									MakeText(NSLOCTEXT("ArchonMapTable", "MiniMapTitle", "MINIMAP"), 15, FLinearColor(0.92f, 0.98f, 0.78f, 1.0f), FName(TEXT("Bold")))
								]
								+ SVerticalBox::Slot()
								.AutoHeight()
								.Padding(0.0f, 7.0f, 0.0f, 7.0f)
								[
									MakeDivider()
								]
								+ SVerticalBox::Slot()
								.AutoHeight()
								[
									SNew(SGridPanel)
									+ SGridPanel::Slot(0, 0).Padding(2.0f)[MakeMapCell(FLinearColor(0.02f, 0.03f, 0.03f, 0.92f))]
									+ SGridPanel::Slot(1, 0).Padding(2.0f)[MakeMapCell(FLinearColor(0.09f, 0.10f, 0.08f, 0.92f))]
									+ SGridPanel::Slot(2, 0).Padding(2.0f)[MakeMapCell(FLinearColor(0.12f, 0.24f, 0.11f, 0.92f))]
									+ SGridPanel::Slot(3, 0).Padding(2.0f)[MakeMapCell(FLinearColor(0.07f, 0.08f, 0.07f, 0.92f))]
									+ SGridPanel::Slot(0, 1).Padding(2.0f)[MakeMapCell(FLinearColor(0.09f, 0.10f, 0.08f, 0.92f))]
									+ SGridPanel::Slot(1, 1).Padding(2.0f)[MakeMapCell(FLinearColor(0.13f, 0.25f, 0.13f, 0.92f))]
									+ SGridPanel::Slot(2, 1).Padding(2.0f)[MakeMapCell(FLinearColor(0.42f, 0.52f, 0.20f, 0.92f))]
									+ SGridPanel::Slot(3, 1).Padding(2.0f)[MakeMapCell(FLinearColor(0.16f, 0.30f, 0.14f, 0.92f))]
									+ SGridPanel::Slot(0, 2).Padding(2.0f)[MakeMapCell(FLinearColor(0.15f, 0.30f, 0.14f, 0.92f))]
									+ SGridPanel::Slot(1, 2).Padding(2.0f)[MakeMapCell(FLinearColor(0.44f, 0.56f, 0.22f, 0.92f))]
									+ SGridPanel::Slot(2, 2).Padding(2.0f)[MakeMapCell(FLinearColor(0.16f, 0.30f, 0.14f, 0.92f))]
									+ SGridPanel::Slot(3, 2).Padding(2.0f)[MakeMapCell(FLinearColor(0.35f, 0.16f, 0.12f, 0.92f))]
									+ SGridPanel::Slot(0, 3).Padding(2.0f)[MakeMapCell(FLinearColor(0.14f, 0.28f, 0.13f, 0.92f))]
									+ SGridPanel::Slot(1, 3).Padding(2.0f)[MakeMapCell(FLinearColor(0.14f, 0.14f, 0.12f, 0.92f))]
									+ SGridPanel::Slot(2, 3).Padding(2.0f)[MakeMapCell(FLinearColor(0.08f, 0.09f, 0.08f, 0.92f))]
									+ SGridPanel::Slot(3, 3).Padding(2.0f)[MakeMapCell(FLinearColor(0.35f, 0.16f, 0.12f, 0.92f))]
								]
								+ SVerticalBox::Slot()
								.AutoHeight()
								.Padding(0.0f, 8.0f, 0.0f, 0.0f)
								[
									SNew(STextBlock)
									.Text_Lambda([WeakThis]()
									{
										const UArchonPlayerInputBridgeComponent* Bridge = WeakThis.Get();
										return FText::FromString(Bridge ? Bridge->FormatRtsControlGroupLine() : FString(TEXT("1 CANARY")));
									})
									.Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
									.ColorAndOpacity(FSlateColor(FLinearColor(0.70f, 0.80f, 0.62f, 0.94f)))
								]
								+ SVerticalBox::Slot()
								.AutoHeight()
								.Padding(0.0f, 6.0f, 0.0f, 0.0f)
								[
									SNew(SGridPanel)
									+ SGridPanel::Slot(0, 0).Padding(2.0f)[MakeControlGroupPill(TEXT("1"), TEXT("Canary"), true)]
									+ SGridPanel::Slot(1, 0).Padding(2.0f)[MakeControlGroupPill(TEXT("2"), TEXT("Root"), false)]
									+ SGridPanel::Slot(0, 1).Padding(2.0f)[MakeControlGroupPill(TEXT("3"), TEXT("Build"), false)]
									+ SGridPanel::Slot(1, 1).Padding(2.0f)[MakeControlGroupPill(TEXT("F1"), TEXT("Hero"), false)]
								]
							]
						]
						+ SHorizontalBox::Slot()
						.FillWidth(1.0f)
						.Padding(0.0f, 0.0f, 14.0f, 0.0f)
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot()
							.AutoHeight()
							[
								MakeText(NSLOCTEXT("ArchonMapTable", "SelectionTitle", "SELECTED UNITS"), 15, FLinearColor(0.92f, 0.98f, 0.78f, 1.0f), FName(TEXT("Bold")))
							]
							+ SVerticalBox::Slot()
							.AutoHeight()
							.Padding(0.0f, 7.0f, 0.0f, 7.0f)
							[
								MakeDivider()
							]
							+ SVerticalBox::Slot()
							.AutoHeight()
							[
								SNew(STextBlock)
								.Text_Lambda([WeakThis]()
								{
									const UArchonPlayerInputBridgeComponent* Bridge = WeakThis.Get();
									return FText::FromString(Bridge ? Bridge->FormatRtsSelectionLine() : FString(TEXT("NO SELECTION")));
								})
								.Font(FCoreStyle::GetDefaultFontStyle("Bold", 14))
								.ColorAndOpacity(FSlateColor(FLinearColor(0.96f, 0.98f, 0.88f, 1.0f)))
							]
							+ SVerticalBox::Slot()
							.AutoHeight()
							.Padding(0.0f, 7.0f, 0.0f, 0.0f)
							[
								SNew(SHorizontalBox)
								+ SHorizontalBox::Slot()
								.AutoWidth()
								.Padding(0.0f, 0.0f, 10.0f, 0.0f)
								[
									SNew(SBox)
									.WidthOverride(118.0f)
									.HeightOverride(118.0f)
									[
										SNew(SBorder)
										.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
										.BorderBackgroundColor(FSlateColor(FLinearColor(0.045f, 0.11f, 0.058f, 0.82f)))
										.Padding(FMargin(8.0f, 6.0f))
										[
											SNew(SVerticalBox)
											+ SVerticalBox::Slot()
											.FillHeight(1.0f)
											[
												SNew(SBorder)
												.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
												.BorderBackgroundColor(FSlateColor(FLinearColor(0.18f, 0.32f, 0.16f, 0.78f)))
											]
											+ SVerticalBox::Slot()
											.AutoHeight()
											.Padding(0.0f, 5.0f, 0.0f, 0.0f)
											[
												MakeText(NSLOCTEXT("ArchonMapTable", "SelectedPortraitTitle", "THORN CAPTAIN"), 10, FLinearColor(0.92f, 0.98f, 0.78f, 1.0f), FName(TEXT("Bold")))
											]
											+ SVerticalBox::Slot()
											.AutoHeight()
											[
												MakeText(NSLOCTEXT("ArchonMapTable", "SelectedPortraitLevel", "LVL 3"), 9, FLinearColor(0.64f, 0.70f, 0.58f, 0.92f), FName(TEXT("Regular")))
											]
										]
									]
								]
								+ SHorizontalBox::Slot()
								.FillWidth(1.0f)
								[
									SNew(SVerticalBox)
									+ SVerticalBox::Slot()
									.AutoHeight()
									[
										SNew(SGridPanel)
										+ SGridPanel::Slot(0, 0).Padding(2.0f)[MakeStatChip(TEXT("HP"), [WeakThis](){ const UArchonPlayerInputBridgeComponent* Bridge = WeakThis.Get(); return FText::FromString(Bridge ? FString(TEXT("574/630")) : FString(TEXT("--"))); })]
										+ SGridPanel::Slot(1, 0).Padding(2.0f)[MakeStatChip(TEXT("MANA"), [](){ return FText::FromString(FString(TEXT("83/140"))); })]
										+ SGridPanel::Slot(2, 0).Padding(2.0f)[MakeStatChip(TEXT("DMG"), [](){ return FText::FromString(FString(TEXT("18-24"))); })]
										+ SGridPanel::Slot(3, 0).Padding(2.0f)[MakeStatChip(TEXT("ARMOR"), [](){ return FText::FromString(FString(TEXT("2"))); })]
										+ SGridPanel::Slot(0, 1).Padding(2.0f)[MakeStatChip(TEXT("RANGE"), [](){ return FText::FromString(FString(TEXT("MIXED"))); })]
										+ SGridPanel::Slot(1, 1).Padding(2.0f)[MakeStatChip(TEXT("UNITS"), [](){ return FText::FromString(FString(TEXT("4"))); })]
										+ SGridPanel::Slot(2, 1).Padding(2.0f)[MakeStatChip(TEXT("ORDERS"), [WeakThis](){ const UArchonPlayerInputBridgeComponent* Bridge = WeakThis.Get(); return FText::AsNumber(Bridge ? Bridge->SubmittedOrderCount : 0); })]
										+ SGridPanel::Slot(3, 1).Padding(2.0f)[MakeStatChip(TEXT("WIDGET"), [WeakThis](){ const UArchonPlayerInputBridgeComponent* Bridge = WeakThis.Get(); return FText::AsNumber(Bridge ? Bridge->RuntimeWidgetSubmittedOrderCount : 0); })]
									]
									+ SVerticalBox::Slot()
									.AutoHeight()
									.Padding(0.0f, 7.0f, 0.0f, 0.0f)
									[
										SNew(SHorizontalBox)
										+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 6.0f, 0.0f)[MakeUnitCard(TEXT("THORN 1"), TEXT("front / moving"), FLinearColor(0.82f, 0.96f, 0.54f, 1.0f))]
										+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 6.0f, 0.0f)[MakeUnitCard(TEXT("THORN 2"), TEXT("front / moving"), FLinearColor(0.82f, 0.96f, 0.54f, 1.0f))]
										+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 6.0f, 0.0f)[MakeUnitCard(TEXT("ROOT"), TEXT("anchor / guard"), FLinearColor(0.96f, 0.86f, 0.52f, 1.0f))]
										+ SHorizontalBox::Slot().AutoWidth()[MakeUnitCard(TEXT("WARD"), TEXT("support / cast"), FLinearColor(0.64f, 0.88f, 0.96f, 1.0f))]
									]
								]
							]
							+ SVerticalBox::Slot()
							.FillHeight(1.0f)
							.Padding(0.0f, 8.0f, 0.0f, 0.0f)
							[
								SNew(SBorder)
								.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
								.BorderBackgroundColor(FSlateColor(FLinearColor(0.035f, 0.052f, 0.042f, 0.70f)))
								.Padding(FMargin(10.0f, 8.0f))
								[
									SNew(SVerticalBox)
									+ SVerticalBox::Slot()
									.AutoHeight()
									[
										SNew(STextBlock)
										.Text_Lambda([WeakThis]()
										{
											const UArchonPlayerInputBridgeComponent* Bridge = WeakThis.Get();
											return FText::FromString(Bridge ? Bridge->FormatRtsLastActionLine() : FString(TEXT("LAST ACTION --")));
										})
										.Font(FCoreStyle::GetDefaultFontStyle("Regular", 13))
										.ColorAndOpacity(FSlateColor(FLinearColor(0.83f, 0.90f, 0.76f, 0.95f)))
									]
									+ SVerticalBox::Slot()
									.AutoHeight()
									.Padding(0.0f, 5.0f, 0.0f, 0.0f)
									[
										SNew(STextBlock)
										.Text_Lambda([WeakThis]()
										{
											const UArchonPlayerInputBridgeComponent* Bridge = WeakThis.Get();
											return FText::FromString(Bridge ? Bridge->FormatRtsProductionLine() : FString(TEXT("PRODUCTION --")));
										})
										.Font(FCoreStyle::GetDefaultFontStyle("Regular", 13))
										.ColorAndOpacity(FSlateColor(FLinearColor(0.90f, 0.84f, 0.62f, 0.95f)))
									]
								]
							]
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							SNew(SBox)
							.WidthOverride(392.0f)
							[
								SNew(SVerticalBox)
								+ SVerticalBox::Slot()
								.AutoHeight()
								[
									MakeText(NSLOCTEXT("ArchonMapTable", "CommandCardTitle", "COMMAND CARD"), 15, FLinearColor(0.92f, 0.98f, 0.78f, 1.0f), FName(TEXT("Bold")))
								]
								+ SVerticalBox::Slot()
								.AutoHeight()
								.Padding(0.0f, 7.0f, 0.0f, 7.0f)
								[
									MakeDivider()
								]
								+ SVerticalBox::Slot()
								.AutoHeight()
								[
									SNew(SGridPanel)
									+ SGridPanel::Slot(0, 0).Padding(3.0f)[MakeCommandButton(TEXT("E"), TEXT("MOVE"), TEXT("ground"), FLinearColor(0.82f, 0.96f, 0.54f, 1.0f), true, [WeakThis]()
									{
										if (UArchonPlayerInputBridgeComponent* Bridge = WeakThis.Get())
										{
											Bridge->SubmitRuntimeRtsOrder(EArchonRtsOrderKind::MoveSquad);
										}
										return FReply::Handled();
									})]
									+ SGridPanel::Slot(1, 0).Padding(3.0f)[MakeCommandButton(TEXT("LMB"), TEXT("ATTACK"), TEXT("target"), FLinearColor(0.96f, 0.58f, 0.48f, 1.0f), true, [WeakThis]()
									{
										if (UArchonPlayerInputBridgeComponent* Bridge = WeakThis.Get())
										{
											Bridge->SubmitRuntimeRtsOrder(EArchonRtsOrderKind::AttackTarget);
										}
										return FReply::Handled();
									})]
									+ SGridPanel::Slot(2, 0).Padding(3.0f)[MakeCommandButton(TEXT("R"), TEXT("RALLY"), TEXT("point"), FLinearColor(0.70f, 0.82f, 0.98f, 1.0f), true, [WeakThis]()
									{
										if (UArchonPlayerInputBridgeComponent* Bridge = WeakThis.Get())
										{
											Bridge->SubmitRuntimeMapTableWidgetRallyPointAt(FVector2D(0.70f, 0.50f), TEXT("command_card_rally"));
										}
										return FReply::Handled();
									})]
									+ SGridPanel::Slot(3, 0).Padding(3.0f)[MakeCommandButton(TEXT("TAB"), TEXT("LEAVE"), TEXT("return FPS"), FLinearColor(0.96f, 0.88f, 0.58f, 1.0f), true, [WeakThis]()
									{
										if (UArchonPlayerInputBridgeComponent* Bridge = WeakThis.Get())
										{
											Bridge->CloseRuntimeMapTable();
										}
										return FReply::Handled();
									})]
									+ SGridPanel::Slot(0, 1).Padding(3.0f)[MakeCommandButton(TEXT("B"), TEXT("TRAIN"), TEXT("squad"), FLinearColor(0.95f, 0.80f, 0.45f, 1.0f), true, [WeakThis]()
									{
										if (UArchonPlayerInputBridgeComponent* Bridge = WeakThis.Get())
										{
											Bridge->RequestReinforcementPurchase();
										}
										return FReply::Handled();
									})]
									+ SGridPanel::Slot(1, 1).Padding(3.0f)[MakeCommandButton(TEXT("T"), TEXT("BUILD"), TEXT("armory"), FLinearColor(0.74f, 0.92f, 0.66f, 1.0f), true, [WeakThis]()
									{
										if (UArchonPlayerInputBridgeComponent* Bridge = WeakThis.Get())
										{
											Bridge->RequestTechBuildingPurchase();
										}
										return FReply::Handled();
									})]
									+ SGridPanel::Slot(2, 1).Padding(3.0f)[MakeCommandButton(TEXT("C"), TEXT("TECH"), TEXT("cycle"), FLinearColor(0.74f, 0.92f, 0.66f, 1.0f), true, [WeakThis]()
									{
										if (UArchonPlayerInputBridgeComponent* Bridge = WeakThis.Get())
										{
											Bridge->CycleSelectedTechBuilding(1);
										}
										return FReply::Handled();
									})]
									+ SGridPanel::Slot(3, 1).Padding(3.0f)[MakeCommandButton(TEXT("F"), TEXT("TAKE"), TEXT("before table"), FLinearColor(0.52f, 0.84f, 0.94f, 1.0f), false, [](){ return FReply::Handled(); })]
									+ SGridPanel::Slot(0, 2).Padding(3.0f)[MakeCommandButton(TEXT("S"), TEXT("STOP"), TEXT("next"), FLinearColor(0.60f, 0.64f, 0.58f, 1.0f), false, [](){ return FReply::Handled(); })]
									+ SGridPanel::Slot(1, 2).Padding(3.0f)[MakeCommandButton(TEXT("H"), TEXT("HOLD"), TEXT("next"), FLinearColor(0.60f, 0.64f, 0.58f, 1.0f), false, [](){ return FReply::Handled(); })]
									+ SGridPanel::Slot(2, 2).Padding(3.0f)[MakeCommandButton(TEXT("P"), TEXT("PATROL"), TEXT("next"), FLinearColor(0.60f, 0.64f, 0.58f, 1.0f), false, [](){ return FReply::Handled(); })]
									+ SGridPanel::Slot(3, 2).Padding(3.0f)[MakeCommandButton(TEXT("X"), TEXT("PING"), TEXT("next"), FLinearColor(0.60f, 0.64f, 0.58f, 1.0f), false, [](){ return FReply::Handled(); })]
								]
							]
						]
					]
				]
			];

		MapTableOverlayWidget = Overlay;
		GEngine->GameViewport->AddViewportWidgetContent(Overlay, /*ZOrder=*/90);
		UE_LOG(LogTemp, Display, TEXT("ArchonFactoryCanary: MapTableOverlayShown visible=true mode=rts_command_cockpit_wc3_reference slots=12 minimap=4x4 controlGroups=true worldAlpha=0.22 hudAlpha=0.86"));
		return;
	}
}

#if 0
	// The command surface must be unmistakable in screenshots and playtests.
	// A small side panel logged as open but did not read as RTS control.
	TWeakObjectPtr<UArchonPlayerInputBridgeComponent> WeakThis(this);
	TSharedRef<SWidget> Overlay =
		SNew(SOverlay)
		+ SOverlay::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		[
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
			.BorderBackgroundColor(FSlateColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.46f)))
		]
		+ SOverlay::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		.Padding(28.0f)
		[
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
			.BorderBackgroundColor(FSlateColor(FLinearColor(0.015f, 0.035f, 0.022f, 0.94f)))
			.Padding(FMargin(42.0f, 32.0f))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 0.0f, 0.0f, 8.0f)
				[
					SNew(STextBlock)
					.Text(NSLOCTEXT("ArchonMapTable", "OverlayTitle", "MAP TABLE — RTS COMMAND"))
					.Text(NSLOCTEXT("ArchonMapTable", "OverlayTitleReadable", "RTS COMMAND MODE"))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 34))
					.ColorAndOpacity(FSlateColor(FLinearColor(0.85f, 0.95f, 0.70f)))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 0.0f, 0.0f, 10.0f)
				[
					SNew(STextBlock)
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 18))
					.Text_Lambda([WeakThis]()
					{
						const UArchonPlayerInputBridgeComponent* Bridge = WeakThis.Get();
						const int32 Sequence = (Bridge && Bridge->MapTable) ? Bridge->MapTable->GetCurrentCommandSequence() : 0;
						return FText::FromString(FString::Printf(
							TEXT("Orders submitted: %d    Sequence: %d"),
							Bridge ? Bridge->SubmittedOrderCount : 0,
							Sequence));
					})
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 0.0f, 0.0f, 18.0f)
				[
					SNew(STextBlock)
					.Text(NSLOCTEXT("ArchonMapTable", "OverlayMapState", "TACTICAL MAP ACTIVE    SELECTED: CANARY SQUAD    CURSOR: ACTIVE"))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 20))
					.ColorAndOpacity(FSlateColor(FLinearColor(0.94f, 0.98f, 0.84f)))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 0.0f, 0.0f, 18.0f)
				[
					SNew(STextBlock)
					.Text_Lambda([WeakThis]()
					{
						const UArchonPlayerInputBridgeComponent* Bridge = WeakThis.Get();
						const FString Summary = (Bridge && Bridge->RuntimeMapTableWidget)
							? Bridge->RuntimeMapTableWidget->FormatVisibilitySummary()
							: TEXT("VISION unavailable");
						return FText::FromString(Summary);
					})
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 16))
					.ColorAndOpacity(FSlateColor(FLinearColor(0.78f, 0.88f, 0.68f)))
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock)
					.Text(NSLOCTEXT("ArchonMapTable", "OverlayMove", "E      MOVE ORDER"))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 20))
				]
				+ SVerticalBox::Slot().AutoHeight()
				.Padding(0.0f, 8.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock)
					.Text(NSLOCTEXT("ArchonMapTable", "OverlayAttack", "LMB    ATTACK ORDER"))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 20))
				]
				+ SVerticalBox::Slot().AutoHeight()
				.Padding(0.0f, 8.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock)
					.Text(NSLOCTEXT("ArchonMapTable", "OverlayReinforce", "B      REINFORCE"))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 20))
				]
				+ SVerticalBox::Slot().AutoHeight()
				.Padding(0.0f, 8.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock)
					.Text(NSLOCTEXT("ArchonMapTable", "OverlayRally", "R      SET RALLY"))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 20))
				]
				+ SVerticalBox::Slot().AutoHeight()
				.Padding(0.0f, 8.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock)
					.Text(NSLOCTEXT("ArchonMapTable", "OverlayBuild", "T      BUILD ARMORY"))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 20))
				]
				+ SVerticalBox::Slot().AutoHeight()
				.Padding(0.0f, 18.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock)
					.Text(NSLOCTEXT("ArchonMapTable", "OverlayClose", "TAB    CLOSE TABLE"))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 20))
					.ColorAndOpacity(FSlateColor(FLinearColor(0.95f, 0.82f, 0.55f)))
				]
			]
		];
	MapTableOverlayWidget = Overlay;
	GEngine->GameViewport->AddViewportWidgetContent(Overlay, /*ZOrder=*/90);
	UE_LOG(LogTemp, Display, TEXT("ArchonFactoryCanary: MapTableOverlayShown visible=true mode=centered_rts_command"));
}
#endif

void UArchonPlayerInputBridgeComponent::HideMapTableOverlay()
{
	if (MapTableOverlayWidget.IsValid() && GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(MapTableOverlayWidget.ToSharedRef());
		UE_LOG(LogTemp, Display, TEXT("ArchonFactoryCanary: MapTableOverlayHidden"));
	}
	MapTableOverlayWidget.Reset();
}

void UArchonPlayerInputBridgeComponent::ShowInteractPrompt()
{
	bInteractPromptVisible = true;

	if (GEngine && GEngine->GameViewport && !InteractPromptWidget.IsValid())
	{
		TSharedRef<SWidget> Prompt =
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			[
				SNew(SSpacer)
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Center)
			.Padding(0.0f, 0.0f, 0.0f, 140.0f)
			[
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
				.BorderBackgroundColor(FSlateColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.55f)))
				.Padding(FMargin(18.0f, 8.0f))
				[
					SNew(STextBlock)
					.Text(NSLOCTEXT("ArchonInteract", "MapTablePrompt", "[E]  MAP TABLE"))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 20))
					.ColorAndOpacity(FSlateColor(FLinearColor(0.85f, 0.95f, 0.70f)))
				]
			];
		InteractPromptWidget = Prompt;
		GEngine->GameViewport->AddViewportWidgetContent(Prompt, /*ZOrder=*/50);
	}

	UE_LOG(LogTemp, Display, TEXT("ArchonFactoryCanary: InteractPromptShown target=map_table"));
	NotifyPlaytestEvent(TEXT("interact_prompt"));
}

void UArchonPlayerInputBridgeComponent::HideInteractPrompt()
{
	bInteractPromptVisible = false;

	if (InteractPromptWidget.IsValid() && GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(InteractPromptWidget.ToSharedRef());
	}
	InteractPromptWidget.Reset();

	UE_LOG(LogTemp, Display, TEXT("ArchonFactoryCanary: InteractPromptHidden"));
}

bool UArchonPlayerInputBridgeComponent::PreviewRuntimeMapTable()
{
	if (!IsDeadStateCommandAllowed())
	{
		LastInteractionResult = FArchonMapTableInteractionResult();
		LastInteractionResult.Reason = TEXT("life_state_blocks_map_table");
		bMapTableSurfaceOpen = false;
		CloseRuntimeMapTableWidget();
		if (MapTable)
		{
			MapTable->SetRuntimeStatusText(TEXT("RTS SURFACE BLOCKED\nReason: life_state_blocks_map_table"));
		}
		UE_LOG(
			LogTemp,
			Display,
			TEXT("ArchonFactoryCanary: RuntimeMapTablePreview opened=false widget=false reason=life_state_blocks_map_table"));
		return false;
	}

	UArchonMapTableInteractorComponent* ActiveInteractor = ResolveInteractor();
	if (!ActiveInteractor || !MapTable)
	{
		LastInteractionResult = FArchonMapTableInteractionResult();
		LastInteractionResult.Reason = TEXT("runtime_bridge_not_ready");
		return false;
	}

	const bool bPreviewOpened = ActiveInteractor->PreviewMapTable(MapTable, LastInteractionResult);
	const FString StatusText = bPreviewOpened
		? FString::Printf(TEXT("RTS SURFACE OPEN\nCOMMAND COCKPIT READY\nTeam %d\nE move  LMB attack  R rally  B train  T build  Tab leave"), InteractorConfig.TeamId)
		: FString::Printf(TEXT("RTS SURFACE BLOCKED\nReason: %s"), *LastInteractionResult.Reason.ToString());
	MapTable->SetRuntimeStatusText(StatusText);

	const bool bWidgetOpened = bPreviewOpened ? OpenRuntimeMapTableWidget() : false;
	const bool bOpened = bPreviewOpened && bWidgetOpened;
	if (bPreviewOpened && !bWidgetOpened)
	{
		LastInteractionResult.Reason = TEXT("runtime_widget_not_ready");
		MapTable->SetRuntimeStatusText(TEXT("RTS SURFACE BLOCKED\nReason: runtime_widget_not_ready"));
	}
	bMapTableSurfaceOpen = bOpened;

	UE_LOG(
		LogTemp,
		Display,
		TEXT("ArchonFactoryCanary: RuntimeMapTablePreview opened=%s widget=%s reason=%s"),
		bOpened ? TEXT("true") : TEXT("false"),
		bWidgetOpened ? TEXT("true") : TEXT("false"),
		*LastInteractionResult.Reason.ToString());

	if (bOpened)
	{
		// Body locked, mouse up: the table is an RTS surface, not a menu
		// you walk around with.
		if (APlayerController* ActiveController = ResolvePlayerController())
		{
			FInputModeGameAndUI InputMode;
			InputMode.SetHideCursorDuringCapture(false);
			InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			ActiveController->SetInputMode(InputMode);
			ActiveController->SetShowMouseCursor(true);
		}
		UE_LOG(LogTemp, Display, TEXT("ArchonFactoryCanary: MapTableBodyLock locked=true cursor=true"));
		ShowMapTableOverlay();
		RequestMapTableVisualEvidence(TEXT("map_table_open"));
		NotifyPlaytestEvent(TEXT("map_table_open"));
	}

	return bOpened;
}

bool UArchonPlayerInputBridgeComponent::SubmitRuntimeRtsOrder(EArchonRtsOrderKind OrderKind)
{
	const EArchonLifeState LifeStateBeforeSubmit = RespawnState ? RespawnState->GetLifeState() : EArchonLifeState::Alive;
	if (!IsDeadStateCommandAllowed())
	{
		LastInteractionResult = FArchonMapTableInteractionResult();
		LastInteractionResult.Reason = TEXT("life_state_blocks_map_table");
		UE_LOG(
			LogTemp,
			Display,
			TEXT("ArchonFactoryCanary: RuntimeMapTableOrder submitted=false order=%d sequence=%d reason=life_state_blocks_map_table"),
			static_cast<int32>(OrderKind),
			MapTable ? MapTable->GetCurrentCommandSequence() : 0);
		return false;
	}

	// Network client: RTS orders are server-authoritative. Route the order
	// through the connection-owned pawn's RPC; the server rebuilds the
	// intent against its own map table (shared Archon command surface).
	// The local map table stays a preview surface only.
	UWorld* World = GetWorld();
	if (World && World->GetNetMode() == NM_Client)
	{
		APlayerController* OwningController = PlayerController ? PlayerController.Get() : ResolvePlayerController();
		AArchonCanaryFpsCharacter* CanaryCharacter = OwningController
			? Cast<AArchonCanaryFpsCharacter>(OwningController->GetPawn())
			: nullptr;
		if (!CanaryCharacter)
		{
			UE_LOG(LogTemp, Warning, TEXT("ArchonFactoryCanary: RuntimeMapTableOrder routed=false reason=no_owned_pawn_on_client"));
			return false;
		}

		CanaryCharacter->ServerSubmitRtsOrder(OrderKind);
		UE_LOG(
			LogTemp,
			Display,
			TEXT("ArchonFactoryCanary: RuntimeMapTableOrder routed=server_rpc order=%d"),
			static_cast<int32>(OrderKind));
		NotifyPlaytestEvent(TEXT("rts_order"));
		RequestMapTableVisualEvidence(TEXT("rts_order"));
		return true;
	}

	UArchonMapTableInteractorComponent* ActiveInteractor = ResolveInteractor();
	if (!ActiveInteractor || !MapTable)
	{
		LastInteractionResult = FArchonMapTableInteractionResult();
		LastInteractionResult.Reason = TEXT("runtime_bridge_not_ready");
		return false;
	}

	const bool bSubmitted = ActiveInteractor->SubmitRtsOrder(MapTable, MakeRuntimeIntent(OrderKind), LastInteractionResult);
	if (bSubmitted)
	{
		++SubmittedOrderCount;
		NotifyPlaytestEvent(TEXT("rts_order"));
		RequestMapTableVisualEvidence(TEXT("rts_order"));
	}
	RecordCommandSubmittedDuringDeath(bSubmitted, LifeStateBeforeSubmit);

	const int32 Sequence = MapTable->GetCurrentCommandSequence();
	const FString StatusText = bSubmitted
		? FString::Printf(TEXT("ORDER ACCEPTED\nSequence %d\nArchon shared RTS state updated"), Sequence)
		: FString::Printf(TEXT("ORDER PREVIEW ONLY\nSequence %d\nReason: %s"), Sequence, *LastInteractionResult.Reason.ToString());
	MapTable->SetRuntimeStatusText(StatusText);

	UE_LOG(
		LogTemp,
		Display,
		TEXT("ArchonFactoryCanary: RuntimeMapTableOrder submitted=%s order=%d sequence=%d reason=%s"),
		bSubmitted ? TEXT("true") : TEXT("false"),
		static_cast<int32>(OrderKind),
		Sequence,
		*LastInteractionResult.Reason.ToString());

	return bSubmitted;
}

void UArchonPlayerInputBridgeComponent::SetRespawnStateComponent(UArchonRespawnStateComponent* InRespawnState)
{
	RespawnState = InRespawnState;
}

bool UArchonPlayerInputBridgeComponent::IsDeadStateCommandAllowed() const
{
	if (!RespawnState)
	{
		return true;
	}

	if (!bAllowMapTableDuringDeath)
	{
		return RespawnState->GetLifeState() == EArchonLifeState::Alive;
	}

	return RespawnState->MayIssueMapTableCommand();
}

bool UArchonPlayerInputBridgeComponent::CloseRuntimeMapTable()
{
	const bool bWasOpen = bMapTableSurfaceOpen;
	bMapTableSurfaceOpen = false;
	CloseRuntimeMapTableWidget();

	if (MapTable)
	{
		MapTable->SetRuntimeStatusText(TEXT("RTS SURFACE CLOSED\nFPS CONTROL ACTIVE\nTab: open map table"));
	}

	UE_LOG(
		LogTemp,
		Display,
		TEXT("ArchonFactoryCanary: RuntimeMapTableClosed wasOpen=%s fpsControlActive=true"),
		bWasOpen ? TEXT("true") : TEXT("false"));

	HideMapTableOverlay();
	if (bWasOpen)
	{
		// Release the body: FPS control resumes (unless the pause menu is
		// about to take the surface — it sets its own input mode after).
		if (APlayerController* ActiveController = ResolvePlayerController())
		{
			ActiveController->SetInputMode(FInputModeGameOnly());
			ActiveController->SetShowMouseCursor(false);
		}
		UE_LOG(LogTemp, Display, TEXT("ArchonFactoryCanary: MapTableBodyLock locked=false cursor=false"));
		NotifyPlaytestEvent(TEXT("map_table_closed"));
	}

	return bWasOpen;
}

bool UArchonPlayerInputBridgeComponent::RunRuntimeRtsProofSequence()
{
	const bool bOpened = PreviewRuntimeMapTable();
	const bool bSubmittedMove = SubmitRuntimeRtsOrder(EArchonRtsOrderKind::MoveSquad);
	const bool bSubmittedAttack = SubmitRuntimeRtsOrder(EArchonRtsOrderKind::AttackTarget);
	const bool bClosed = CloseRuntimeMapTable();

	bRuntimeRtsProofSequenceRan = bOpened && bSubmittedMove && bSubmittedAttack && bClosed;

	const int32 Sequence = MapTable ? MapTable->GetCurrentCommandSequence() : 0;
	UE_LOG(
		LogTemp,
		Display,
		TEXT("ArchonFactoryCanary: RuntimeRtsProofSequence completed=%s opened=%s move=%s attack=%s closed=%s sequence=%d"),
		bRuntimeRtsProofSequenceRan ? TEXT("true") : TEXT("false"),
		bOpened ? TEXT("true") : TEXT("false"),
		bSubmittedMove ? TEXT("true") : TEXT("false"),
		bSubmittedAttack ? TEXT("true") : TEXT("false"),
		bClosed ? TEXT("true") : TEXT("false"),
		Sequence);

	return bRuntimeRtsProofSequenceRan;
}

bool UArchonPlayerInputBridgeComponent::RunRuntimeMapTableWidgetProofSequence()
{
	const bool bOpened = PreviewRuntimeMapTable();
	const bool bSelected = RuntimeMapTableWidget
		&& RuntimeMapTableWidget->GetSelectedSquadIds().Contains(TEXT("canary_squad"));
	const bool bSubmitted = SubmitRuntimeMapTableWidgetMoveOrder();
	const bool bClosed = CloseRuntimeMapTable();

	bRuntimeMapTableWidgetProofSequenceRan = bOpened && bSelected && bSubmitted && bClosed;

	const int32 Sequence = MapTable ? MapTable->GetCurrentCommandSequence() : 0;
	UE_LOG(
		LogTemp,
		Display,
		TEXT("ArchonFactoryCanary: RuntimeMapTableWidgetProofSequence completed=%s opened=%s selected=%s order=%s closed=%s sequence=%d"),
		bRuntimeMapTableWidgetProofSequenceRan ? TEXT("true") : TEXT("false"),
		bOpened ? TEXT("true") : TEXT("false"),
		bSelected ? TEXT("true") : TEXT("false"),
		bSubmitted ? TEXT("true") : TEXT("false"),
		bClosed ? TEXT("true") : TEXT("false"),
		Sequence);

	return bRuntimeMapTableWidgetProofSequenceRan;
}

bool UArchonPlayerInputBridgeComponent::ApplyRuntimeFactionMovementInput(
	ACharacter* Character,
	bool bSprintHeld,
	bool bJumpPressed,
	bool bJumpReleased)
{
	if (!Character)
	{
		return false;
	}

	UArchonFactionMovementComponent* FactionMovement = Character->FindComponentByClass<UArchonFactionMovementComponent>();
	if (FactionMovement)
	{
		FactionMovement->NotifySprintHeld(bSprintHeld);
		if (bJumpPressed)
		{
			FactionMovement->NotifyJumpPressed();
		}
	}

	const bool bSuppressStandardJump =
		bJumpPressed &&
		FactionMovement &&
		FactionMovement->WillVaultThisFrame();

	if (bJumpPressed && !bSuppressStandardJump)
	{
		Character->Jump();
	}

	if (bJumpReleased)
	{
		Character->StopJumping();
	}

	if (bJumpPressed && FactionMovement)
	{
		UE_LOG(
			LogTemp,
			Display,
			TEXT("ArchonFactoryCanary: RuntimeRootVaultInput routed=true sprintHeld=%s jumpPressed=true vaultPending=%s standardJumpSuppressed=%s"),
			bSprintHeld ? TEXT("true") : TEXT("false"),
			FactionMovement->WillVaultThisFrame() ? TEXT("true") : TEXT("false"),
			bSuppressStandardJump ? TEXT("true") : TEXT("false"));
	}

	return bSuppressStandardJump;
}

bool UArchonPlayerInputBridgeComponent::ApplyRuntimeWeaponInput(
	ACharacter* Character,
	bool bFirePressed,
	bool bReloadPressed)
{
	if (!Character || (!bFirePressed && !bReloadPressed) || bMapTableSurfaceOpen)
	{
		return false;
	}

	UArchonVerdantThornsproutBow* Bow = Character->FindComponentByClass<UArchonVerdantThornsproutBow>();
	if (!Bow)
	{
		UE_LOG(LogTemp, Warning, TEXT("ArchonFactoryCanary: RuntimeWeaponInput routed=false reason=missing_verdant_bow"));
		return false;
	}

	AArchonCanaryFpsCharacter* CanaryCharacter = Cast<AArchonCanaryFpsCharacter>(Character);
	FVector Origin = Character->GetActorLocation();
	FVector Direction = Character->GetActorForwardVector();
	if (APlayerController* AimController = ResolvePlayerController())
	{
		// Fire along the ACTIVE viewpoint — first person, third person,
		// whatever the player is actually aiming with (Jonathan
		// 2026-06-10: "things don't shoot the direction I expect").
		FRotator ViewRotation;
		AimController->GetPlayerViewPoint(Origin, ViewRotation);
		Direction = ViewRotation.Vector();
	}
	else if (CanaryCharacter)
	{
		if (const UCameraComponent* Camera = CanaryCharacter->GetFirstPersonCamera())
		{
			Origin = Camera->GetComponentLocation();
			Direction = Camera->GetForwardVector();
		}
	}

	// Network client: the weapon is server-authoritative (TryFire on a
	// proxy rejects with not_authority), so route the trigger pull through
	// the connection-owned pawn's server RPC instead.
	if (!Character->HasAuthority())
	{
		if (!CanaryCharacter)
		{
			UE_LOG(LogTemp, Warning, TEXT("ArchonFactoryCanary: RuntimeWeaponInput routed=false reason=non_canary_character_on_client"));
			return false;
		}

		if (bFirePressed)
		{
			CanaryCharacter->ServerFireBow(Origin, Direction);
			NotifyPlaytestEvent(TEXT("weapon_fire"));
		}
		if (bReloadPressed)
		{
			CanaryCharacter->ServerReloadBow();
		}

		UE_LOG(
			LogTemp,
			Display,
			TEXT("ArchonFactoryCanary: RuntimeWeaponInput routed=server_rpc firePressed=%s reloadPressed=%s ammo=%d"),
			bFirePressed ? TEXT("true") : TEXT("false"),
			bReloadPressed ? TEXT("true") : TEXT("false"),
			Bow->GetState().CurrentAmmo);
		return true;
	}

	const bool bFired = bFirePressed ? Bow->TryFire(Origin, Direction) : false;
	const bool bReloaded = bReloadPressed ? Bow->TryReload() : false;
	if (bFired)
	{
		NotifyPlaytestEvent(TEXT("weapon_fire"));
	}
	const FArchonWeaponState WeaponState = Bow->GetState();

	UE_LOG(
		LogTemp,
		Display,
		TEXT("ArchonFactoryCanary: RuntimeWeaponInput routed=true firePressed=%s fired=%s reloadPressed=%s reloaded=%s ammo=%d shots=%d reloads=%d state=%d"),
		bFirePressed ? TEXT("true") : TEXT("false"),
		bFired ? TEXT("true") : TEXT("false"),
		bReloadPressed ? TEXT("true") : TEXT("false"),
		bReloaded ? TEXT("true") : TEXT("false"),
		WeaponState.CurrentAmmo,
		Bow->GetShotsFired(),
		Bow->GetReloadCount(),
		static_cast<int32>(WeaponState.FireState));

	return bFired || bReloaded;
}

bool UArchonPlayerInputBridgeComponent::RunRuntimeWeaponProofSequence(ACharacter* Character)
{
	if (!Character)
	{
		return false;
	}

	if (bMapTableSurfaceOpen)
	{
		CloseRuntimeMapTable();
	}

	UArchonVerdantThornsproutBow* Bow = Character->FindComponentByClass<UArchonVerdantThornsproutBow>();
	if (!Bow)
	{
		UE_LOG(LogTemp, Error, TEXT("ArchonFactoryCanary: RuntimeWeaponProofSequence completed=false reason=missing_verdant_bow"));
		return false;
	}

	const int32 InitialShotsFired = Bow->GetShotsFired();
	const int32 InitialReloadCount = Bow->GetReloadCount();
	const bool bFired = ApplyRuntimeWeaponInput(Character, true, false);
	Bow->TickComponent(Bow->GetStats().FireCycleSeconds + 0.05f, LEVELTICK_All, nullptr);

	const bool bReloadStarted = ApplyRuntimeWeaponInput(Character, false, true);
	Bow->TickComponent(Bow->GetStats().ReloadSeconds + 0.05f, LEVELTICK_All, nullptr);

	const FArchonWeaponState FinalState = Bow->GetState();
	const bool bReloadCompleted = Bow->GetReloadCount() > InitialReloadCount;
	bRuntimeWeaponProofSequenceRan =
		bFired &&
		bReloadStarted &&
		bReloadCompleted &&
		Bow->GetShotsFired() == InitialShotsFired + 1 &&
		FinalState.FireState == EArchonWeaponFireState::Ready &&
		FinalState.CurrentAmmo == Bow->GetStats().QuiverCapacity &&
		!bMapTableSurfaceOpen;

	const TCHAR* CompletedText = bRuntimeWeaponProofSequenceRan ? TEXT("true") : TEXT("false");
	const TCHAR* FiredText = bFired ? TEXT("true") : TEXT("false");
	const TCHAR* ReloadStartedText = bReloadStarted ? TEXT("true") : TEXT("false");
	const TCHAR* ReloadCompletedText = bReloadCompleted ? TEXT("true") : TEXT("false");
	const TCHAR* MapTableOpenText = bMapTableSurfaceOpen ? TEXT("true") : TEXT("false");
	if (bRuntimeWeaponProofSequenceRan)
	{
		UE_LOG(
			LogTemp,
			Display,
			TEXT("ArchonFactoryCanary: RuntimeWeaponProofSequence completed=%s fired=%s reloadStarted=%s reloadCompleted=%s shots=%d reloads=%d ammo=%d mapTableOpen=%s"),
			CompletedText,
			FiredText,
			ReloadStartedText,
			ReloadCompletedText,
			Bow->GetShotsFired(),
			Bow->GetReloadCount(),
			FinalState.CurrentAmmo,
			MapTableOpenText);
	}
	else
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("ArchonFactoryCanary: RuntimeWeaponProofSequence completed=%s fired=%s reloadStarted=%s reloadCompleted=%s shots=%d reloads=%d ammo=%d mapTableOpen=%s"),
			CompletedText,
			FiredText,
			ReloadStartedText,
			ReloadCompletedText,
			Bow->GetShotsFired(),
			Bow->GetReloadCount(),
			FinalState.CurrentAmmo,
			MapTableOpenText);
	}

	return bRuntimeWeaponProofSequenceRan;
}

bool UArchonPlayerInputBridgeComponent::TogglePauseMenu()
{
	return bPauseMenuOpen ? ClosePauseMenu() : OpenPauseMenu();
}

bool UArchonPlayerInputBridgeComponent::OpenPauseMenu()
{
	APlayerController* Controller = ResolvePlayerController();
	if (bPauseMenuOpen || !Controller || !GEngine || !GEngine->GameViewport)
	{
		return false;
	}

	if (bMapTableSurfaceOpen)
	{
		CloseRuntimeMapTable();
	}

	TSharedRef<SArchonPauseMenuPanel> Panel = SNew(SArchonPauseMenuPanel).InputBridge(this);
	PauseMenuWidget = Panel;
	GEngine->GameViewport->AddViewportWidgetContent(Panel, /*ZOrder=*/200);

	// GameAndUI (not UIOnly) so the Escape poll in TickComponent still sees
	// the close press; the fullscreen dim border catches stray clicks so
	// the viewport can never recapture the mouse.
	FInputModeGameAndUI InputMode;
	InputMode.SetHideCursorDuringCapture(false);
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	Controller->SetInputMode(InputMode);
	Controller->SetShowMouseCursor(true);

	bPauseMenuOpen = true;
	UE_LOG(LogTemp, Display, TEXT("ArchonFactoryCanary: PauseMenuOpened lookScale=%.3f"), MouseLookScale);
	NotifyPlaytestEvent(TEXT("pause_open"));
	return true;
}

bool UArchonPlayerInputBridgeComponent::ClosePauseMenu()
{
	if (!bPauseMenuOpen)
	{
		return false;
	}

	if (PauseMenuWidget.IsValid() && GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(PauseMenuWidget.ToSharedRef());
	}
	PauseMenuWidget.Reset();

	if (APlayerController* Controller = ResolvePlayerController())
	{
		Controller->SetInputMode(FInputModeGameOnly());
		Controller->SetShowMouseCursor(false);
	}

	bPauseMenuOpen = false;
	UE_LOG(LogTemp, Display, TEXT("ArchonFactoryCanary: PauseMenuClosed lookScale=%.3f"), MouseLookScale);
	NotifyPlaytestEvent(TEXT("pause_close"));
	return true;
}

void UArchonPlayerInputBridgeComponent::SetMouseLookScale(float NewScale)
{
	MouseLookScale = FMath::Clamp(NewScale, MinMouseLookScale, MaxMouseLookScale);
	if (GConfig)
	{
		GConfig->SetFloat(TEXT("ArchonControls"), TEXT("MouseLookScale"), MouseLookScale, GGameUserSettingsIni);
		GConfig->Flush(false, GGameUserSettingsIni);
	}
	UE_LOG(LogTemp, Display, TEXT("ArchonFactoryCanary: MouseLookScaleChanged scale=%.3f"), MouseLookScale);
}

void UArchonPlayerInputBridgeComponent::QuitToMainMenu()
{
	APlayerController* Controller = ResolvePlayerController();
	if (!Controller)
	{
		return;
	}

	UE_LOG(LogTemp, Display, TEXT("ArchonFactoryCanary: PauseMenuQuitToMenu url=/Engine/Maps/Entry?ArchonMainMenu"));
	ClosePauseMenu();
	Controller->ClientTravel(TEXT("/Engine/Maps/Entry?ArchonMainMenu"), TRAVEL_Absolute);
}

void UArchonPlayerInputBridgeComponent::QuitGame()
{
	UE_LOG(LogTemp, Display, TEXT("ArchonFactoryCanary: PauseMenuQuitGame"));
	if (APlayerController* Controller = ResolvePlayerController())
	{
		Controller->ConsoleCommand(TEXT("quit"));
	}
}

void UArchonPlayerInputBridgeComponent::NotifyPlaytestEvent(const TCHAR* EventName) const
{
	UWorld* World = GetWorld();
	if (UArchonCanaryWorldSubsystem* Subsystem = World ? World->GetSubsystem<UArchonCanaryWorldSubsystem>() : nullptr)
	{
		Subsystem->NotifyPlaytestEvent(EventName);
	}
}

APlayerController* UArchonPlayerInputBridgeComponent::ResolvePlayerController() const
{
	if (PlayerController)
	{
		return PlayerController;
	}

	return Cast<APlayerController>(GetOwner());
}

UArchonMapTableInteractorComponent* UArchonPlayerInputBridgeComponent::ResolveInteractor()
{
	if (!Interactor)
	{
		AActor* Owner = GetOwner();
		UObject* InteractorOuter = Owner ? static_cast<UObject*>(Owner) : static_cast<UObject*>(this);
		Interactor = NewObject<UArchonMapTableInteractorComponent>(InteractorOuter, TEXT("ArchonRuntimeMapTableInteractor"));
		if (Interactor && Owner)
		{
			Interactor->RegisterComponent();
		}
	}

	if (Interactor)
	{
		Interactor->ConfigureInteractor(InteractorConfig);
	}

	return Interactor;
}

void UArchonPlayerInputBridgeComponent::ApplyStandardFpsLook(APlayerController& Controller)
{
	float DeltaX = 0.0f;
	float DeltaY = 0.0f;
	Controller.GetInputMouseDelta(DeltaX, DeltaY);

	if (!FMath::IsNearlyZero(DeltaX))
	{
		Controller.AddYawInput(DeltaX * MouseLookScale);
	}

	if (!FMath::IsNearlyZero(DeltaY))
	{
		Controller.AddPitchInput(DeltaY * -MouseLookScale);
	}
}

void UArchonPlayerInputBridgeComponent::ApplyStandardFpsMovement(APlayerController& Controller)
{
	APawn* Pawn = Controller.GetPawn();
	if (!Pawn)
	{
		return;
	}

	const float ForwardInput =
		(Controller.IsInputKeyDown(EKeys::W) ? 1.0f : 0.0f) +
		(Controller.IsInputKeyDown(EKeys::S) ? -1.0f : 0.0f);
	const float RightInput =
		(Controller.IsInputKeyDown(EKeys::D) ? 1.0f : 0.0f) +
		(Controller.IsInputKeyDown(EKeys::A) ? -1.0f : 0.0f);

	const FRotator ControlYaw(0.0f, Controller.GetControlRotation().Yaw, 0.0f);
	if (!FMath::IsNearlyZero(ForwardInput))
	{
		Pawn->AddMovementInput(ControlYaw.Vector(), ForwardInput);
	}

	if (!FMath::IsNearlyZero(RightInput))
	{
		Pawn->AddMovementInput(FRotationMatrix(ControlYaw).GetUnitAxis(EAxis::Y), RightInput);
	}
}

void UArchonPlayerInputBridgeComponent::ApplyStandardFpsMobility(APlayerController& Controller)
{
	ACharacter* Character = Cast<ACharacter>(Controller.GetPawn());
	if (!Character)
	{
		return;
	}

	if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
	{
		Movement->MaxWalkSpeed = Controller.IsInputKeyDown(EKeys::LeftShift) ? SprintWalkSpeed : DefaultWalkSpeed;
	}

	ApplyRuntimeFactionMovementInput(
		Character,
		Controller.IsInputKeyDown(EKeys::LeftShift),
		Controller.WasInputKeyJustPressed(EKeys::SpaceBar),
		Controller.WasInputKeyJustReleased(EKeys::SpaceBar));

	const bool bCrouchHeld = Controller.IsInputKeyDown(EKeys::LeftControl) || Controller.IsInputKeyDown(EKeys::C);
	if (bCrouchHeld && !Character->bIsCrouched)
	{
		Character->Crouch();
	}
	else if (!bCrouchHeld && Character->bIsCrouched)
	{
		Character->UnCrouch();
	}
}

void UArchonPlayerInputBridgeComponent::HandleMapTableInput(APlayerController& Controller)
{
	if (!IsDeadStateCommandAllowed())
	{
		return;
	}

	if (Controller.WasInputKeyJustPressed(EKeys::Tab))
	{
		const bool bSurfaceOpenBefore = bMapTableSurfaceOpen;
		if (bMapTableSurfaceOpen)
		{
			const bool bClosed = CloseRuntimeMapTable();
			LogMapTableInputResult(TEXT("Tab"), bSurfaceOpenBefore, bClosed, FString(TEXT("closed")));
		}
		else
		{
			const bool bOpened = PreviewRuntimeMapTable();
			LogMapTableInputResult(
				TEXT("Tab"),
				bSurfaceOpenBefore,
				bOpened,
				bOpened ? FString(TEXT("opened_from_anywhere")) : FString::Printf(TEXT("blocked_%s"), *LastInteractionResult.Reason.ToString()));
		}
	}

	if (Controller.WasInputKeyJustPressed(EKeys::E))
	{
		const bool bSurfaceOpenBefore = bMapTableSurfaceOpen;
		if (!bMapTableSurfaceOpen)
		{
			// E is the world-interact key: range-gated with an on-screen
			// prompt (Tab stays the from-anywhere commander toggle).
			const bool bOpened = TryInteract();
			LogMapTableInputResult(
				TEXT("E"),
				bSurfaceOpenBefore,
				bOpened,
				bOpened ? FString(TEXT("opened_interact")) : FString::Printf(TEXT("blocked_%s"), *LastInteractionResult.Reason.ToString()));
		}
		else
		{
			const bool bSubmitted = SubmitRuntimeRtsOrder(EArchonRtsOrderKind::MoveSquad);
			LogMapTableInputResult(
				TEXT("E"),
				bSurfaceOpenBefore,
				bSubmitted,
				bSubmitted ? FString(TEXT("move_order")) : FString::Printf(TEXT("move_blocked_%s"), *LastInteractionResult.Reason.ToString()));
		}
	}

	if (bMapTableSurfaceOpen && Controller.WasInputKeyJustPressed(EKeys::LeftMouseButton))
	{
		const bool bSubmitted = SubmitRuntimeRtsOrder(EArchonRtsOrderKind::AttackTarget);
		LogMapTableInputResult(
			TEXT("LMB"),
			true,
			bSubmitted,
			bSubmitted ? FString(TEXT("attack_order")) : FString::Printf(TEXT("attack_blocked_%s"), *LastInteractionResult.Reason.ToString()));
	}

	// B at the open table: buy a reinforcement squad from the team
	// bank (the income loop's commander verb; auto-spend stays a lazy
	// fallback above the banking threshold).
	if (bMapTableSurfaceOpen && Controller.WasInputKeyJustPressed(EKeys::B))
	{
		RequestReinforcementPurchase();
		LogMapTableInputResult(TEXT("B"), true, true, FString(TEXT("reinforcement_requested")));
	}

	// R at the open table: set the team rally point. Reinforcements and
	// later production use this as their first waypoint.
	if (bMapTableSurfaceOpen && Controller.WasInputKeyJustPressed(EKeys::R))
	{
		const bool bSubmitted = SubmitRuntimeMapTableWidgetRallyPointAt(
			FVector2D(0.70f, 0.50f),
			TEXT("runtime_team_rally"));
		LogMapTableInputResult(
			TEXT("R"),
			true,
			bSubmitted,
			bSubmitted ? FString(TEXT("rally_point_set")) : FString::Printf(TEXT("rally_blocked_%s"), *LastInteractionResult.Reason.ToString()));
	}

	// C at the open table: cycle which tech building T will place.
	if (bMapTableSurfaceOpen && Controller.WasInputKeyJustPressed(EKeys::C))
	{
		const FName SelectedTech = CycleSelectedTechBuilding(1);
		LogMapTableInputResult(
			TEXT("C"),
			true,
			!SelectedTech.IsNone(),
			FString::Printf(TEXT("tech_selected_%s"), *SelectedTech.ToString()));
	}

	// T at the open table: place the selected tech building. This is
	// the playable RTS -> FPS-shop loop: table build order, team supply
	// spend, tech unlocks matching item/unit rows.
	if (bMapTableSurfaceOpen && Controller.WasInputKeyJustPressed(EKeys::T))
	{
		const bool bPurchased = RequestTechBuildingPurchase();
		LogMapTableInputResult(
			TEXT("T"),
			true,
			bPurchased,
			bPurchased ? FString(TEXT("tech_build_requested")) : FString(TEXT("tech_build_blocked")));
	}
}

void UArchonPlayerInputBridgeComponent::RequestReinforcementPurchase()
{
	AArchonMatchStateActor* MatchState = nullptr;
	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<AArchonMatchStateActor> It(World); It; ++It)
		{
			MatchState = *It;
			break;
		}
	}
	const int32 PurchasingTeam = MapTable ? MapTable->GetTeamId() : INDEX_NONE;
	const bool bAccepted = MatchState && MatchState->TryPurchaseReinforcement(PurchasingTeam);
	const bool bTrainOrderSubmitted = bAccepted ? SubmitRuntimeRtsOrder(EArchonRtsOrderKind::TrainUnit) : false;
	UE_LOG(
		LogTemp,
		Display,
		TEXT("ArchonFactoryCanary: ReinforcementOrderedAtTable team=%d accepted=%s trainOrderSubmitted=%s"),
		PurchasingTeam,
		bAccepted ? TEXT("true") : TEXT("false"),
		bTrainOrderSubmitted ? TEXT("true") : TEXT("false"));
	NotifyPlaytestEvent(bAccepted ? TEXT("ReinforcementBought") : TEXT("ReinforcementDenied"));
	RequestMapTableVisualEvidence(bAccepted ? TEXT("map_table_reinforce") : TEXT("map_table_reinforce_denied"));
}

bool UArchonPlayerInputBridgeComponent::RequestTechBuildingPurchase()
{
	AArchonMatchStateActor* MatchState = nullptr;
	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<AArchonMatchStateActor> It(World); It; ++It)
		{
			MatchState = *It;
			break;
		}
	}

	const int32 PurchasingTeam = MapTable ? MapTable->GetTeamId() : INDEX_NONE;
	const FName TechId = GetSelectedTechBuildingId();
	const int32 TechCost = ResolveTechBuildCost(TechId);

	TArray<FArchonShopItemRow> RowsUnlockedByTech;
	for (const FArchonShopItemRow& Row : UArchonItemShopPolicyLibrary::BuildDefaultCatalog())
	{
		if (Row.RequiredTech == TechId)
		{
			RowsUnlockedByTech.Add(Row);
		}
	}
	if (MatchState)
	{
		for (const FArchonShopItemRow& Row : RowsUnlockedByTech)
		{
			const FArchonShopPurchaseDecision BeforeDecision = UArchonItemShopPolicyLibrary::EvaluatePurchase(
				Row,
				MatchState->GetTeamSupply(PurchasingTeam),
				MatchState->GetBuiltTech(PurchasingTeam));
			if (!BeforeDecision.bCanPurchase && BeforeDecision.Reason == TEXT("missing_tech"))
			{
				UE_LOG(
					LogTemp,
					Display,
					TEXT("ArchonFactoryCanary: ShopRowLocked team=%d item=%s requiredTech=%s reason=%s"),
					PurchasingTeam,
					*Row.ItemId.ToString(),
					*Row.RequiredTech.ToString(),
					*BeforeDecision.Reason.ToString());
			}
		}
	}

	const bool bBuildOrderAccepted = SubmitRuntimeRtsOrder(EArchonRtsOrderKind::BuildStructure);
	const bool bPurchaseAccepted = bBuildOrderAccepted && MatchState && MatchState->TryPurchaseTechBuilding(PurchasingTeam, TechId, TechCost);
	UE_LOG(
		LogTemp,
		Display,
		TEXT("ArchonFactoryCanary: TechBuildOrderedAtTable team=%d tech=%s orderAccepted=%s purchaseAccepted=%s"),
		PurchasingTeam,
		*TechId.ToString(),
		bBuildOrderAccepted ? TEXT("true") : TEXT("false"),
		bPurchaseAccepted ? TEXT("true") : TEXT("false"));
	NotifyPlaytestEvent(bPurchaseAccepted ? TEXT("TechBuilt") : TEXT("TechBuildDenied"));
	RequestMapTableVisualEvidence(bPurchaseAccepted ? TEXT("map_table_build") : TEXT("map_table_build_denied"));
	return bPurchaseAccepted;
}

void UArchonPlayerInputBridgeComponent::HandleFpsWeaponInput(APlayerController& Controller)
{
	if (bMapTableSurfaceOpen)
	{
		return;
	}

	ACharacter* Character = Cast<ACharacter>(Controller.GetPawn());
	if (!Character)
	{
		return;
	}

	ApplyRuntimeWeaponInput(
		Character,
		Controller.WasInputKeyJustPressed(EKeys::LeftMouseButton),
		Controller.WasInputKeyJustPressed(EKeys::R));
}

bool UArchonPlayerInputBridgeComponent::OpenRuntimeMapTableWidget()
{
	if (!MapTable)
	{
		return false;
	}

	if (!RuntimeMapTableWidget)
	{
		if (APlayerController* ActivePlayerController = ResolvePlayerController())
		{
			RuntimeMapTableWidget = CreateWidget<UArchonMapTableWidget>(
				ActivePlayerController,
				UArchonMapTableWidget::StaticClass());
		}

		if (!RuntimeMapTableWidget)
		{
			RuntimeMapTableWidget = NewObject<UArchonMapTableWidget>(
				this,
				UArchonMapTableWidget::StaticClass(),
				TEXT("ArchonRuntimeMapTableWidget"));
		}
	}

	if (!RuntimeMapTableWidget)
	{
		UE_LOG(LogTemp, Error, TEXT("ArchonFactoryCanary: RuntimeMapTableWidgetOpened opened=false reason=create_failed"));
		return false;
	}

	RuntimeMapTableWidget->ConfigureMapTableWidget(
		MapTable,
		InteractorConfig.PlayerId,
		InteractorConfig.TeamId,
		FVector(0.0f, -1000.0f, 0.0f),
		FVector(3000.0f, 2000.0f, 0.0f));
	SeedRuntimeMapTableWidgetCandidates();
	const bool bSelected = SelectRuntimeMapTableWidgetCanarySquad();

	if (ResolvePlayerController() && !RuntimeMapTableWidget->IsInViewport())
	{
		RuntimeMapTableWidget->AddToViewport();
	}

	bRuntimeMapTableWidgetMounted = true;

	const FString VisibilitySummary = RuntimeMapTableWidget->FormatVisibilitySummary();
	if (MapTable)
	{
		MapTable->SetRuntimeStatusText(FString::Printf(
			TEXT("RTS SURFACE OPEN\nCOMMAND COCKPIT READY\nTeam %d\n%s\nSelected squads: %d"),
			InteractorConfig.TeamId,
			*VisibilitySummary,
			RuntimeMapTableWidget->GetSelectedSquadIds().Num()));
	}

	UE_LOG(
		LogTemp,
		Display,
		TEXT("ArchonFactoryCanary: RuntimeMapTableWidgetOpened opened=true selected=%d summary=\"%s\""),
		RuntimeMapTableWidget->GetSelectedSquadIds().Num(),
		*VisibilitySummary);

	if (bSelected)
	{
		UE_LOG(
			LogTemp,
			Display,
			TEXT("ArchonFactoryCanary: RuntimeMapTableWidgetSelected selected=canary_squad count=%d"),
			RuntimeMapTableWidget->GetSelectedSquadIds().Num());
	}

	return true;
}

void UArchonPlayerInputBridgeComponent::CloseRuntimeMapTableWidget()
{
	const bool bWasMounted = bRuntimeMapTableWidgetMounted;

	if (RuntimeMapTableWidget && RuntimeMapTableWidget->IsInViewport())
	{
		RuntimeMapTableWidget->RemoveFromParent();
	}

	bRuntimeMapTableWidgetMounted = false;

	UE_LOG(
		LogTemp,
		Display,
		TEXT("ArchonFactoryCanary: RuntimeMapTableWidgetClosed closed=%s wasMounted=%s"),
		bWasMounted ? TEXT("true") : TEXT("false"),
		bWasMounted ? TEXT("true") : TEXT("false"));
}

void UArchonPlayerInputBridgeComponent::SeedRuntimeMapTableWidgetCandidates()
{
	if (!RuntimeMapTableWidget)
	{
		return;
	}

	FArchonSelectableSquadCandidate CanarySquad;
	CanarySquad.SquadId = TEXT("canary_squad");
	CanarySquad.TeamId = InteractorConfig.TeamId;
	CanarySquad.TableSpacePosition = FVector2D(0.21f, 0.37f);
	CanarySquad.bIsAlive = true;

	RuntimeMapTableWidget->SetSelectableSquadCandidates({ CanarySquad });
}

bool UArchonPlayerInputBridgeComponent::SelectRuntimeMapTableWidgetCanarySquad()
{
	if (!RuntimeMapTableWidget)
	{
		return false;
	}

	const bool bSelected = RuntimeMapTableWidget->CommitDragBox(
		FVector2D(0.15f, 0.30f),
		FVector2D(0.30f, 0.44f));
	return bSelected && RuntimeMapTableWidget->GetSelectedSquadIds().Contains(TEXT("canary_squad"));
}

bool UArchonPlayerInputBridgeComponent::SubmitRuntimeMapTableWidgetMoveOrder()
{
	return SubmitRuntimeMapTableWidgetMoveOrderAt(FVector2D(0.70f, 0.50f), TEXT("canary_widget_rally"));
}

bool UArchonPlayerInputBridgeComponent::SubmitRuntimeMapTableWidgetMoveOrderAt(
	const FVector2D& TableSpacePoint,
	FName TargetId)
{
	const EArchonLifeState LifeStateBeforeSubmit = RespawnState ? RespawnState->GetLifeState() : EArchonLifeState::Alive;
	if (!IsDeadStateCommandAllowed())
	{
		UE_LOG(
			LogTemp,
			Display,
			TEXT("ArchonFactoryCanary: RuntimeMapTableWidgetOrder submitted=false target=%s sequence=%d location=none reason=life_state_blocks_map_table"),
			*TargetId.ToString(),
			MapTable ? MapTable->GetCurrentCommandSequence() : 0);
		return false;
	}

	if (!RuntimeMapTableWidget || !MapTable)
	{
		return false;
	}

	if (!RuntimeMapTableWidget->GetSelectedSquadIds().Contains(TEXT("canary_squad")))
	{
		SelectRuntimeMapTableWidgetCanarySquad();
	}

	FArchonRtsAuthorityDecision Decision;
	const bool bSubmitted = RuntimeMapTableWidget->HandleRightClickOrder(
		TableSpacePoint,
		TargetId,
		Decision);

	if (bSubmitted)
	{
		++RuntimeWidgetSubmittedOrderCount;
		MapTable->SetRuntimeStatusText(FString::Printf(
			TEXT("WIDGET ORDER ACCEPTED\nSequence %d\nTarget %s"),
			MapTable->GetCurrentCommandSequence(),
			*TargetId.ToString()));
		RequestMapTableVisualEvidence(TEXT("map_table_move"));
	}
	RecordCommandSubmittedDuringDeath(bSubmitted, LifeStateBeforeSubmit);

	UE_LOG(
		LogTemp,
		Display,
		TEXT("ArchonFactoryCanary: RuntimeMapTableWidgetOrder submitted=%s target=%s sequence=%d location=%s reason=%s"),
		bSubmitted ? TEXT("true") : TEXT("false"),
		*TargetId.ToString(),
		MapTable->GetCurrentCommandSequence(),
		RuntimeMapTableWidget ? *RuntimeMapTableWidget->GetLastOrderTargetLocation().ToCompactString() : TEXT("none"),
		*Decision.Reason.ToString());

	return bSubmitted;
}

bool UArchonPlayerInputBridgeComponent::SubmitRuntimeMapTableWidgetRallyPointAt(
	const FVector2D& TableSpacePoint,
	FName TargetId)
{
	const EArchonLifeState LifeStateBeforeSubmit = RespawnState ? RespawnState->GetLifeState() : EArchonLifeState::Alive;
	if (!IsDeadStateCommandAllowed())
	{
		LastInteractionResult = FArchonMapTableInteractionResult();
		LastInteractionResult.Reason = TEXT("life_state_blocks_map_table");
		UE_LOG(
			LogTemp,
			Display,
			TEXT("ArchonFactoryCanary: RuntimeRallyPointSet submitted=false target=%s sequence=%d location=none reason=life_state_blocks_map_table"),
			*TargetId.ToString(),
			MapTable ? MapTable->GetCurrentCommandSequence() : 0);
		return false;
	}

	if (!RuntimeMapTableWidget || !MapTable)
	{
		LastInteractionResult = FArchonMapTableInteractionResult();
		LastInteractionResult.Reason = TEXT("runtime_widget_not_ready");
		return false;
	}

	FArchonRtsAuthorityDecision Decision;
	const bool bSubmitted = RuntimeMapTableWidget->HandleRallyPointOrder(
		TableSpacePoint,
		TargetId,
		Decision);

	if (bSubmitted)
	{
		++RuntimeWidgetSubmittedOrderCount;
		++SubmittedOrderCount;
		MapTable->SetRuntimeStatusText(FString::Printf(
			TEXT("RALLY POINT SET\nSequence %d\nTarget %s"),
			MapTable->GetCurrentCommandSequence(),
			*TargetId.ToString()));
		NotifyPlaytestEvent(TEXT("RallyPointSet"));
		RequestMapTableVisualEvidence(TEXT("map_table_rally"));
	}
	LastInteractionResult.Reason = Decision.Reason;
	RecordCommandSubmittedDuringDeath(bSubmitted, LifeStateBeforeSubmit);

	UE_LOG(
		LogTemp,
		Display,
		TEXT("ArchonFactoryCanary: RuntimeRallyPointSet submitted=%s target=%s sequence=%d location=%s reason=%s"),
		bSubmitted ? TEXT("true") : TEXT("false"),
		*TargetId.ToString(),
		MapTable->GetCurrentCommandSequence(),
		RuntimeMapTableWidget ? *RuntimeMapTableWidget->GetLastOrderTargetLocation().ToCompactString() : TEXT("none"),
		*Decision.Reason.ToString());

	return bSubmitted;
}

void UArchonPlayerInputBridgeComponent::RecordCommandSubmittedDuringDeath(
	bool bSubmitted,
	EArchonLifeState LifeStateBeforeSubmit)
{
	if (!bSubmitted || LifeStateBeforeSubmit != EArchonLifeState::Dead)
	{
		return;
	}

	++CommandsIssuedDuringDeath;
	UE_LOG(
		LogTemp,
		Display,
		TEXT("ArchonFactoryCanary: CommandWhileYouWait submitted=true lifeState=Dead sequence=%d"),
		MapTable ? MapTable->GetCurrentCommandSequence() : 0);
}

FArchonRtsCommandIntent UArchonPlayerInputBridgeComponent::MakeRuntimeIntent(EArchonRtsOrderKind OrderKind) const
{
	FArchonRtsCommandIntent Intent;
	Intent.TeamId = InteractorConfig.TeamId;
	Intent.IssuingPlayerId = InteractorConfig.PlayerId;
	Intent.OrderKind = OrderKind;
	if (OrderKind == EArchonRtsOrderKind::BuildStructure)
	{
		Intent.SubjectId = GetSelectedTechBuildingId();
		Intent.TargetId = GetSelectedTechBuildingTargetId();
	}
	else if (OrderKind == EArchonRtsOrderKind::TrainUnit)
	{
		Intent.SubjectId = TEXT("reinforcement_queue");
		Intent.TargetId = TEXT("verdant_reinforcement_squad");
	}
	else if (OrderKind == EArchonRtsOrderKind::SetRallyPoint)
	{
		Intent.SubjectId = TEXT("team_rally");
		Intent.TargetId = TEXT("runtime_team_rally");
	}
	else
	{
		Intent.SubjectId = TEXT("canary_squad");
		Intent.TargetId = OrderKind == EArchonRtsOrderKind::AttackTarget ? TEXT("canary_target") : TEXT("canary_rally_point");
	}
	return Intent;
}
