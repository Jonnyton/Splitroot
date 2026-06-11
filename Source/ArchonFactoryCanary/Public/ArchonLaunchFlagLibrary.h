#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ArchonLaunchFlagLibrary.generated.h"

/**
 * Dual-source launch flags. Proof scripts pass flags on the process
 * command line (-ArchonSplitrootValley); map rotation passes them as
 * travel-URL options (/Engine/Maps/Entry?ArchonSplitrootValley) because
 * the command line cannot change across ServerTravel. Runtime gates ask
 * this library so both launch paths behave identically.
 */
UCLASS()
class ARCHONFACTORYCANARY_API UArchonLaunchFlagLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// True when the flag is on the process command line OR among the
	// world's travel-URL options.
	static bool IsLaunchFlagActive(const UWorld* World, const TCHAR* FlagName);

	// URL-option half, separated for tests (the command-line half is
	// process-global state).
	static bool HasUrlOption(const FURL& Url, const TCHAR* FlagName);
};
