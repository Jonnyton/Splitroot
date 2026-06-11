#include "ArchonMapTableInteractorComponent.h"
#include "ArchonMapTableActor.h"

UArchonMapTableInteractorComponent::UArchonMapTableInteractorComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UArchonMapTableInteractorComponent::ConfigureInteractor(const FArchonMapTableInteractorConfig& InConfig)
{
	Config = InConfig;
}

bool UArchonMapTableInteractorComponent::PreviewMapTable(
	AArchonMapTableActor* MapTable,
	FArchonMapTableInteractionResult& OutResult) const
{
	OutResult = FArchonMapTableInteractionResult();

	if (!MapTable)
	{
		OutResult.Reason = TEXT("missing_map_table");
		return false;
	}

	const FArchonMapTableCommandContext Context = BuildContextForTable(MapTable);
	OutResult.MapTableDecision = MapTable->EvaluateCommand(EArchonMapTableCommandKind::Preview, Context);
	OutResult.bOpenedTableSurface = OutResult.MapTableDecision.bCanPreview;
	OutResult.bPreviewOnly = OutResult.bOpenedTableSurface;
	OutResult.Reason = OutResult.MapTableDecision.Reason;
	return OutResult.bOpenedTableSurface;
}

bool UArchonMapTableInteractorComponent::SubmitRtsOrder(
	AArchonMapTableActor* MapTable,
	FArchonRtsCommandIntent Intent,
	FArchonMapTableInteractionResult& OutResult) const
{
	OutResult = FArchonMapTableInteractionResult();

	if (!MapTable)
	{
		OutResult.Reason = TEXT("missing_map_table");
		return false;
	}

	FillIntentDefaults(Intent);

	const FArchonMapTableCommandContext Context = BuildContextForTable(MapTable);
	OutResult.MapTableDecision = MapTable->EvaluateCommand(EArchonMapTableCommandKind::ExecuteOrder, Context);
	OutResult.bOpenedTableSurface = OutResult.MapTableDecision.bCanPreview;

	if (!OutResult.MapTableDecision.bCanPreview)
	{
		OutResult.bPreviewOnly = false;
		OutResult.Reason = OutResult.MapTableDecision.Reason;
		return false;
	}

	OutResult.bSubmittedOrder = MapTable->SubmitCommand(
		EArchonMapTableCommandKind::ExecuteOrder,
		Context,
		Intent,
		OutResult.AuthorityDecision);

	OutResult.bPreviewOnly = !OutResult.bSubmittedOrder;
	OutResult.Reason = OutResult.bSubmittedOrder ? OutResult.AuthorityDecision.Reason : OutResult.AuthorityDecision.Reason;
	return OutResult.bSubmittedOrder;
}

FArchonMapTableCommandContext UArchonMapTableInteractorComponent::BuildContextForTable(const AArchonMapTableActor* MapTable) const
{
	FArchonMapTableCommandContext Context;
	Context.bAtValidMapTable = MapTable != nullptr;
	Context.bSameTeam = MapTable && MapTable->GetTeamId() == Config.TeamId;
	Context.bHasResources = true;
	Context.bLegalTarget = true;
	return Context;
}

void UArchonMapTableInteractorComponent::FillIntentDefaults(FArchonRtsCommandIntent& Intent) const
{
	if (Intent.TeamId == INDEX_NONE)
	{
		Intent.TeamId = Config.TeamId;
	}

	if (Intent.IssuingPlayerId == INDEX_NONE)
	{
		Intent.IssuingPlayerId = Config.PlayerId;
	}
}
