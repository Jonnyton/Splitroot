#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ArchonMapTableTypes.h"
#include "ArchonTeamRtsTypes.h"
#include "ArchonTeamRtsStateComponent.generated.h"

DECLARE_MULTICAST_DELEGATE_TwoParams(
	FArchonRtsCommandAcceptedNative,
	const FArchonRtsCommandIntent&,
	const FArchonRtsAuthorityDecision&);

UCLASS(ClassGroup=(Archon), Blueprintable, meta=(BlueprintSpawnableComponent))
class ARCHONFACTORYCANARY_API UArchonTeamRtsStateComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UArchonTeamRtsStateComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintCallable, Category = "Archon|RTS")
	void ConfigureTeamState(int32 InTeamId);

	UFUNCTION(BlueprintCallable, Category = "Archon|RTS")
	bool SubmitMapTableCommand(
		const FArchonMapTableDecision& MapTableDecision,
		const FArchonRtsCommandIntent& Intent,
		FArchonRtsAuthorityDecision& OutDecision);

	UFUNCTION(BlueprintCallable, Category = "Archon|RTS")
	void ResetProofState();

	UFUNCTION(BlueprintPure, Category = "Archon|RTS")
	int32 GetTeamId() const { return TeamId; }

	UFUNCTION(BlueprintPure, Category = "Archon|RTS")
	int32 GetCurrentCommandSequence() const { return CurrentCommandSequence; }

	UFUNCTION(BlueprintPure, Category = "Archon|RTS")
	FName GetLastAcceptedReason() const { return LastAcceptedReason; }

	UFUNCTION(BlueprintPure, Category = "Archon|RTS")
	FName GetLastRejectedReason() const { return LastRejectedReason; }

	UFUNCTION(BlueprintPure, Category = "Archon|RTS")
	FArchonRtsCommandIntent GetLastAcceptedCommandIntent() const { return LastAcceptedCommandIntent; }

	UFUNCTION(BlueprintPure, Category = "Archon|RTS")
	bool HasTeamRallyPoint() const { return bHasTeamRallyPoint; }

	UFUNCTION(BlueprintPure, Category = "Archon|RTS")
	FVector GetTeamRallyPoint() const { return TeamRallyPoint; }

	FArchonRtsCommandAcceptedNative OnCommandAccepted;

private:
	UPROPERTY(Replicated)
	int32 TeamId = INDEX_NONE;

	UPROPERTY(Replicated)
	int32 CurrentCommandSequence = 0;

	UPROPERTY(Replicated)
	FName LastAcceptedReason;

	UPROPERTY(Replicated)
	FName LastRejectedReason;

	UPROPERTY(Replicated)
	FArchonRtsCommandIntent LastAcceptedCommandIntent;

	UPROPERTY(Replicated)
	FVector TeamRallyPoint = FVector::ZeroVector;

	UPROPERTY(Replicated)
	bool bHasTeamRallyPoint = false;
};
