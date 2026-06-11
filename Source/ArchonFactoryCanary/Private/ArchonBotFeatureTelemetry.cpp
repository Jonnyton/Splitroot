#include "ArchonBotFeatureTelemetry.h"

namespace
{
	struct FArchonBotFeatureCoverageCounters
	{
		int32 Attempted = 0;
		int32 Succeeded = 0;
		int32 Skipped = 0;
		int32 Unavailable = 0;
	};

	TMap<FName, FArchonBotFeatureCoverageCounters>& Coverage()
	{
		static TMap<FName, FArchonBotFeatureCoverageCounters> Counters;
		return Counters;
	}
}

void ArchonBotFeatureTelemetry::ResetCoverage()
{
	Coverage().Reset();
}

void ArchonBotFeatureTelemetry::LogUse(int32 BotIndex, int32 TeamId, FName Feature, FName Decision, FName Result, FName Reason)
{
	if (Feature.IsNone() || Decision.IsNone() || Result.IsNone() || Reason.IsNone())
	{
		return;
	}

	FArchonBotFeatureCoverageCounters& Counters = Coverage().FindOrAdd(Feature);
	if (Decision == TEXT("attempt"))
	{
		++Counters.Attempted;
	}
	if (Result == TEXT("success"))
	{
		++Counters.Succeeded;
	}
	else if (Result == TEXT("skipped"))
	{
		++Counters.Skipped;
	}
	else if (Result == TEXT("unavailable"))
	{
		++Counters.Unavailable;
	}

	UE_LOG(
		LogTemp,
		Display,
		TEXT("ArchonFactoryCanary: BotFeatureUse bot=%d team=%d feature=%s decision=%s result=%s reason=%s"),
		BotIndex,
		TeamId,
		*Feature.ToString(),
		*Decision.ToString(),
		*Result.ToString(),
		*Reason.ToString());
	UE_LOG(
		LogTemp,
		Display,
		TEXT("ArchonFactoryCanary: BotFeatureCoverage feature=%s attempted=%d succeeded=%d skipped=%d unavailable=%d"),
		*Feature.ToString(),
		Counters.Attempted,
		Counters.Succeeded,
		Counters.Skipped,
		Counters.Unavailable);
}
