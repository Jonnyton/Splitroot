#include "ArchonTeamRtsStateComponent.h"
#include "ArchonTeamRtsPolicyLibrary.h"
#include "Net/UnrealNetwork.h"

UArchonTeamRtsStateComponent::UArchonTeamRtsStateComponent()
{
	SetIsReplicatedByDefault(true);
}

void UArchonTeamRtsStateComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UArchonTeamRtsStateComponent, TeamId);
	DOREPLIFETIME(UArchonTeamRtsStateComponent, CurrentCommandSequence);
	DOREPLIFETIME(UArchonTeamRtsStateComponent, LastAcceptedReason);
	DOREPLIFETIME(UArchonTeamRtsStateComponent, LastRejectedReason);
	DOREPLIFETIME(UArchonTeamRtsStateComponent, LastAcceptedCommandIntent);
	DOREPLIFETIME(UArchonTeamRtsStateComponent, TeamRallyPoint);
	DOREPLIFETIME(UArchonTeamRtsStateComponent, bHasTeamRallyPoint);
}

void UArchonTeamRtsStateComponent::ConfigureTeamState(int32 InTeamId)
{
	TeamId = InTeamId;
}

bool UArchonTeamRtsStateComponent::SubmitMapTableCommand(
	const FArchonMapTableDecision& MapTableDecision,
	const FArchonRtsCommandIntent& Intent,
	FArchonRtsAuthorityDecision& OutDecision)
{
	if (TeamId == INDEX_NONE)
	{
		OutDecision = FArchonRtsAuthorityDecision();
		OutDecision.PreviousSequence = CurrentCommandSequence;
		OutDecision.FinalSequence = CurrentCommandSequence;
		OutDecision.Reason = TEXT("team_state_unconfigured");
		LastRejectedReason = OutDecision.Reason;
		return false;
	}

	if (Intent.TeamId != TeamId)
	{
		OutDecision = FArchonRtsAuthorityDecision();
		OutDecision.PreviousSequence = CurrentCommandSequence;
		OutDecision.FinalSequence = CurrentCommandSequence;
		OutDecision.Reason = TEXT("wrong_team_state");
		LastRejectedReason = OutDecision.Reason;
		return false;
	}

	OutDecision = UArchonTeamRtsPolicyLibrary::FinalizeTeamCommand(MapTableDecision, Intent, CurrentCommandSequence);
	if (OutDecision.bAccepted)
	{
		CurrentCommandSequence = OutDecision.FinalSequence;
		LastAcceptedReason = OutDecision.Reason;
		LastAcceptedCommandIntent = Intent;
		if (Intent.OrderKind == EArchonRtsOrderKind::SetRallyPoint && Intent.bTargetLocationValid)
		{
			TeamRallyPoint = Intent.TargetLocation;
			bHasTeamRallyPoint = true;
			UE_LOG(
				LogTemp,
				Display,
				TEXT("ArchonFactoryCanary: TeamRallyPointSet team=%d sequence=%d target=%s location=%s"),
				TeamId,
				CurrentCommandSequence,
				*Intent.TargetId.ToString(),
				*TeamRallyPoint.ToCompactString());
		}
		OnCommandAccepted.Broadcast(Intent, OutDecision);
	}
	else
	{
		LastRejectedReason = OutDecision.Reason;
	}

	return OutDecision.bAccepted;
}

void UArchonTeamRtsStateComponent::ResetProofState()
{
	CurrentCommandSequence = 0;
	LastAcceptedReason = NAME_None;
	LastRejectedReason = NAME_None;
	LastAcceptedCommandIntent = FArchonRtsCommandIntent();
	TeamRallyPoint = FVector::ZeroVector;
	bHasTeamRallyPoint = false;
}
