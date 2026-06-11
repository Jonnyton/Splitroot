#include "ArchonRenownPolicyLibrary.h"

FArchonRenownTickResult UArchonRenownPolicyLibrary::EvaluateTrickle(
	float AccumulatorSeconds,
	float DeltaSeconds,
	float IntervalSeconds,
	int32 TricklePerInterval)
{
	FArchonRenownTickResult Result;
	if (IntervalSeconds <= 0.0f || TricklePerInterval <= 0)
	{
		Result.NewAccumulatorSeconds = 0.0f;
		return Result;
	}

	float Accumulated = AccumulatorSeconds + FMath::Max(0.0f, DeltaSeconds);
	while (Accumulated >= IntervalSeconds)
	{
		Accumulated -= IntervalSeconds;
		Result.Payout += TricklePerInterval;
	}
	Result.NewAccumulatorSeconds = Accumulated;
	return Result;
}

int32 UArchonRenownPolicyLibrary::GetKillBounty()
{
	// Renegade bounties scaled with target value; v1 is flat and tuned
	// against the 5-trickle: one kill ≈ one minute of passive income.
	return 30;
}

int32 UArchonRenownPolicyLibrary::GetHarvestPayout()
{
	// The team-wide heartbeat payment (Renegade harvester dump ~300 at
	// 2002 prices; ours tuned to our smaller price ladder later).
	return 50;
}
