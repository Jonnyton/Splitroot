#pragma once

#include "CoreMinimal.h"

namespace ArchonBotFeatureTelemetry
{
	void ResetCoverage();
	void LogUse(int32 BotIndex, int32 TeamId, FName Feature, FName Decision, FName Result, FName Reason);
}
