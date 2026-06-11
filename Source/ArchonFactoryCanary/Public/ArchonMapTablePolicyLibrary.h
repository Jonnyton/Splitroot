#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ArchonMapTableTypes.h"
#include "ArchonSessionTypes.h"
#include "ArchonMapTablePolicyLibrary.generated.h"

UCLASS()
class ARCHONFACTORYCANARY_API UArchonMapTablePolicyLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Archon|MapTable")
	static FArchonMapTableDecision EvaluateMapTableCommand(
		EArchonSessionRoute Route,
		bool bOwnsRtsExecutionExpansion,
		EArchonMapTableCommandKind CommandKind,
		const FArchonMapTableCommandContext& Context);
};
