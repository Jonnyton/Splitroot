#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ArchonTeamRtsTypes.h"
#include "ArchonCanaryRtsSquadActor.generated.h"

class UArchonTeamRtsStateComponent;
class UStaticMeshComponent;
class UTextRenderComponent;

UENUM(BlueprintType)
enum class EArchonCanaryRtsSquadOrderState : uint8
{
	Idle UMETA(DisplayName = "Idle"),
	Moving UMETA(DisplayName = "Moving"),
	Attacking UMETA(DisplayName = "Attacking")
};

UCLASS(Blueprintable)
class ARCHONFACTORYCANARY_API AArchonCanaryRtsSquadActor : public AActor
{
	GENERATED_BODY()

public:
	AArchonCanaryRtsSquadActor();

	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintCallable, Category = "Archon|RTS")
	void ConfigureSquad(int32 InTeamId, FName InSquadId);

	UFUNCTION(BlueprintCallable, Category = "Archon|RTS")
	void AttachTeamState(UArchonTeamRtsStateComponent* InTeamState);

	UFUNCTION(BlueprintCallable, Category = "Archon|RTS")
	bool ApplyAcceptedCommand(const FArchonRtsCommandIntent& Intent, const FArchonRtsAuthorityDecision& Decision);

	UFUNCTION(BlueprintPure, Category = "Archon|RTS")
	int32 GetTeamId() const { return TeamId; }

	UFUNCTION(BlueprintPure, Category = "Archon|RTS")
	FName GetSquadId() const { return SquadId; }

	UFUNCTION(BlueprintPure, Category = "Archon|RTS")
	int32 GetLastAppliedCommandSequence() const { return LastAppliedCommandSequence; }

	UFUNCTION(BlueprintPure, Category = "Archon|RTS")
	EArchonCanaryRtsSquadOrderState GetOrderState() const { return OrderState; }

	UFUNCTION(BlueprintPure, Category = "Archon|RTS")
	FVector GetActiveDestination() const { return ActiveDestination; }

	UFUNCTION(BlueprintPure, Category = "Archon|RTS")
	FString GetRuntimeStatusText() const { return RuntimeStatusText; }

private:
	void HandleAcceptedCommand(const FArchonRtsCommandIntent& Intent, const FArchonRtsAuthorityDecision& Decision);
	void SetRuntimeStatusText(const FString& InStatusText);
	void AdvanceTowardDestination(float DeltaSeconds);

	UPROPERTY(VisibleAnywhere, Category = "Archon|RTS")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, Category = "Archon|RTS")
	TObjectPtr<UStaticMeshComponent> SquadMesh;

	UPROPERTY(VisibleAnywhere, Category = "Archon|RTS")
	TObjectPtr<UTextRenderComponent> StatusText;

	UPROPERTY()
	TObjectPtr<UArchonTeamRtsStateComponent> TeamState;

	UPROPERTY(EditAnywhere, Category = "Archon|RTS")
	int32 TeamId = INDEX_NONE;

	UPROPERTY(EditAnywhere, Category = "Archon|RTS")
	FName SquadId = TEXT("canary_squad");

	UPROPERTY(EditAnywhere, Category = "Archon|RTS")
	FVector RallyDestination = FVector(850.0f, -260.0f, 120.0f);

	UPROPERTY(EditAnywhere, Category = "Archon|RTS")
	FVector AttackDestination = FVector(850.0f, 260.0f, 120.0f);

	UPROPERTY(EditAnywhere, Category = "Archon|RTS")
	float MovementSpeed = 260.0f;

	UPROPERTY()
	FVector ActiveDestination = FVector::ZeroVector;

	UPROPERTY()
	FString RuntimeStatusText;

	UPROPERTY()
	EArchonCanaryRtsSquadOrderState OrderState = EArchonCanaryRtsSquadOrderState::Idle;

	UPROPERTY()
	int32 LastAppliedCommandSequence = 0;
};
