#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ArchonMapTableSelectionPolicyLibrary.h"
#include "ArchonTeamRtsTypes.h"
#include "ArchonMapTableWidget.generated.h"

class AArchonMapTableActor;

UCLASS(Blueprintable)
class ARCHONFACTORYCANARY_API UArchonMapTableWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Archon|MapTable")
	void ConfigureMapTableWidget(
		AArchonMapTableActor* InMapTable,
		int32 InPlayerId,
		int32 InTeamId,
		const FVector& InWorldOrigin,
		const FVector& InWorldExtents);

	UFUNCTION(BlueprintCallable, Category = "Archon|MapTable")
	void SetSelectableSquadCandidates(const TArray<FArchonSelectableSquadCandidate>& InCandidates);

	UFUNCTION(BlueprintCallable, Category = "Archon|MapTable")
	bool CommitDragBox(const FVector2D& StartCorner, const FVector2D& EndCorner);

	UFUNCTION(BlueprintCallable, Category = "Archon|MapTable")
	bool CommitClickSelection(const FVector2D& ClickPoint, float ClickRadius);

	UFUNCTION(BlueprintCallable, Category = "Archon|MapTable")
	bool HandleRightClickOrder(const FVector2D& TableSpacePoint, FName TargetId, FArchonRtsAuthorityDecision& OutDecision);

	UFUNCTION(BlueprintCallable, Category = "Archon|MapTable")
	bool HandleRallyPointOrder(const FVector2D& TableSpacePoint, FName TargetId, FArchonRtsAuthorityDecision& OutDecision);

	UFUNCTION(BlueprintPure, Category = "Archon|MapTable")
	TArray<FName> GetSelectedSquadIds() const { return SelectedSquadIds; }

	UFUNCTION(BlueprintPure, Category = "Archon|MapTable")
	FVector GetLastOrderTargetLocation() const { return LastOrderTargetLocation; }

	UFUNCTION(BlueprintPure, Category = "Archon|MapTable")
	bool HasSubmittedOrder() const { return bHasSubmittedOrder; }

	UFUNCTION(BlueprintPure, Category = "Archon|MapTable")
	FString FormatVisibilitySummary() const;

private:
	FName ResolvePrimarySelectedSquadId() const;

	UPROPERTY()
	TObjectPtr<AArchonMapTableActor> MapTable;

	UPROPERTY()
	TArray<FArchonSelectableSquadCandidate> SelectableSquadCandidates;

	UPROPERTY()
	TArray<FName> SelectedSquadIds;

	UPROPERTY()
	int32 PlayerId = INDEX_NONE;

	UPROPERTY()
	int32 TeamId = INDEX_NONE;

	UPROPERTY()
	FVector WorldOrigin = FVector(-2000.0f, -2000.0f, 0.0f);

	UPROPERTY()
	FVector WorldExtents = FVector(24000.0f, 16000.0f, 0.0f);

	UPROPERTY()
	FVector LastOrderTargetLocation = FVector::ZeroVector;

	UPROPERTY()
	bool bHasSubmittedOrder = false;
};
