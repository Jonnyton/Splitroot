#pragma once

#include "CoreMinimal.h"
#include "ArchonMapTableTypes.h"
#include "ArchonTeamRtsTypes.h"
#include "ArchonMapTableInteractionTypes.generated.h"

USTRUCT(BlueprintType)
struct FArchonMapTableInteractorConfig
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Archon|Interaction")
	int32 PlayerId = INDEX_NONE;

	UPROPERTY(BlueprintReadWrite, Category = "Archon|Interaction")
	int32 TeamId = INDEX_NONE;
};

USTRUCT(BlueprintType)
struct FArchonMapTableInteractionResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Archon|Interaction")
	bool bOpenedTableSurface = false;

	UPROPERTY(BlueprintReadOnly, Category = "Archon|Interaction")
	bool bSubmittedOrder = false;

	UPROPERTY(BlueprintReadOnly, Category = "Archon|Interaction")
	bool bPreviewOnly = false;

	UPROPERTY(BlueprintReadOnly, Category = "Archon|Interaction")
	FArchonMapTableDecision MapTableDecision;

	UPROPERTY(BlueprintReadOnly, Category = "Archon|Interaction")
	FArchonRtsAuthorityDecision AuthorityDecision;

	UPROPERTY(BlueprintReadOnly, Category = "Archon|Interaction")
	FName Reason;
};
