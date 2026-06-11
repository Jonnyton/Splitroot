#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ArchonMapTableTypes.h"
#include "ArchonTeamRtsTypes.h"
#include "ArchonTeamRtsPolicyLibrary.generated.h"

UCLASS()
class ARCHONFACTORYCANARY_API UArchonTeamRtsPolicyLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Archon|RTS")
	static FArchonRtsAuthorityDecision FinalizeTeamCommand(
		const FArchonMapTableDecision& MapTableDecision,
		const FArchonRtsCommandIntent& Intent,
		int32 CurrentTeamCommandSequence);
};
