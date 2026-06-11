#include "ArchonMapTablePolicyLibrary.h"
#include "ArchonSessionPolicyLibrary.h"

FArchonMapTableDecision UArchonMapTablePolicyLibrary::EvaluateMapTableCommand(
	EArchonSessionRoute Route,
	bool bOwnsRtsExecutionExpansion,
	EArchonMapTableCommandKind CommandKind,
	const FArchonMapTableCommandContext& Context)
{
	FArchonMapTableDecision Decision;

	if (!Context.bAtValidMapTable)
	{
		Decision.Reason = TEXT("invalid_map_table");
		return Decision;
	}

	if (!Context.bSameTeam)
	{
		Decision.Reason = TEXT("wrong_team");
		return Decision;
	}

	Decision.bCanInspect = true;
	Decision.bCanSelect = true;
	Decision.bCanPreview = true;

	if (CommandKind != EArchonMapTableCommandKind::ExecuteOrder)
	{
		Decision.Reason = TEXT("preview_surface");
		return Decision;
	}

	if (!Context.bHasResources)
	{
		Decision.Reason = TEXT("insufficient_resources");
		return Decision;
	}

	if (!Context.bLegalTarget)
	{
		Decision.Reason = TEXT("illegal_target");
		return Decision;
	}

	if (!UArchonSessionPolicyLibrary::CanExecuteLiveRtsCommands(Route, bOwnsRtsExecutionExpansion))
	{
		Decision.bExplicitNoOp = true;
		Decision.Reason = TEXT("steam_online_preview_no_op");
		return Decision;
	}

	Decision.bWillExecute = true;
	Decision.Reason = TEXT("execute_authoritative_order");
	return Decision;
}
