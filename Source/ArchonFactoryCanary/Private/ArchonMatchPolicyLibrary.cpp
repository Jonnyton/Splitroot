#include "ArchonMatchPolicyLibrary.h"

FArchonMatchClock UArchonMatchPolicyLibrary::TickMatchClock(
	const FArchonMatchClock& Clock,
	float DeltaSeconds,
	const FArchonMatchConfig& Config,
	EArchonMatchWinner Winner)
{
	FArchonMatchClock Next = Clock;
	Next.PhaseSeconds += FMath::Max(0.0f, DeltaSeconds);

	switch (Clock.Phase)
	{
	case EArchonMatchPhase::Warmup:
		if (Next.PhaseSeconds >= Config.WarmupSeconds)
		{
			Next.Phase = EArchonMatchPhase::Live;
			Next.PhaseSeconds = 0.0f;
		}
		break;
	case EArchonMatchPhase::Live:
		if (Winner != EArchonMatchWinner::Undecided)
		{
			Next.Phase = EArchonMatchPhase::MatchEnd;
			Next.PhaseSeconds = 0.0f;
		}
		break;
	case EArchonMatchPhase::MatchEnd:
		if (Next.PhaseSeconds >= Config.ScoreboardSeconds)
		{
			Next.Phase = EArchonMatchPhase::Traveling;
			Next.PhaseSeconds = 0.0f;
		}
		break;
	case EArchonMatchPhase::Traveling:
		// Absorbing state; the travel itself tears this world down.
		break;
	}

	return Next;
}

EArchonMatchWinner UArchonMatchPolicyLibrary::EvaluateWinner(
	float CoreHealthTeamA,
	float CoreHealthTeamB,
	bool bTimeLimitExpired,
	int32 PointsTeamA,
	int32 PointsTeamB,
	int32 SitesOwnedTeamA,
	int32 SitesOwnedTeamB)
{
	const bool bCoreADead = CoreHealthTeamA <= 0.0f;
	const bool bCoreBDead = CoreHealthTeamB <= 0.0f;

	if (bCoreADead && bCoreBDead)
	{
		return EArchonMatchWinner::Draw;
	}
	if (bCoreADead)
	{
		return EArchonMatchWinner::TeamB;
	}
	if (bCoreBDead)
	{
		return EArchonMatchWinner::TeamA;
	}

	if (bTimeLimitExpired)
	{
		if (PointsTeamA > PointsTeamB)
		{
			return EArchonMatchWinner::TeamA;
		}
		if (PointsTeamB > PointsTeamA)
		{
			return EArchonMatchWinner::TeamB;
		}
		// Points tied (the all-AI stalemate shape): map control decides
		// before we concede a Draw. WC3/SC sensibility — a held map is a
		// won game, and the rotation deserves a verdict.
		if (SitesOwnedTeamA > SitesOwnedTeamB)
		{
			return EArchonMatchWinner::TeamA;
		}
		if (SitesOwnedTeamB > SitesOwnedTeamA)
		{
			return EArchonMatchWinner::TeamB;
		}
		return EArchonMatchWinner::Draw;
	}

	return EArchonMatchWinner::Undecided;
}

int32 UArchonMatchPolicyLibrary::ComputeTowerDamagePoints(float DamageApplied, const FArchonMatchConfig& Config)
{
	return FMath::RoundToInt(FMath::Max(0.0f, DamageApplied) * FMath::Max(0.0f, Config.TowerDamagePointScale));
}

bool UArchonMatchPolicyLibrary::CanAffordReinforcement(int32 Supply, const FArchonMatchConfig& Config)
{
	return Config.ReinforcementSquadSupplyCost > 0 && Supply >= Config.ReinforcementSquadSupplyCost;
}

bool UArchonMatchPolicyLibrary::ShouldAutoReinforce(int32 Supply, const FArchonMatchConfig& Config)
{
	if (Config.ReinforcementSquadSupplyCost <= 0)
	{
		return false;
	}
	const float Multiple = FMath::Max(1.0f, Config.AutoReinforceBankMultiple);
	return Supply >= FMath::CeilToInt(Config.ReinforcementSquadSupplyCost * Multiple);
}

FArchonResourceSiteState UArchonMatchPolicyLibrary::TickSiteCapture(
	const FArchonResourceSiteState& Site,
	int32 PresentTeamA,
	int32 PresentTeamB,
	float DeltaSeconds,
	const FArchonMatchConfig& Config)
{
	FArchonResourceSiteState Next = Site;
	const float Delta = FMath::Max(0.0f, DeltaSeconds);

	const bool bAPresent = PresentTeamA > 0;
	const bool bBPresent = PresentTeamB > 0;

	if (bAPresent && bBPresent)
	{
		// Contested: progress freezes until one side clears the zone.
		return Next;
	}

	if (!bAPresent && !bBPresent)
	{
		// Empty: capture progress decays; ownership persists.
		Next.CaptureProgressSeconds = FMath::Max(0.0f, Next.CaptureProgressSeconds - Delta);
		if (Next.CaptureProgressSeconds <= 0.0f)
		{
			Next.CapturingTeam = INDEX_NONE;
		}
		return Next;
	}

	const int32 PresentTeam = bAPresent ? 0 : 1;
	if (PresentTeam == Site.OwningTeam)
	{
		// Standing on your own site clears any rival progress.
		Next.CaptureProgressSeconds = 0.0f;
		Next.CapturingTeam = INDEX_NONE;
		return Next;
	}

	if (Next.CapturingTeam != PresentTeam)
	{
		Next.CapturingTeam = PresentTeam;
		Next.CaptureProgressSeconds = 0.0f;
	}
	Next.CaptureProgressSeconds += Delta;
	if (Next.CaptureProgressSeconds >= Config.SiteCaptureSeconds)
	{
		Next.OwningTeam = PresentTeam;
		Next.CapturingTeam = INDEX_NONE;
		Next.CaptureProgressSeconds = 0.0f;
	}

	return Next;
}

int32 UArchonMatchPolicyLibrary::ComputeSupplyPerTick(int32 SitesOwned, const FArchonMatchConfig& Config)
{
	return Config.BaseSupplyPerTick + FMath::Max(0, SitesOwned) * Config.SupplyPerSitePerTick;
}

bool UArchonMatchPolicyLibrary::IsLocationInsideSite(
	const FVector& Location,
	const FVector& SiteLocation,
	const FArchonMatchConfig& Config)
{
	return FVector::Dist2D(Location, SiteLocation) <= Config.SiteCaptureRadius;
}
