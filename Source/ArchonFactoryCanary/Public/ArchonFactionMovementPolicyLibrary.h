#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ArchonFactionTypes.h"
#include "ArchonFactionMovementPolicyLibrary.generated.h"

UCLASS()
class ARCHONFACTORYCANARY_API UArchonFactionMovementPolicyLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Archon|Locomotion")
	static EArchonFactionMovementVerb GetMovementVerbForFaction(EArchonFaction Faction);

	UFUNCTION(BlueprintCallable, Category = "Archon|Locomotion")
	static FArchonFactionMovementDecision EvaluateLaunch(
		EArchonFaction Faction,
		const FArchonFactionMovementInputState& Input,
		const FArchonFactionMovementCooldown& CurrentCooldown,
		const FArchonFactionMovementTuning& Tuning);

	UFUNCTION(BlueprintCallable, Category = "Archon|Locomotion")
	static FArchonFactionMovementCooldown AdvanceCooldown(
		const FArchonFactionMovementCooldown& Current,
		float DeltaSeconds);

	UFUNCTION(BlueprintPure, Category = "Archon|Locomotion")
	static bool IsCooldownReady(const FArchonFactionMovementCooldown& Cooldown);
};
