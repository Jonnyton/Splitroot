#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ArchonMapTableTypes.h"
#include "ArchonSessionTypes.h"
#include "ArchonTeamRtsTypes.h"
#include "ArchonMapTableActor.generated.h"

class UArchonMapTableMiniatureComponent;
class UArchonTeamRtsStateComponent;
class UArchonTeamVisibilityStateComponent;
class UStaticMeshComponent;
class UTextRenderComponent;

UCLASS(Blueprintable)
class ARCHONFACTORYCANARY_API AArchonMapTableActor : public AActor
{
	GENERATED_BODY()

public:
	AArchonMapTableActor();

	UFUNCTION(BlueprintCallable, Category = "Archon|MapTable")
	void ConfigureMapTable(int32 InTeamId, EArchonSessionRoute InSessionRoute, bool bInOwnsRtsExecutionExpansion);

	UFUNCTION(BlueprintCallable, Category = "Archon|MapTable")
	void AttachTeamState(UArchonTeamRtsStateComponent* InTeamState);

	UFUNCTION(BlueprintCallable, Category = "Archon|MapTable")
	void AttachTeamVisibilityState(UArchonTeamVisibilityStateComponent* InTeamVisibilityState);

	UFUNCTION(BlueprintPure, Category = "Archon|MapTable")
	int32 GetTeamId() const { return TeamId; }

	UFUNCTION(BlueprintCallable, Category = "Archon|MapTable")
	void SetRuntimeStatusText(const FString& InStatusText);

	UFUNCTION(BlueprintPure, Category = "Archon|MapTable")
	FString GetRuntimeStatusText() const { return RuntimeStatusText; }

	UFUNCTION(BlueprintPure, Category = "Archon|MapTable")
	int32 GetCurrentCommandSequence() const;

	UFUNCTION(BlueprintPure, Category = "Archon|MapTable")
	UArchonTeamRtsStateComponent* GetTeamState() const { return TeamState; }

	UFUNCTION(BlueprintPure, Category = "Archon|MapTable")
	UArchonTeamVisibilityStateComponent* GetTeamVisibilityState() const { return TeamVisibilityState; }

	UFUNCTION(BlueprintCallable, Category = "Archon|MapTable")
	FArchonMapTableDecision EvaluateCommand(EArchonMapTableCommandKind CommandKind, const FArchonMapTableCommandContext& Context) const;

	UFUNCTION(BlueprintCallable, Category = "Archon|MapTable")
	bool SubmitCommand(
		EArchonMapTableCommandKind CommandKind,
		const FArchonMapTableCommandContext& Context,
		const FArchonRtsCommandIntent& Intent,
		FArchonRtsAuthorityDecision& OutDecision) const;

private:
	UPROPERTY(VisibleAnywhere, Category = "Archon|MapTable")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, Category = "Archon|MapTable")
	TObjectPtr<UStaticMeshComponent> TableMesh;

	UPROPERTY(VisibleAnywhere, Category = "Archon|MapTable")
	TObjectPtr<UTextRenderComponent> HeaderText;

	UPROPERTY(VisibleAnywhere, Category = "Archon|MapTable")
	TObjectPtr<UTextRenderComponent> StatusText;

	UPROPERTY(VisibleAnywhere, Category = "Archon|MapTable")
	TObjectPtr<UArchonTeamRtsStateComponent> OwnedTeamState;

	UPROPERTY(VisibleAnywhere, Category = "Archon|MapTable")
	TObjectPtr<UArchonTeamVisibilityStateComponent> OwnedTeamVisibilityState;

	UPROPERTY(VisibleAnywhere, Category = "Archon|MapTable")
	TObjectPtr<UArchonMapTableMiniatureComponent> Miniature;

	UPROPERTY(EditAnywhere, Category = "Archon|MapTable")
	int32 TeamId = INDEX_NONE;

	UPROPERTY(EditAnywhere, Category = "Archon|MapTable")
	EArchonSessionRoute SessionRoute = EArchonSessionRoute::SteamOnline;

	UPROPERTY(EditAnywhere, Category = "Archon|MapTable")
	bool bOwnsRtsExecutionExpansion = false;

	UPROPERTY()
	TObjectPtr<UArchonTeamRtsStateComponent> TeamState;

	UPROPERTY()
	TObjectPtr<UArchonTeamVisibilityStateComponent> TeamVisibilityState;

	UPROPERTY()
	FString RuntimeStatusText;
};
