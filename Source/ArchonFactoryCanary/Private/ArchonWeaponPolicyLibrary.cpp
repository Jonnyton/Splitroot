#include "ArchonWeaponPolicyLibrary.h"

namespace
{
	int32 GetClampedCapacity(const FArchonWeaponStats& Stats)
	{
		return FMath::Max(0, Stats.QuiverCapacity);
	}
}

FArchonWeaponState UArchonWeaponPolicyLibrary::MakeInitialState(const FArchonWeaponStats& Stats)
{
	FArchonWeaponState State;
	State.FireState = EArchonWeaponFireState::Ready;
	State.CurrentAmmo = GetClampedCapacity(Stats);
	State.SecondsUntilReady = 0.0f;
	return State;
}

FArchonWeaponFireDecision UArchonWeaponPolicyLibrary::EvaluateFire(
	const FArchonWeaponState& CurrentState,
	const FArchonWeaponStats& Stats,
	bool bFirePressed)
{
	FArchonWeaponFireDecision Decision;
	Decision.NewState = CurrentState;

	if (!bFirePressed)
	{
		Decision.Reason = TEXT("not_pressed");
		return Decision;
	}

	if (CurrentState.FireState == EArchonWeaponFireState::OnFireCycle)
	{
		Decision.Reason = TEXT("on_cycle");
		return Decision;
	}

	if (CurrentState.FireState == EArchonWeaponFireState::Reloading)
	{
		Decision.Reason = TEXT("reloading");
		return Decision;
	}

	if (CurrentState.FireState == EArchonWeaponFireState::Empty || CurrentState.CurrentAmmo <= 0)
	{
		Decision.NewState.FireState = EArchonWeaponFireState::Empty;
		Decision.NewState.CurrentAmmo = 0;
		Decision.NewState.SecondsUntilReady = 0.0f;
		Decision.Reason = TEXT("empty");
		return Decision;
	}

	Decision.bShouldFire = true;
	Decision.NewState.FireState = EArchonWeaponFireState::OnFireCycle;
	Decision.NewState.CurrentAmmo = FMath::Max(0, CurrentState.CurrentAmmo - 1);
	Decision.NewState.SecondsUntilReady = FMath::Max(0.0f, Stats.FireCycleSeconds);
	Decision.Reason = TEXT("accepted_fire");
	return Decision;
}

FArchonWeaponReloadDecision UArchonWeaponPolicyLibrary::EvaluateReloadStart(
	const FArchonWeaponState& CurrentState,
	const FArchonWeaponStats& Stats,
	bool bReloadPressed)
{
	FArchonWeaponReloadDecision Decision;
	Decision.NewState = CurrentState;

	if (!bReloadPressed)
	{
		Decision.Reason = TEXT("not_pressed");
		return Decision;
	}

	if (CurrentState.FireState == EArchonWeaponFireState::Reloading)
	{
		Decision.Reason = TEXT("already_reloading");
		return Decision;
	}

	const int32 Capacity = GetClampedCapacity(Stats);
	if (CurrentState.CurrentAmmo >= Capacity)
	{
		Decision.Reason = TEXT("already_full");
		return Decision;
	}

	Decision.bShouldReload = true;
	Decision.NewState.FireState = EArchonWeaponFireState::Reloading;
	Decision.NewState.CurrentAmmo = FMath::Clamp(CurrentState.CurrentAmmo, 0, Capacity);
	Decision.NewState.SecondsUntilReady = FMath::Max(0.0f, Stats.ReloadSeconds);
	Decision.Reason = TEXT("accepted_reload");
	return Decision;
}

FArchonWeaponState UArchonWeaponPolicyLibrary::AdvanceWeaponState(
	const FArchonWeaponState& CurrentState,
	const FArchonWeaponStats& Stats,
	float DeltaSeconds,
	bool bAutoReloadOnEmpty)
{
	FArchonWeaponState NextState = CurrentState;
	const float ClampedDelta = FMath::Max(0.0f, DeltaSeconds);
	const int32 Capacity = GetClampedCapacity(Stats);

	switch (CurrentState.FireState)
	{
	case EArchonWeaponFireState::OnFireCycle:
		NextState.SecondsUntilReady = FMath::Max(0.0f, CurrentState.SecondsUntilReady - ClampedDelta);
		if (NextState.SecondsUntilReady <= 0.0f)
		{
			if (NextState.CurrentAmmo > 0)
			{
				NextState.FireState = EArchonWeaponFireState::Ready;
			}
			else if (bAutoReloadOnEmpty && Capacity > 0)
			{
				NextState.FireState = EArchonWeaponFireState::Reloading;
				NextState.SecondsUntilReady = FMath::Max(0.0f, Stats.ReloadSeconds);
			}
			else
			{
				NextState.FireState = EArchonWeaponFireState::Empty;
			}
		}
		break;

	case EArchonWeaponFireState::Reloading:
		NextState.SecondsUntilReady = FMath::Max(0.0f, CurrentState.SecondsUntilReady - ClampedDelta);
		if (NextState.SecondsUntilReady <= 0.0f)
		{
			NextState.FireState = EArchonWeaponFireState::Ready;
			NextState.CurrentAmmo = Capacity;
		}
		break;

	case EArchonWeaponFireState::Ready:
		NextState.SecondsUntilReady = 0.0f;
		NextState.CurrentAmmo = FMath::Clamp(CurrentState.CurrentAmmo, 0, Capacity);
		break;

	case EArchonWeaponFireState::Empty:
	default:
		NextState.FireState = EArchonWeaponFireState::Empty;
		NextState.CurrentAmmo = FMath::Clamp(CurrentState.CurrentAmmo, 0, Capacity);
		NextState.SecondsUntilReady = 0.0f;
		break;
	}

	return NextState;
}
