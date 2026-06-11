#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ArchonSessionTypes.h"
#include "ArchonSessionPolicyLibrary.generated.h"

UCLASS()
class ARCHONFACTORYCANARY_API UArchonSessionPolicyLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Archon|Session")
	static EArchonSessionRoute ResolveRouteFromName(const FString& RouteName);

	UFUNCTION(BlueprintPure, Category = "Archon|Session")
	static EArchonSessionRoute ResolveAmbiguousOnlineFallback();

	UFUNCTION(BlueprintPure, Category = "Archon|Entitlements")
	static bool IsFullFreeRoute(EArchonSessionRoute Route);

	UFUNCTION(BlueprintPure, Category = "Archon|Entitlements")
	static FArchonEffectiveAccess MakeEffectiveAccess(
		EArchonSessionRoute Route,
		bool bOwnsHorizontalHeroExpansion,
		bool bOwnsRtsExecutionExpansion);

	UFUNCTION(BlueprintPure, Category = "Archon|Entitlements")
	static bool CanExecuteLiveRtsCommands(EArchonSessionRoute Route, bool bOwnsRtsExecutionExpansion);

	UFUNCTION(BlueprintPure, Category = "Archon|Entitlements")
	static bool IsMapTablePreviewOnly(EArchonSessionRoute Route, bool bOwnsRtsExecutionExpansion);

	UFUNCTION(BlueprintPure, Category = "Archon|Entitlements")
	static bool CanSpawnHero(EArchonSessionRoute Route, const FArchonHeroEntitlement& Hero, bool bOwnsHorizontalHeroExpansion);
};
