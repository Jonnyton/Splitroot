#include "ArchonSessionPolicyLibrary.h"

EArchonSessionRoute UArchonSessionPolicyLibrary::ResolveRouteFromName(const FString& RouteName)
{
	const FString Normalized = RouteName.TrimStartAndEnd().ToLower();

	if (Normalized == TEXT("offlineskirmish") || Normalized == TEXT("offline") || Normalized == TEXT("skirmish"))
	{
		return EArchonSessionRoute::OfflineSkirmish;
	}

	if (Normalized == TEXT("lanhosted") || Normalized == TEXT("lan"))
	{
		return EArchonSessionRoute::LANHosted;
	}

	if (Normalized == TEXT("privatehost") || Normalized == TEXT("private"))
	{
		return EArchonSessionRoute::PrivateHost;
	}

	return EArchonSessionRoute::SteamOnline;
}

EArchonSessionRoute UArchonSessionPolicyLibrary::ResolveAmbiguousOnlineFallback()
{
	return EArchonSessionRoute::SteamOnline;
}

bool UArchonSessionPolicyLibrary::IsFullFreeRoute(EArchonSessionRoute Route)
{
	return Route == EArchonSessionRoute::OfflineSkirmish ||
		Route == EArchonSessionRoute::LANHosted ||
		Route == EArchonSessionRoute::PrivateHost;
}

FArchonEffectiveAccess UArchonSessionPolicyLibrary::MakeEffectiveAccess(
	EArchonSessionRoute Route,
	bool bOwnsHorizontalHeroExpansion,
	bool bOwnsRtsExecutionExpansion)
{
	FArchonEffectiveAccess Access;
	Access.bCoreCombat = true;
	Access.bRespawn = true;
	Access.bDefaultHeroes = true;
	Access.bMapTablePreview = true;

	if (IsFullFreeRoute(Route))
	{
		Access.bAllHeroes = true;
		Access.bHorizontalPaidHeroes = true;
		Access.bLiveRtsExecution = true;
		return Access;
	}

	Access.bAllHeroes = false;
	Access.bHorizontalPaidHeroes = bOwnsHorizontalHeroExpansion;
	Access.bLiveRtsExecution = bOwnsRtsExecutionExpansion;
	return Access;
}

bool UArchonSessionPolicyLibrary::CanExecuteLiveRtsCommands(EArchonSessionRoute Route, bool bOwnsRtsExecutionExpansion)
{
	return MakeEffectiveAccess(Route, false, bOwnsRtsExecutionExpansion).bLiveRtsExecution;
}

bool UArchonSessionPolicyLibrary::IsMapTablePreviewOnly(EArchonSessionRoute Route, bool bOwnsRtsExecutionExpansion)
{
	const FArchonEffectiveAccess Access = MakeEffectiveAccess(Route, false, bOwnsRtsExecutionExpansion);
	return Access.bMapTablePreview && !Access.bLiveRtsExecution;
}

bool UArchonSessionPolicyLibrary::CanSpawnHero(
	EArchonSessionRoute Route,
	const FArchonHeroEntitlement& Hero,
	bool bOwnsHorizontalHeroExpansion)
{
	if (Hero.bImprovesCombatPower)
	{
		return false;
	}

	if (IsFullFreeRoute(Route))
	{
		return true;
	}

	if (Hero.bIsDefaultHero)
	{
		return true;
	}

	return Hero.bIsHorizontalPaidHero && bOwnsHorizontalHeroExpansion;
}
