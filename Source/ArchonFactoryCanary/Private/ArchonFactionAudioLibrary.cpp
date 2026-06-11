#include "ArchonFactionAudioLibrary.h"
#include "Sound/SoundBase.h"

namespace
{
	const TArray<FString>& LenswrightForbiddenTermsRef()
	{
		static const TArray<FString> Terms = {
			TEXT("gunshot"), TEXT("gunfire"), TEXT("firearm"), TEXT("muzzle"),
			TEXT("muzzle_flash"), TEXT("rifle"), TEXT("pistol"), TEXT("handgun"),
			TEXT("musket"), TEXT("blunderbuss"), TEXT("powder"), TEXT("gunpowder"),
			TEXT("crack"), TEXT("bang"), TEXT("pop"), TEXT("flintlock"),
			TEXT("matchlock"), TEXT("wheellock"), TEXT("snaplock")
		};
		return Terms;
	}
}

USoundBase* UArchonFactionAudioLibrary::GetFactionWeaponFireCue(EArchonFaction /*Faction*/)
{
	// Asset import pending — Freesound CC0/CC-BY sourcing + EQ pass per faction
	// is Jonathan's lane. Returning nullptr is the honest absence until imports land.
	return nullptr;
}

USoundBase* UArchonFactionAudioLibrary::GetFactionWeaponImpactCue(EArchonFaction /*Faction*/)
{
	return nullptr;
}

USoundBase* UArchonFactionAudioLibrary::GetFactionFootstepCue(EArchonFaction /*Faction*/)
{
	return nullptr;
}

USoundBase* UArchonFactionAudioLibrary::GetFactionHeroAmbientLoop(EArchonFaction /*Faction*/)
{
	return nullptr;
}

USoundBase* UArchonFactionAudioLibrary::GetFactionDeathCue(EArchonFaction /*Faction*/)
{
	return nullptr;
}

USoundBase* UArchonFactionAudioLibrary::GetStrategicEventCue(EArchonStrategicAudioEvent /*Event*/)
{
	return nullptr;
}

TArray<FString> UArchonFactionAudioLibrary::GetLenswrightForbiddenTerms()
{
	return LenswrightForbiddenTermsRef();
}

bool UArchonFactionAudioLibrary::ValidateLenswrightCueName(const FString& CueName)
{
	const FString Lower = CueName.ToLower();
	for (const FString& Term : LenswrightForbiddenTermsRef())
	{
		if (Lower.Contains(Term, ESearchCase::IgnoreCase))
		{
			return false;
		}
	}
	return true;
}

bool UArchonFactionAudioLibrary::ValidateCueNameForFaction(EArchonFaction Faction, const FString& CueName)
{
	if (Faction == EArchonFaction::LenswrightCompact)
	{
		return ValidateLenswrightCueName(CueName);
	}
	return true;
}
