#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ArchonMapRegistryLibrary.generated.h"

/**
 * One playable map the host can pick and the rotation can advance to.
 * Runtime reads games/splitroot/FactoryContracts/map_registry.json, with
 * C++ literals retained only as a fail-closed fallback.
 */
USTRUCT(BlueprintType)
struct FArchonMapEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Archon|Maps")
	FName MapId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Archon|Maps")
	FString DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Archon|Maps")
	FString MapUrl;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Archon|Maps")
	TArray<FString> UrlOptions;
};

/**
 * Map registry + rotation policy (Renegade pattern: ordered rotation,
 * everyone auto-loads the next map, no lobby return). Pure functions;
 * the front-end menu and the match actor's Traveling phase consume them.
 */
UCLASS()
class ARCHONFACTORYCANARY_API UArchonMapRegistryLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Archon|Maps")
	static TArray<FArchonMapEntry> GetSplitrootMapRotation();

	static bool LoadMapRotationFromFile(const FString& RegistryPath, TArray<FArchonMapEntry>& OutRotation, FString& OutError);

	// Travel URL with all flag options plus ArchonMapId=<id> so the next
	// world knows its own rotation position. ExtraOptions are appended
	// verbatim (proof plumbing like ArchonMatchProofQuit).
	UFUNCTION(BlueprintPure, Category = "Archon|Maps")
	static FString BuildTravelUrl(const FArchonMapEntry& Entry, const TArray<FString>& ExtraOptions);

	UFUNCTION(BlueprintPure, Category = "Archon|Maps")
	static bool FindMapById(const TArray<FArchonMapEntry>& Rotation, FName MapId, FArchonMapEntry& OutEntry);

	// Next entry after CurrentMapId, wrapping; first entry when the
	// current id is unknown (command-line launches carry no map id).
	UFUNCTION(BlueprintPure, Category = "Archon|Maps")
	static FArchonMapEntry GetNextMapInRotation(const TArray<FArchonMapEntry>& Rotation, FName CurrentMapId);

	// Next match-playable entry after CurrentMapId, wrapping. Non-match
	// template maps stay hostable/testable without breaking live match flow.
	UFUNCTION(BlueprintPure, Category = "Archon|Maps")
	static FArchonMapEntry GetNextMatchMapInRotation(const TArray<FArchonMapEntry>& Rotation, FName CurrentMapId);
};
