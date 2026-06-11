#include "ArchonTeamRtsPolicyLibrary.h"

FArchonRtsAuthorityDecision UArchonTeamRtsPolicyLibrary::FinalizeTeamCommand(
	const FArchonMapTableDecision& MapTableDecision,
	const FArchonRtsCommandIntent& Intent,
	int32 CurrentTeamCommandSequence)
{
	FArchonRtsAuthorityDecision Decision;
	Decision.PreviousSequence = CurrentTeamCommandSequence;
	Decision.FinalSequence = CurrentTeamCommandSequence;
	Decision.bRequiresCommanderToken = false;

	if (Intent.TeamId == INDEX_NONE)
	{
		Decision.Reason = TEXT("missing_team");
		return Decision;
	}

	if (Intent.IssuingPlayerId == INDEX_NONE)
	{
		Decision.Reason = TEXT("missing_player");
		return Decision;
	}

	if (MapTableDecision.bExplicitNoOp)
	{
		Decision.Reason = TEXT("map_table_no_op");
		return Decision;
	}

	if (!MapTableDecision.bWillExecute)
	{
		Decision.Reason = TEXT("map_table_not_executable");
		return Decision;
	}

	if (Intent.OrderKind == EArchonRtsOrderKind::SetRallyPoint && !Intent.bTargetLocationValid)
	{
		Decision.Reason = TEXT("rally_missing_location");
		return Decision;
	}

	Decision.bAccepted = true;
	Decision.bMutatesTeamState = true;
	Decision.bReplicatesToTeam = true;
	Decision.FinalSequence = CurrentTeamCommandSequence + 1;
	Decision.Reason = TEXT("accepted_standard_archon_order");
	return Decision;
}
