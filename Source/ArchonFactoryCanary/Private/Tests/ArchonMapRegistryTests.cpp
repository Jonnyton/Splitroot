#include "ArchonLaunchFlagLibrary.h"
#include "ArchonMapRegistryLibrary.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonMapRegistryRotationTest,
	"ArchonFactory.Maps.RotationOrderAndTravelUrls",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FArchonMapRegistryRotationTest::RunTest(const FString& Parameters)
{
	const TArray<FArchonMapEntry> Rotation = UArchonMapRegistryLibrary::GetSplitrootMapRotation();
	TestTrue(TEXT("Rotation has at least two maps (switching is testable)"), Rotation.Num() >= 2);
	TestEqual(TEXT("Valley leads the rotation"), Rotation[0].MapId, FName(TEXT("splitroot_valley")));

	FArchonMapEntry Valley;
	TestTrue(TEXT("Valley findable by id"), UArchonMapRegistryLibrary::FindMapById(Rotation, TEXT("splitroot_valley"), Valley));
	TestTrue(TEXT("Valley carries the valley flag"), Valley.UrlOptions.Contains(TEXT("ArchonSplitrootValley")));
	TestTrue(TEXT("Valley carries the match loop flag"), Valley.UrlOptions.Contains(TEXT("ArchonMatchLoop")));

	TestTrue(
		TEXT("Valley selects the match game mode"),
		Valley.UrlOptions.Contains(TEXT("game=/Script/ArchonFactoryCanary.ArchonMatchGameMode")));

	const FString ValleyUrl = UArchonMapRegistryLibrary::BuildTravelUrl(Valley, {});
	TestEqual(
		TEXT("Valley travel URL"),
		ValleyUrl,
		FString(TEXT("/Engine/Maps/Entry?ArchonSplitrootValley?ArchonMatchLoop?game=/Script/ArchonFactoryCanary.ArchonMatchGameMode?ArchonMapId=splitroot_valley")));

	const FString ValleyProofUrl = UArchonMapRegistryLibrary::BuildTravelUrl(Valley, { TEXT("ArchonMatchProofQuit") });
	TestTrue(TEXT("Extra options append"), ValleyProofUrl.EndsWith(TEXT("?ArchonMatchProofQuit")));

	// Rotation advances and wraps.
	TestEqual(
		TEXT("After valley comes the range"),
		UArchonMapRegistryLibrary::GetNextMapInRotation(Rotation, TEXT("splitroot_valley")).MapId,
		FName(TEXT("canary_range")));
	TestEqual(
		TEXT("Rotation wraps to valley"),
		UArchonMapRegistryLibrary::GetNextMapInRotation(Rotation, TEXT("canary_range")).MapId,
		FName(TEXT("splitroot_valley")));
	TestEqual(
		TEXT("Unknown current id falls back to the first entry"),
		UArchonMapRegistryLibrary::GetNextMapInRotation(Rotation, NAME_None).MapId,
		FName(TEXT("splitroot_valley")));
	TestEqual(
		TEXT("Live match rotation skips the non-match template and loops valley"),
		UArchonMapRegistryLibrary::GetNextMatchMapInRotation(Rotation, TEXT("splitroot_valley")).MapId,
		FName(TEXT("splitroot_valley")));
	TestEqual(
		TEXT("Unknown match rotation starts on the first match map"),
		UArchonMapRegistryLibrary::GetNextMatchMapInRotation(Rotation, NAME_None).MapId,
		FName(TEXT("splitroot_valley")));

	TArray<FArchonMapEntry> TwoMatchMaps = Rotation;
	FArchonMapEntry SecondMatch;
	SecondMatch.MapId = TEXT("second_match");
	SecondMatch.DisplayName = TEXT("Second Match");
	SecondMatch.MapUrl = TEXT("/Engine/Maps/Entry");
	SecondMatch.UrlOptions = {
		TEXT("ArchonSplitrootValley"),
		TEXT("ArchonMatchLoop"),
		TEXT("game=/Script/ArchonFactoryCanary.ArchonMatchGameMode") };
	TwoMatchMaps.Add(SecondMatch);
	TestEqual(
		TEXT("Live match rotation advances when another match map exists"),
		UArchonMapRegistryLibrary::GetNextMatchMapInRotation(TwoMatchMaps, TEXT("splitroot_valley")).MapId,
		FName(TEXT("second_match")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonMapRegistryJsonLoadTest,
	"ArchonFactory.Maps.RegistryJsonLoadFeedsRotation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FArchonMapRegistryJsonLoadTest::RunTest(const FString& Parameters)
{
	const FString RegistryPath = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Proof"), TEXT("test-map-registry.json"));
	IFileManager::Get().MakeDirectory(*FPaths::GetPath(RegistryPath), true);

	const FString JsonText = TEXT(R"JSON(
{
  "schema": "tinyassets.game_factory.map_registry.v1",
  "maps": [
    {
      "map_id": "splitroot_valley",
      "display_name": "Splitroot Valley",
      "map_url": "/Engine/Maps/Entry",
      "url_options": ["ArchonSplitrootValley", "ArchonMatchLoop", "game=/Script/ArchonFactoryCanary.ArchonMatchGameMode"],
      "shipped": true
    },
    {
      "map_id": "quarry_pass",
      "display_name": "Quarry Pass",
      "map_url": "/Engine/Maps/Entry",
      "url_options": ["ArchonSplitrootValley", "ArchonMatchLoop", "game=/Script/ArchonFactoryCanary.ArchonMatchGameMode"],
      "shipped": true
    },
    {
      "map_id": "unshipped_lab",
      "display_name": "Unshipped Lab",
      "map_url": "/Engine/Maps/Entry",
      "url_options": ["ArchonMatchLoop"],
      "shipped": false
    }
  ],
  "rotation_order": ["splitroot_valley", "unshipped_lab", "quarry_pass"]
}
)JSON");
	TestTrue(TEXT("Temp registry saved"), FFileHelper::SaveStringToFile(JsonText, *RegistryPath));

	TArray<FArchonMapEntry> Rotation;
	FString Error;
	TestTrue(TEXT("Registry loads"), UArchonMapRegistryLibrary::LoadMapRotationFromFile(RegistryPath, Rotation, Error));
	TestEqual(TEXT("Only shipped maps enter rotation"), Rotation.Num(), 2);
	TestEqual(TEXT("First rotation map"), Rotation[0].MapId, FName(TEXT("splitroot_valley")));
	TestEqual(TEXT("Second rotation map comes from registry file"), Rotation[1].MapId, FName(TEXT("quarry_pass")));
	TestTrue(TEXT("Registry match map carries loop flag"), Rotation[1].UrlOptions.Contains(TEXT("ArchonMatchLoop")));
	TestEqual(
		TEXT("Next match map can advance to file-authored map"),
		UArchonMapRegistryLibrary::GetNextMatchMapInRotation(Rotation, TEXT("splitroot_valley")).MapId,
		FName(TEXT("quarry_pass")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonLaunchFlagUrlOptionTest,
	"ArchonFactory.Maps.LaunchFlagsReadTravelUrlOptions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FArchonLaunchFlagUrlOptionTest::RunTest(const FString& Parameters)
{
	FURL Url;
	Url.AddOption(TEXT("ArchonSplitrootValley"));
	Url.AddOption(TEXT("ArchonMapId=splitroot_valley"));

	TestTrue(
		TEXT("Plain option found"),
		UArchonLaunchFlagLibrary::HasUrlOption(Url, TEXT("ArchonSplitrootValley")));
	TestTrue(
		TEXT("Valued option found by key"),
		UArchonLaunchFlagLibrary::HasUrlOption(Url, TEXT("ArchonMapId=")));
	TestTrue(
		TEXT("Absent option not found"),
		!UArchonLaunchFlagLibrary::HasUrlOption(Url, TEXT("ArchonMatchLoop")));

	// Travel URLs built by the registry parse back into findable options.
	const TArray<FArchonMapEntry> Rotation = UArchonMapRegistryLibrary::GetSplitrootMapRotation();
	FArchonMapEntry Valley;
	UArchonMapRegistryLibrary::FindMapById(Rotation, TEXT("splitroot_valley"), Valley);
	FURL TravelUrl(nullptr, *UArchonMapRegistryLibrary::BuildTravelUrl(Valley, { TEXT("ArchonMatchProofQuit") }), TRAVEL_Absolute);
	TestTrue(
		TEXT("Built URL: valley flag round-trips"),
		UArchonLaunchFlagLibrary::HasUrlOption(TravelUrl, TEXT("ArchonSplitrootValley")));
	TestTrue(
		TEXT("Built URL: match loop flag round-trips"),
		UArchonLaunchFlagLibrary::HasUrlOption(TravelUrl, TEXT("ArchonMatchLoop")));
	TestTrue(
		TEXT("Built URL: extra option round-trips"),
		UArchonLaunchFlagLibrary::HasUrlOption(TravelUrl, TEXT("ArchonMatchProofQuit")));

	return true;
}
