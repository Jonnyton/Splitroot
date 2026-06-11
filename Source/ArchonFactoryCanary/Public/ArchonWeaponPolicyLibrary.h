#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ArchonWeaponTypes.h"
#include "ArchonWeaponPolicyLibrary.generated.h"

UCLASS()
class ARCHONFACTORYCANARY_API UArchonWeaponPolicyLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Archon|Weapon")
	static FArchonWeaponState MakeInitialState(const FArchonWeaponStats& Stats);

	UFUNCTION(BlueprintCallable, Category = "Archon|Weapon")
	static FArchonWeaponFireDecision EvaluateFire(
		const FArchonWeaponState& CurrentState,
		const FArchonWeaponStats& Stats,
		bool bFirePressed);

	UFUNCTION(BlueprintCallable, Category = "Archon|Weapon")
	static FArchonWeaponReloadDecision EvaluateReloadStart(
		const FArchonWeaponState& CurrentState,
		const FArchonWeaponStats& Stats,
		bool bReloadPressed);

	UFUNCTION(BlueprintCallable, Category = "Archon|Weapon")
	static FArchonWeaponState AdvanceWeaponState(
		const FArchonWeaponState& CurrentState,
		const FArchonWeaponStats& Stats,
		float DeltaSeconds,
		bool bAutoReloadOnEmpty);
};
