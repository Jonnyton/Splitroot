#include "ArchonMatchStateActor.h"
#include "ArchonBaseCoreActor.h"
#include "ArchonBaseDefenseTowerActor.h"
#include "ArchonBotFeatureTelemetry.h"
#include "ArchonCombatHealthComponent.h"
#include "ArchonItemShopPolicyLibrary.h"
#include "ArchonLaunchFlagLibrary.h"
#include "ArchonMapRegistryLibrary.h"
#include "ArchonMatchPolicyLibrary.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"
#include "Net/UnrealNetwork.h"

namespace
{
	const TCHAR* PhaseName(EArchonMatchPhase Phase)
	{
		switch (Phase)
		{
		case EArchonMatchPhase::Warmup:    return TEXT("Warmup");
		case EArchonMatchPhase::Live:      return TEXT("Live");
		case EArchonMatchPhase::MatchEnd:  return TEXT("MatchEnd");
		case EArchonMatchPhase::Traveling: return TEXT("Traveling");
		}
		return TEXT("Unknown");
	}

	const TCHAR* WinnerName(EArchonMatchWinner Winner)
	{
		switch (Winner)
		{
		case EArchonMatchWinner::Undecided: return TEXT("Undecided");
		case EArchonMatchWinner::TeamA:     return TEXT("TeamA");
		case EArchonMatchWinner::TeamB:     return TEXT("TeamB");
		case EArchonMatchWinner::Draw:      return TEXT("Draw");
		}
		return TEXT("Unknown");
	}

	const FDateTime GArchonProcessStartUtc = FDateTime::UtcNow();

	FString FormatUtcOrMissing(const FDateTime& Timestamp)
	{
		return Timestamp.GetTicks() > 0 ? Timestamp.ToIso8601() : FString(TEXT("missing"));
	}

	FDateTime MaxTimestamp(const FDateTime& A, const FDateTime& B)
	{
		if (A.GetTicks() <= 0)
		{
			return B;
		}
		if (B.GetTicks() <= 0)
		{
			return A;
		}
		return B > A ? B : A;
	}

	FString MakeBuildVersion(const FString& ModuleDllUtc)
	{
		FString Version = ModuleDllUtc;
		Version.ReplaceInline(TEXT("-"), TEXT(""));
		Version.ReplaceInline(TEXT(":"), TEXT(""));
		Version.ReplaceInline(TEXT("."), TEXT(""));
		Version.ReplaceInline(TEXT("Z"), TEXT(""));
		Version.ReplaceInline(TEXT("T"), TEXT("-"));
		if (Version.IsEmpty() || Version.Equals(TEXT("missing"), ESearchCase::IgnoreCase))
		{
			return FString(TEXT("vunknown"));
		}
		return FString::Printf(TEXT("v%s"), *Version.Left(15));
	}

	bool ShouldIgnoreRuntimeInputFile(const FString& File)
	{
		FString Normalized = File;
		Normalized.ReplaceInline(TEXT("\\"), TEXT("/"));
		return Normalized.Contains(TEXT("/FactoryContracts/thumbs/generated/"), ESearchCase::IgnoreCase);
	}

	void AccumulateNewestRuntimeInputUtc(
		IFileManager& FileManager,
		const FString& Root,
		const TSet<FString>& Extensions,
		FDateTime& InOutNewestUtc)
	{
		if (!FileManager.DirectoryExists(*Root))
		{
			if (FileManager.FileExists(*Root))
			{
				const FString Ext = FPaths::GetExtension(Root, true).ToLower();
				if (Extensions.Contains(Ext) && !ShouldIgnoreRuntimeInputFile(Root))
				{
					InOutNewestUtc = MaxTimestamp(InOutNewestUtc, FileManager.GetTimeStamp(*Root));
				}
			}
			return;
		}

		TArray<FString> Files;
		FileManager.FindFilesRecursive(Files, *Root, TEXT("*.*"), true, false);
		for (const FString& File : Files)
		{
			const FString Ext = FPaths::GetExtension(File, true).ToLower();
			if (Extensions.Contains(Ext) && !ShouldIgnoreRuntimeInputFile(File))
			{
				InOutNewestUtc = MaxTimestamp(InOutNewestUtc, FileManager.GetTimeStamp(*File));
			}
		}
	}

	FDateTime ResolveRuntimeInputUtc()
	{
		IFileManager& FileManager = IFileManager::Get();
		const FString ProjectDir = FPaths::ProjectDir();
		const TSet<FString> ContractExtensions = {
			TEXT(".ini"),
			TEXT(".json"),
			TEXT(".uproject")
		};
		const TSet<FString> ContentExtensions = {
			TEXT(".uasset"),
			TEXT(".umap"),
			TEXT(".png"),
			TEXT(".wav"),
			TEXT(".obj"),
			TEXT(".glb"),
			TEXT(".fbx")
		};

		FDateTime NewestUtc;
		AccumulateNewestRuntimeInputUtc(FileManager, FPaths::Combine(ProjectDir, TEXT("Config")), ContractExtensions, NewestUtc);
		AccumulateNewestRuntimeInputUtc(FileManager, FPaths::Combine(ProjectDir, TEXT("FactoryContracts")), ContractExtensions, NewestUtc);
		AccumulateNewestRuntimeInputUtc(FileManager, FPaths::Combine(ProjectDir, TEXT("Content")), ContentExtensions, NewestUtc);
		AccumulateNewestRuntimeInputUtc(FileManager, FPaths::Combine(ProjectDir, TEXT("games"), TEXT("splitroot"), TEXT("FactoryContracts")), ContractExtensions, NewestUtc);
		AccumulateNewestRuntimeInputUtc(FileManager, FPaths::Combine(ProjectDir, TEXT("games"), TEXT("splitroot"), TEXT("Content")), ContentExtensions, NewestUtc);
		AccumulateNewestRuntimeInputUtc(FileManager, FPaths::Combine(ProjectDir, TEXT("ArchonFactoryCanary.uproject")), ContractExtensions, NewestUtc);
		return NewestUtc;
	}

	FString ResolveModuleDllPath()
	{
		return FPaths::ConvertRelativePathToFull(FPaths::Combine(
			FPaths::ProjectDir(),
			TEXT("Binaries"),
			TEXT("Win64"),
			TEXT("UnrealEditor-ArchonFactoryCanary.dll")));
	}
}

AArchonMatchStateActor::AArchonMatchStateActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	bReplicates = true;
	bAlwaysRelevant = true;
}

void AArchonMatchStateActor::InitializeMatch(
	const FArchonMatchConfig& InConfig,
	const TArray<FArchonValleyPlacement>& SitePlacements,
	AArchonBaseCoreActor* InTeamACore,
	AArchonBaseCoreActor* InTeamBCore,
	bool bInProofScript)
{
	Config = InConfig;
	Sites = SitePlacements;
	SiteStates.Init(FArchonResourceSiteState(), Sites.Num());
	TeamACore = InTeamACore;
	TeamBCore = InTeamBCore;
	bProofScript = bInProofScript;
	bInitialized = true;

	// Points are damage dealt to the enemy core. Cores never take friendly
	// fire in v0, so credit always goes to the damaged core's opponent.
	if (TeamACore && TeamACore->GetHealth())
	{
		TeamACore->GetHealth()->OnHealthChanged.AddLambda(
			[this](const FArchonDamageApplicationResult& DamageResult)
			{
				HandleCoreDamaged(0, DamageResult);
			});
	}
	if (TeamBCore && TeamBCore->GetHealth())
	{
		TeamBCore->GetHealth()->OnHealthChanged.AddLambda(
			[this](const FArchonDamageApplicationResult& DamageResult)
			{
				HandleCoreDamaged(1, DamageResult);
			});
	}

	UE_LOG(
		LogTemp,
		Display,
		TEXT("ArchonFactoryCanary: MatchLoopActive sites=%d proof=%s warmupSeconds=%.1f"),
		Sites.Num(),
		bProofScript ? TEXT("true") : TEXT("false"),
		Config.WarmupSeconds);
	const FString ModuleDllPath = ResolveModuleDllPath();
	const FDateTime ModuleDllTimestampUtc = IFileManager::Get().GetTimeStamp(*ModuleDllPath);
	const FDateTime RuntimeInputTimestampUtc = ResolveRuntimeInputUtc();
	const FString ModuleDllUtc = FormatUtcOrMissing(ModuleDllTimestampUtc);
	const FString ProcessStartUtc = GArchonProcessStartUtc.ToIso8601();
	const FString RuntimeInputUtc = FormatUtcOrMissing(RuntimeInputTimestampUtc);
	EffectiveBuildModuleUtc = ModuleDllUtc;
	EffectiveBuildRuntimeUtc = RuntimeInputUtc;
	EffectiveBuildUtc = ProcessStartUtc;
	EffectiveBuildVersion = MakeBuildVersion(FormatUtcOrMissing(MaxTimestamp(ModuleDllTimestampUtc, RuntimeInputTimestampUtc)));
	UE_LOG(
		LogTemp,
		Display,
		TEXT("ArchonFactoryCanary: BuildFingerprint moduleDllUtc=%s runtimeInputUtc=%s processStartUtc=%s modulePath=%s version=%s effectiveUtc=%s"),
		*ModuleDllUtc,
		*EffectiveBuildRuntimeUtc,
		*ProcessStartUtc,
		*ModuleDllPath,
		*EffectiveBuildVersion,
		*EffectiveBuildUtc);
	UE_LOG(
		LogTemp,
		Display,
		TEXT("ArchonFactoryCanary: BuildVersionEffective version=%s effectiveUtc=%s moduleDllUtc=%s runtimeInputUtc=%s"),
		*EffectiveBuildVersion,
		*EffectiveBuildUtc,
		*EffectiveBuildModuleUtc,
		*EffectiveBuildRuntimeUtc);
	UE_LOG(
		LogTemp,
		Display,
		TEXT("ArchonFactoryCanary: MatchConfig warmupSeconds=%.1f matchTimeLimitSeconds=%.1f scoreboardSeconds=%.1f maxCycleSeconds=%.1f"),
		Config.WarmupSeconds,
		Config.MatchTimeLimitSeconds,
		Config.ScoreboardSeconds,
		Config.WarmupSeconds + Config.MatchTimeLimitSeconds + Config.ScoreboardSeconds);
	UE_LOG(LogTemp, Display, TEXT("ArchonFactoryCanary: MatchPhase phase=Warmup"));
}

void AArchonMatchStateActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bInitialized || MatchClock.Phase == EArchonMatchPhase::Traveling)
	{
		return;
	}

	if (MatchClock.Phase == EArchonMatchPhase::Live)
	{
		LiveSeconds += DeltaSeconds;
		TickSites(DeltaSeconds);
		TickSupply(DeltaSeconds);
		if (HasAuthority())
		{
			TickAutoReinforce();
		}
	}

	Winner = EvaluateCurrentWinner();

	const EArchonMatchPhase OldPhase = MatchClock.Phase;
	MatchClock = UArchonMatchPolicyLibrary::TickMatchClock(MatchClock, DeltaSeconds, Config, Winner);
	if (MatchClock.Phase != OldPhase)
	{
		HandlePhaseChange(OldPhase, MatchClock.Phase);
	}

	if (bProofScript)
	{
		RunProofScript();
	}

	RefreshNetSnapshot();
}

void AArchonMatchStateActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AArchonMatchStateActor, NetSnapshot);
}

void AArchonMatchStateActor::RefreshNetSnapshot()
{
	NetSnapshot.Phase = MatchClock.Phase;
	NetSnapshot.Winner = Winner;
	NetSnapshot.PhaseSeconds = MatchClock.PhaseSeconds;
	NetSnapshot.ScoreboardSeconds = Config.ScoreboardSeconds;
	NetSnapshot.SupplyA = TeamSupply[0];
	NetSnapshot.SupplyB = TeamSupply[1];
	NetSnapshot.PointsA = TeamPoints[0];
	NetSnapshot.PointsB = TeamPoints[1];
	NetSnapshot.NextMapId = PendingNextMapId;
	NetSnapshot.MatchEndReason = MatchEndReason;
	NetSnapshot.BuildVersion = EffectiveBuildVersion;
	NetSnapshot.BuildEffectiveUtc = EffectiveBuildUtc;
	NetSnapshot.BuildModuleUtc = EffectiveBuildModuleUtc;
	NetSnapshot.BuildRuntimeUtc = EffectiveBuildRuntimeUtc;

	// Base-under-attack readouts (fair-senses HUD: which building +
	// how hurt; never where the attacker is).
	const auto BuildAlert = [this](int32 Team) -> FString
	{
		FArchonBaseAttackEvent Event;
		if (GetLatestBaseAttack(Team, 8.0, Event))
		{
			return FString::Printf(TEXT("%s %d%%"), *Event.StructureLabel.ToString(), FMath::RoundToInt(Event.HealthFraction * 100.0f));
		}
		return FString();
	};
	NetSnapshot.BaseAlertA = BuildAlert(0);
	NetSnapshot.BaseAlertB = BuildAlert(1);
}

void AArchonMatchStateActor::OnRep_NetSnapshot()
{
	if (static_cast<uint8>(NetSnapshot.Phase) == LastClientLoggedPhase)
	{
		return;
	}

	LastClientLoggedPhase = static_cast<uint8>(NetSnapshot.Phase);
	UE_LOG(
		LogTemp,
		Display,
		TEXT("ArchonFactoryCanary: MatchStateClientSnapshot phase=%s supplyA=%d supplyB=%d pointsA=%d pointsB=%d winner=%s"),
		PhaseName(NetSnapshot.Phase),
		NetSnapshot.SupplyA,
		NetSnapshot.SupplyB,
		NetSnapshot.PointsA,
		NetSnapshot.PointsB,
		WinnerName(NetSnapshot.Winner));
}

int32 AArchonMatchStateActor::GetTeamSupply(int32 InTeamId) const
{
	return (InTeamId == 0 || InTeamId == 1) ? TeamSupply[InTeamId] : 0;
}

int32 AArchonMatchStateActor::GetTeamPoints(int32 InTeamId) const
{
	return (InTeamId == 0 || InTeamId == 1) ? TeamPoints[InTeamId] : 0;
}

int32 AArchonMatchStateActor::GetSitesOwnedByTeam(int32 InTeamId) const
{
	int32 Owned = 0;
	for (const FArchonResourceSiteState& Site : SiteStates)
	{
		if (Site.OwningTeam == InTeamId)
		{
			++Owned;
		}
	}
	return Owned;
}

void AArchonMatchStateActor::ScanSitePresence(int32 SiteIndex, int32& OutPresentTeamA, int32& OutPresentTeamB) const
{
	OutPresentTeamA = 0;
	OutPresentTeamB = 0;
	if (!Sites.IsValidIndex(SiteIndex))
	{
		return;
	}
	const FVector& SiteLocation = Sites[SiteIndex].Location;

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// Anything alive with a combat health component projects presence:
	// FPS bodies, squads, and faction AI all carry one. Cores are
	// objectives, not soldiers — they do not hold their own ground.
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (!Actor || Actor->IsA<AArchonBaseCoreActor>())
		{
			continue;
		}
		const UArchonCombatHealthComponent* ActorHealth = Actor->FindComponentByClass<UArchonCombatHealthComponent>();
		if (!ActorHealth || !ActorHealth->IsAlive())
		{
			continue;
		}
		if (!UArchonMatchPolicyLibrary::IsLocationInsideSite(Actor->GetActorLocation(), SiteLocation, Config))
		{
			continue;
		}
		if (ActorHealth->GetTeamId() == 0)
		{
			++OutPresentTeamA;
		}
		else if (ActorHealth->GetTeamId() == 1)
		{
			++OutPresentTeamB;
		}
	}
}

void AArchonMatchStateActor::TickSites(float DeltaSeconds)
{
	for (int32 SiteIndex = 0; SiteIndex < SiteStates.Num(); ++SiteIndex)
	{
		int32 PresentTeamA = 0;
		int32 PresentTeamB = 0;
		ScanSitePresence(SiteIndex, PresentTeamA, PresentTeamB);

		const int32 OldOwner = SiteStates[SiteIndex].OwningTeam;
		SiteStates[SiteIndex] = UArchonMatchPolicyLibrary::TickSiteCapture(
			SiteStates[SiteIndex], PresentTeamA, PresentTeamB, DeltaSeconds, Config);
		if (SiteStates[SiteIndex].OwningTeam != OldOwner)
		{
			UE_LOG(
				LogTemp,
				Display,
				TEXT("ArchonFactoryCanary: SiteCaptured site=%s team=%d"),
				*Sites[SiteIndex].PieceId.ToString(),
				SiteStates[SiteIndex].OwningTeam);
			if (UArchonLaunchFlagLibrary::IsLaunchFlagActive(GetWorld(), TEXT("ArchonBotMatch")))
			{
				ArchonBotFeatureTelemetry::LogUse(
					INDEX_NONE,
					SiteStates[SiteIndex].OwningTeam,
					TEXT("capture_site"),
					TEXT("attempt"),
					TEXT("success"),
					TEXT("site_captured"));
			}
		}
	}
}

void AArchonMatchStateActor::TickSupply(float DeltaSeconds)
{
	SupplyAccumulatorSeconds += DeltaSeconds;
	while (SupplyAccumulatorSeconds >= Config.SupplyTickIntervalSeconds)
	{
		SupplyAccumulatorSeconds -= Config.SupplyTickIntervalSeconds;
		for (int32 Team = 0; Team < 2; ++Team)
		{
			const int32 SitesOwned = GetSitesOwnedByTeam(Team);
			const int32 Gained = UArchonMatchPolicyLibrary::ComputeSupplyPerTick(SitesOwned, Config);
			TeamSupply[Team] += Gained;
			UE_LOG(
				LogTemp,
				Display,
				TEXT("ArchonFactoryCanary: SupplyTick team=%d sites=%d gained=%d total=%d"),
				Team,
				SitesOwned,
				Gained,
				TeamSupply[Team]);
		}
		bAnySupplyTickLogged = true;
	}
}

EArchonMatchWinner AArchonMatchStateActor::EvaluateCurrentWinner() const
{
	if (MatchClock.Phase != EArchonMatchPhase::Live)
	{
		return Winner;
	}
	const float CoreHealthA = (TeamACore && TeamACore->GetHealth()) ? TeamACore->GetHealth()->GetCurrentHealth() : 0.0f;
	const float CoreHealthB = (TeamBCore && TeamBCore->GetHealth()) ? TeamBCore->GetHealth()->GetCurrentHealth() : 0.0f;
	const bool bTimeLimitExpired = LiveSeconds >= Config.MatchTimeLimitSeconds;
	return UArchonMatchPolicyLibrary::EvaluateWinner(
		CoreHealthA, CoreHealthB, bTimeLimitExpired, TeamPoints[0], TeamPoints[1],
		GetSitesOwnedByTeam(0), GetSitesOwnedByTeam(1));
}

void AArchonMatchStateActor::HandlePhaseChange(EArchonMatchPhase OldPhase, EArchonMatchPhase NewPhase)
{
	UE_LOG(LogTemp, Display, TEXT("ArchonFactoryCanary: MatchPhase phase=%s"), PhaseName(NewPhase));

	if (NewPhase == EArchonMatchPhase::MatchEnd)
	{
		const bool bCoreADead = TeamACore && TeamACore->GetHealth() && !TeamACore->GetHealth()->IsAlive();
		const bool bCoreBDead = TeamBCore && TeamBCore->GetHealth() && !TeamBCore->GetHealth()->IsAlive();
		MatchEndReason = (bCoreADead || bCoreBDead) ? TEXT("core_destroyed") : TEXT("time_limit");
		const FArchonMapEntry NextMap = ResolveNextMapEntry(GetWorld());
		PendingNextMapId = NextMap.MapId;
		UE_LOG(
			LogTemp,
			Display,
			TEXT("ArchonFactoryCanary: MatchEnd winner=%s reason=%s pointsA=%d pointsB=%d liveSeconds=%.1f sitesA=%d sitesB=%d"),
			WinnerName(Winner),
			*MatchEndReason,
			TeamPoints[0],
			TeamPoints[1],
			LiveSeconds,
			GetSitesOwnedByTeam(0),
			GetSitesOwnedByTeam(1));
		UE_LOG(
			LogTemp,
			Display,
			TEXT("ArchonFactoryCanary: NextMatchPending nextMap=%s countdownSeconds=%.1f"),
			*PendingNextMapId.ToString(),
			Config.ScoreboardSeconds);
	}

	if (NewPhase == EArchonMatchPhase::Traveling && !bTravelRequested)
	{
		bTravelRequested = true;
		UE_LOG(LogTemp, Display, TEXT("ArchonFactoryCanary: TravelRequested reason=match_complete"));

		UWorld* World = GetWorld();
		const bool bProofTravel =
			UArchonLaunchFlagLibrary::IsLaunchFlagActive(World, TEXT("ArchonMatchProofTravel"));
		const bool bProofRestart =
			UArchonLaunchFlagLibrary::IsLaunchFlagActive(World, TEXT("ArchonMatchProofRestart"));

		// Plain proof mode ends the process at the travel boundary (the
		// match-loop smoke's contract). Proof-travel mode and live play
		// advance the rotation for real — Renegade pattern: everyone
		// auto-loads the next map, no lobby return.
		if (bProofScript && !bProofTravel && !bProofRestart)
		{
			if (APlayerController* PlayerController = World ? World->GetFirstPlayerController() : nullptr)
			{
				PlayerController->ConsoleCommand(TEXT("quit"));
			}
			return;
		}
		if (!World)
		{
			return;
		}

		FArchonMapEntry NextMap = ResolveNextMapEntry(World);
		PendingNextMapId = NextMap.MapId;
		TArray<FString> ExtraOptions;
		if (bProofScript && UArchonLaunchFlagLibrary::IsLaunchFlagActive(World, TEXT("ArchonMatchProofQuitAfterTravel")))
		{
			ExtraOptions.Add(TEXT("ArchonMatchProofQuit"));
		}

		// Perpetual bot playtest (Jonathan 2026-06-10): bot matches keep
		// the bot flag across travel. The match-map picker above skips
		// non-match templates, so Valley loops onto itself until another
		// ArchonMatchLoop map enters the rotation.
		if (UArchonLaunchFlagLibrary::IsLaunchFlagActive(World, TEXT("ArchonBotMatch")))
		{
			ExtraOptions.Add(TEXT("ArchonBotMatch"));
		}
		if (World && UArchonLaunchFlagLibrary::HasUrlOption(World->URL, TEXT("listen")))
		{
			ExtraOptions.Add(TEXT("listen"));
		}
		const FString TravelUrl = UArchonMapRegistryLibrary::BuildTravelUrl(NextMap, ExtraOptions);
		UE_LOG(
			LogTemp,
			Display,
			TEXT("ArchonFactoryCanary: NextMatchLoading map=%s"),
			*NextMap.MapId.ToString());
		UE_LOG(
			LogTemp,
			Display,
			TEXT("ArchonFactoryCanary: TravelingTo map=%s url=%s"),
			*NextMap.MapId.ToString(),
			*TravelUrl);
		World->ServerTravel(TravelUrl, /*bAbsolute=*/true);
	}
}

FArchonMapEntry AArchonMatchStateActor::ResolveNextMapEntry(const UWorld* World) const
{
	FName CurrentMapId = NAME_None;
	if (World)
	{
		const FString CurrentIdString = World->URL.GetOption(TEXT("ArchonMapId="), TEXT(""));
		if (!CurrentIdString.IsEmpty())
		{
			CurrentMapId = FName(*CurrentIdString);
		}
	}
	if (CurrentMapId.IsNone())
	{
		// Command-line launches carry no map id; the match loop only runs
		// on the valley, so that is who we are.
		CurrentMapId = TEXT("splitroot_valley");
	}

	const TArray<FArchonMapEntry> Rotation = UArchonMapRegistryLibrary::GetSplitrootMapRotation();
	const bool bProofTravel =
		UArchonLaunchFlagLibrary::IsLaunchFlagActive(World, TEXT("ArchonMatchProofTravel"));
	return bProofTravel
		? UArchonMapRegistryLibrary::GetNextMapInRotation(Rotation, CurrentMapId)
		: UArchonMapRegistryLibrary::GetNextMatchMapInRotation(Rotation, CurrentMapId);
}

void AArchonMatchStateActor::RunProofScript()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// Step 1: once live, stand the player on the central site so presence
	// capture runs against a real pawn, not a synthetic location.
	if (MatchClock.Phase == EArchonMatchPhase::Live && !bProofPlayerPlaced && Sites.Num() > 0)
	{
		if (APlayerController* PlayerController = World->GetFirstPlayerController())
		{
			if (APawn* Pawn = PlayerController->GetPawn())
			{
				Pawn->SetActorLocation(Sites[0].Location + FVector(0.0, 0.0, 150.0));
				bProofPlayerPlaced = true;
				UE_LOG(LogTemp, Display, TEXT("ArchonFactoryCanary: MatchProofPlayerPlaced site=%s"), *Sites[0].PieceId.ToString());
			}
		}
	}

	// Step 2: after the economy has visibly run (site flipped + supply
	// ticked), destroy the enemy core to drive the win path end to end.
	if (!bProofCoreKillIssued && bAnySupplyTickLogged && GetSitesOwnedByTeam(0) > 0 &&
		TeamBCore && TeamBCore->GetHealth() && TeamBCore->GetHealth()->IsAlive())
	{
		bProofCoreKillIssued = true;
		TeamBCore->GetHealth()->ApplyDirectDamage(
			Config.CoreMaxHealth * 10.0f, EArchonDamageType::VerdantLivingArrow, 0, INDEX_NONE);
		UE_LOG(LogTemp, Display, TEXT("ArchonFactoryCanary: MatchProofCoreKillIssued target=lenswright_base_core"));
	}
}

void AArchonMatchStateActor::HandleCoreDamaged(int32 DamagedCoreTeam, const FArchonDamageApplicationResult& DamageResult)
{
	const int32 ScoringTeam = 1 - DamagedCoreTeam;
	if (ScoringTeam == 0 || ScoringTeam == 1)
	{
		TeamPoints[ScoringTeam] += FMath::RoundToInt(DamageResult.DamageApplied);
	}

	if (DamageResult.bAccepted && DamageResult.DamageApplied > 0.0f)
	{
		AArchonBaseCoreActor* Core = DamagedCoreTeam == 0 ? TeamACore.Get() : TeamBCore.Get();
		UArchonCombatHealthComponent* CoreHealth = Core ? Core->GetHealth() : nullptr;
		if (Core)
		{
			RecordBaseAttack(
				DamagedCoreTeam,
				TEXT("CORE"),
				Core->GetActorLocation(),
				CoreHealth ? CoreHealth->GetLastAcceptedShotOrigin() : FVector::ZeroVector,
				CoreHealth ? CoreHealth->GetHealthFraction() : 0.0f);
		}
	}
}

bool AArchonMatchStateActor::TryPurchaseReinforcement(int32 TeamId)
{
	if (TeamId < 0 || TeamId > 1 || MatchClock.Phase != EArchonMatchPhase::Live)
	{
		return false;
	}
	if (!UArchonMatchPolicyLibrary::CanAffordReinforcement(TeamSupply[TeamId], Config))
	{
		return false;
	}
	TeamSupply[TeamId] -= Config.ReinforcementSquadSupplyCost;
	UE_LOG(
		LogTemp,
		Display,
		TEXT("ArchonFactoryCanary: ReinforcementPurchased team=%d cost=%d size=%d remainingSupply=%d"),
		TeamId,
		Config.ReinforcementSquadSupplyCost,
		Config.ReinforcementSquadSize,
		TeamSupply[TeamId]);
	OnReinforcementPurchased.Broadcast(TeamId, Config.ReinforcementSquadSize);
	return true;
}

bool AArchonMatchStateActor::TryPurchaseTechBuilding(int32 TeamId, FName TechId, int32 Cost)
{
	if (TeamId < 0 || TeamId > 1)
	{
		UE_LOG(LogTemp, Display, TEXT("ArchonFactoryCanary: TechBuildRejected team=%d tech=%s reason=invalid_team"), TeamId, *TechId.ToString());
		return false;
	}
	if (MatchClock.Phase != EArchonMatchPhase::Live)
	{
		UE_LOG(LogTemp, Display, TEXT("ArchonFactoryCanary: TechBuildRejected team=%d tech=%s reason=match_not_live"), TeamId, *TechId.ToString());
		return false;
	}
	if (TechId.IsNone())
	{
		UE_LOG(LogTemp, Display, TEXT("ArchonFactoryCanary: TechBuildRejected team=%d tech=none reason=invalid_tech"), TeamId);
		return false;
	}
	if (BuiltTech[TeamId].Contains(TechId))
	{
		UE_LOG(LogTemp, Display, TEXT("ArchonFactoryCanary: TechBuildRejected team=%d tech=%s reason=already_built"), TeamId, *TechId.ToString());
		return false;
	}

	const int32 SafeCost = FMath::Max(0, Cost);
	if (TeamSupply[TeamId] < SafeCost)
	{
		UE_LOG(
			LogTemp,
			Display,
			TEXT("ArchonFactoryCanary: TechBuildRejected team=%d tech=%s reason=insufficient_supply cost=%d supply=%d"),
			TeamId,
			*TechId.ToString(),
			SafeCost,
			TeamSupply[TeamId]);
		return false;
	}

	TeamSupply[TeamId] -= SafeCost;
	AddBuiltTech(TeamId, TechId);
	UE_LOG(
		LogTemp,
		Display,
		TEXT("ArchonFactoryCanary: TechBuilt team=%d tech=%s cost=%d remainingSupply=%d"),
		TeamId,
		*TechId.ToString(),
		SafeCost,
		TeamSupply[TeamId]);

	for (const FArchonShopItemRow& Row : UArchonItemShopPolicyLibrary::BuildDefaultCatalog())
	{
		if (Row.RequiredTech == TechId)
		{
			const FArchonShopPurchaseDecision Decision =
				UArchonItemShopPolicyLibrary::EvaluatePurchase(Row, TeamSupply[TeamId], BuiltTech[TeamId]);
			UE_LOG(
				LogTemp,
				Display,
				TEXT("ArchonFactoryCanary: ShopRowUnlocked team=%d item=%s requiredTech=%s finalCost=%d affordable=%s reason=%s"),
				TeamId,
				*Row.ItemId.ToString(),
				*TechId.ToString(),
				Decision.FinalCost,
				Decision.bCanPurchase ? TEXT("true") : TEXT("false"),
				*Decision.Reason.ToString());
		}
	}

	OnTechBuildingPurchased.Broadcast(TeamId, TechId);
	return true;
}

void AArchonMatchStateActor::AddBuiltTech(int32 TeamId, FName TechId)
{
	if (TeamId < 0 || TeamId > 1 || TechId.IsNone())
	{
		return;
	}
	BuiltTech[TeamId].AddUnique(TechId);
}

void AArchonMatchStateActor::RemoveBuiltTech(int32 TeamId, FName TechId)
{
	if (TeamId < 0 || TeamId > 1 || TechId.IsNone())
	{
		return;
	}
	const int32 Removed = BuiltTech[TeamId].Remove(TechId);
	if (Removed <= 0)
	{
		return;
	}

	for (const FArchonShopItemRow& Row : UArchonItemShopPolicyLibrary::BuildDefaultCatalog())
	{
		if (Row.RequiredTech == TechId)
		{
			const FArchonShopPurchaseDecision Decision =
				UArchonItemShopPolicyLibrary::EvaluatePurchase(Row, TeamSupply[TeamId], BuiltTech[TeamId]);
			UE_LOG(
				LogTemp,
				Display,
				TEXT("ArchonFactoryCanary: ShopRowLockedOrInflatedAfterTechLoss team=%d item=%s requiredTech=%s reason=%s"),
				TeamId,
				*Row.ItemId.ToString(),
				*TechId.ToString(),
				*Decision.Reason.ToString());
		}
	}
}

TArray<FName> AArchonMatchStateActor::GetBuiltTech(int32 TeamId) const
{
	return (TeamId >= 0 && TeamId <= 1) ? BuiltTech[TeamId] : TArray<FName>();
}

void AArchonMatchStateActor::TickAutoReinforce()
{
	// Lazy banker (WC3: map control -> income -> army): the auto
	// spender buys only past the banking threshold, so a human
	// commander at the table gets first claim on the bank. One
	// purchase per team per tick keeps the burn readable in the log.
	for (int32 Team = 0; Team < 2; ++Team)
	{
		if (UArchonMatchPolicyLibrary::ShouldAutoReinforce(TeamSupply[Team], Config))
		{
			TryPurchaseReinforcement(Team);
		}
	}
}

bool AArchonMatchStateActor::GetSiteInfo(int32 SiteIndex, FVector& OutLocation, int32& OutOwningTeam) const
{
	if (!Sites.IsValidIndex(SiteIndex) || !SiteStates.IsValidIndex(SiteIndex))
	{
		return false;
	}
	OutLocation = Sites[SiteIndex].Location;
	OutOwningTeam = SiteStates[SiteIndex].OwningTeam;
	return true;
}

void AArchonMatchStateActor::NotifyTowerDamaged(AArchonBaseDefenseTowerActor* Tower, const FArchonDamageApplicationResult& DamageResult)
{
	if (!Tower || !DamageResult.bAccepted || DamageResult.DamageApplied <= 0.0f)
	{
		return;
	}
	const int32 TowerTeamId = Tower->GetTeamId();
	const int32 ScoringTeam = 1 - TowerTeamId;
	if (ScoringTeam == 0 || ScoringTeam == 1)
	{
		TeamPoints[ScoringTeam] += UArchonMatchPolicyLibrary::ComputeTowerDamagePoints(DamageResult.DamageApplied, Config);
	}

	UArchonCombatHealthComponent* TowerHealth = Tower->GetHealth();
	RecordBaseAttack(
		TowerTeamId,
		TEXT("TOWER"),
		Tower->GetActorLocation(),
		TowerHealth ? TowerHealth->GetLastAcceptedShotOrigin() : FVector::ZeroVector,
		TowerHealth ? TowerHealth->GetHealthFraction() : 0.0f);
}

void AArchonMatchStateActor::RecordBaseAttack(
	int32 DefendingTeam,
	FName StructureLabel,
	const FVector& StructureLocation,
	const FVector& ShotOrigin,
	float HealthFraction)
{
	UWorld* World = GetWorld();
	if (!World || (DefendingTeam != 0 && DefendingTeam != 1))
	{
		return;
	}
	const double Now = World->GetTimeSeconds();

	FArchonBaseAttackEvent Event;
	Event.DefendingTeam = DefendingTeam;
	Event.StructureLabel = StructureLabel;
	Event.StructureLocation = StructureLocation;
	Event.ShotOrigin = ShotOrigin;
	Event.HealthFraction = HealthFraction;
	Event.EventTimeSeconds = Now;
	RecentBaseAttacks.Add(Event);

	// Prune the ring on write; the window only needs to outlive the
	// HUD/defense read horizon.
	RecentBaseAttacks.RemoveAll([Now](const FArchonBaseAttackEvent& Old)
	{
		return Now - Old.EventTimeSeconds > 12.0;
	});
}

bool AArchonMatchStateActor::GetLatestBaseAttack(int32 DefendingTeam, double MaxAgeSeconds, FArchonBaseAttackEvent& OutEvent) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}
	const double Now = World->GetTimeSeconds();
	for (int32 Index = RecentBaseAttacks.Num() - 1; Index >= 0; --Index)
	{
		const FArchonBaseAttackEvent& Event = RecentBaseAttacks[Index];
		if (Event.DefendingTeam == DefendingTeam && Now - Event.EventTimeSeconds <= MaxAgeSeconds)
		{
			OutEvent = Event;
			return true;
		}
	}
	return false;
}
