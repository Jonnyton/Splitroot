#include "ArchonMainMenuController.h"
#include "ArchonCanaryWorldSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

TArray<FArchonMapEntry> UArchonMainMenuController::GetHostableMaps()
{
	return UArchonMapRegistryLibrary::GetSplitrootMapRotation();
}

bool UArchonMainMenuController::HostMap(FName MapId, const TArray<FString>& ExtraOptions)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogTemp, Error, TEXT("ArchonFactoryCanary: MainMenuHost map=%s failed=no_world"), *MapId.ToString());
		return false;
	}

	FArchonMapEntry Entry;
	if (!UArchonMapRegistryLibrary::FindMapById(GetHostableMaps(), MapId, Entry))
	{
		UE_LOG(LogTemp, Error, TEXT("ArchonFactoryCanary: MainMenuHost map=%s failed=unknown_map"), *MapId.ToString());
		return false;
	}

	// Hosting is always a listen server so a friend can join mid-match
	// (Renegade drop-in pattern); solo play just never gets a second login.
	TArray<FString> Options = ExtraOptions;
	Options.Add(TEXT("listen"));
	const FString TravelUrl = UArchonMapRegistryLibrary::BuildTravelUrl(Entry, Options);
	UE_LOG(LogTemp, Display, TEXT("ArchonFactoryCanary: MainMenuHost map=%s url=%s"), *MapId.ToString(), *TravelUrl);
	if (UArchonCanaryWorldSubsystem* Subsystem = World->GetSubsystem<UArchonCanaryWorldSubsystem>())
	{
		Subsystem->NotifyPlaytestEvent(TEXT("menu_host"));
	}
	World->ServerTravel(TravelUrl, /*bAbsolute=*/true);
	return true;
}

bool UArchonMainMenuController::JoinByAddress(const FString& Address)
{
	const FString Trimmed = Address.TrimStartAndEnd();
	UWorld* World = GetWorld();
	APlayerController* PlayerController = World ? World->GetFirstPlayerController() : nullptr;
	if (Trimmed.IsEmpty() || !PlayerController)
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("ArchonFactoryCanary: MainMenuJoin address=%s failed=%s"),
			*Trimmed,
			Trimmed.IsEmpty() ? TEXT("empty_address") : TEXT("no_player_controller"));
		return false;
	}

	UE_LOG(LogTemp, Display, TEXT("ArchonFactoryCanary: MainMenuJoin address=%s"), *Trimmed);
	if (UArchonCanaryWorldSubsystem* Subsystem = World->GetSubsystem<UArchonCanaryWorldSubsystem>())
	{
		Subsystem->NotifyPlaytestEvent(TEXT("menu_join"));
	}
	PlayerController->ClientTravel(Trimmed, TRAVEL_Absolute);
	return true;
}

void UArchonMainMenuController::QuitGame()
{
	UWorld* World = GetWorld();
	if (World && GEngine)
	{
		UE_LOG(LogTemp, Display, TEXT("ArchonFactoryCanary: MainMenuQuit"));
		GEngine->Exec(World, TEXT("quit"));
	}
}
