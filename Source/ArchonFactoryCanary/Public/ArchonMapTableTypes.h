#pragma once

#include "CoreMinimal.h"
#include "ArchonMapTableTypes.generated.h"

UENUM(BlueprintType)
enum class EArchonMapTableCommandKind : uint8
{
	Inspect UMETA(DisplayName = "Inspect"),
	Select UMETA(DisplayName = "Select"),
	Preview UMETA(DisplayName = "Preview"),
	ExecuteOrder UMETA(DisplayName = "Execute Order")
};

USTRUCT(BlueprintType)
struct FArchonMapTableCommandContext
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "Archon|MapTable")
	bool bAtValidMapTable = true;

	UPROPERTY(BlueprintReadWrite, Category = "Archon|MapTable")
	bool bSameTeam = true;

	UPROPERTY(BlueprintReadWrite, Category = "Archon|MapTable")
	bool bHasResources = true;

	UPROPERTY(BlueprintReadWrite, Category = "Archon|MapTable")
	bool bLegalTarget = true;
};

USTRUCT(BlueprintType)
struct FArchonMapTableDecision
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Archon|MapTable")
	bool bCanInspect = false;

	UPROPERTY(BlueprintReadOnly, Category = "Archon|MapTable")
	bool bCanSelect = false;

	UPROPERTY(BlueprintReadOnly, Category = "Archon|MapTable")
	bool bCanPreview = false;

	UPROPERTY(BlueprintReadOnly, Category = "Archon|MapTable")
	bool bWillExecute = false;

	UPROPERTY(BlueprintReadOnly, Category = "Archon|MapTable")
	bool bExplicitNoOp = false;

	UPROPERTY(BlueprintReadOnly, Category = "Archon|MapTable")
	FName Reason;
};
