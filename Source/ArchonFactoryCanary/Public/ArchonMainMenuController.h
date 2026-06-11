#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ArchonMapRegistryLibrary.h"
#include "ArchonMainMenuController.generated.h"

/**
 * Front-end logic: host picks a map from the registry and the match
 * starts; join connects to a host address; quit exits. Pure decisions
 * live here so the headless smoke can drive the same path the Slate
 * panel's buttons call. Menu activation is URL-option-only
 * (?ArchonMainMenu) — a command-line flag would persist across the host
 * travel and respawn the menu on the gameplay map.
 */
UCLASS()
class ARCHONFACTORYCANARY_API UArchonMainMenuController : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Archon|FrontEnd")
	static TArray<FArchonMapEntry> GetHostableMaps();

	// Travels this process into the picked map with its full option set
	// (the listen-server option arrives with the networked-play slice).
	UFUNCTION(BlueprintCallable, Category = "Archon|FrontEnd")
	bool HostMap(FName MapId, const TArray<FString>& ExtraOptions);

	// Connects to a host by address (ip[:port]). Meaningful once two
	// processes exist; the menu wires it now so the join path has one
	// owner.
	UFUNCTION(BlueprintCallable, Category = "Archon|FrontEnd")
	bool JoinByAddress(const FString& Address);

	UFUNCTION(BlueprintCallable, Category = "Archon|FrontEnd")
	void QuitGame();
};
