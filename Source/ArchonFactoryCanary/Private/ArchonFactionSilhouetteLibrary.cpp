#include "ArchonFactionSilhouetteLibrary.h"

namespace
{
	FArchonFactionSilhouette MakeVerdantSilhouette()
	{
		FArchonFactionSilhouette S;
		S.CapsuleHeightMultiplier = 1.05f;
		S.CapsuleRadiusMultiplier = 1.0f;
		S.LowerCapsuleRadiusMultiplier = 1.0f;
		S.MantleType = EArchonMantleType::RoundedTopCap;
		S.ShoulderProtrusionUnits = 35.0f;
		S.ShoulderSide = EArchonShoulderSide::Right;
		S.HeadLensSphereRadiusUnits = 0.0f;
		S.ForwardLeanDegrees = 0.0f;
		S.bHasAnimalCompanion = false;
		return S;
	}

	FArchonFactionSilhouette MakeLenswrightSilhouette()
	{
		FArchonFactionSilhouette S;
		S.CapsuleHeightMultiplier = 1.0f;
		S.CapsuleRadiusMultiplier = 1.0f;
		S.LowerCapsuleRadiusMultiplier = 1.2f;
		S.MantleType = EArchonMantleType::RightShoulderProtrusion;
		S.ShoulderProtrusionUnits = 50.0f;
		S.ShoulderSide = EArchonShoulderSide::Right;
		S.HeadLensSphereRadiusUnits = 12.0f;
		S.ForwardLeanDegrees = 0.0f;
		S.bHasAnimalCompanion = false;
		return S;
	}

	FArchonFactionSilhouette MakeKinwildSilhouette()
	{
		FArchonFactionSilhouette S;
		S.CapsuleHeightMultiplier = 1.0f;
		S.CapsuleRadiusMultiplier = 1.0f;
		S.LowerCapsuleRadiusMultiplier = 1.0f;
		S.MantleType = EArchonMantleType::MantleCloak;
		S.ShoulderProtrusionUnits = 0.0f;
		S.ShoulderSide = EArchonShoulderSide::None;
		S.HeadLensSphereRadiusUnits = 0.0f;
		S.ForwardLeanDegrees = 10.0f;
		S.bHasAnimalCompanion = true;
		S.CompanionCapsuleHeightUnits = 80.0f;
		S.CompanionCapsuleRadiusUnits = 28.0f;
		S.CompanionOffsetRightUnits = 80.0f;
		return S;
	}
}

FArchonFactionSilhouette UArchonFactionSilhouetteLibrary::GetFactionSilhouette(EArchonFaction Faction)
{
	switch (Faction)
	{
	case EArchonFaction::VerdantChoir:      return MakeVerdantSilhouette();
	case EArchonFaction::LenswrightCompact: return MakeLenswrightSilhouette();
	case EArchonFaction::KinwildCovenant:   return MakeKinwildSilhouette();
	case EArchonFaction::None:
	default:                                return FArchonFactionSilhouette();
	}
}

FArchonHeroSilhouetteOverlay UArchonFactionSilhouetteLibrary::GetHeroOverlay()
{
	return FArchonHeroSilhouetteOverlay();
}

float UArchonFactionSilhouetteLibrary::GetResolvedCapsuleHeightUnits(EArchonFaction Faction)
{
	return GetBaselineCapsuleHeightUnits() * GetFactionSilhouette(Faction).CapsuleHeightMultiplier;
}

float UArchonFactionSilhouetteLibrary::GetResolvedCapsuleRadiusUnits(EArchonFaction Faction)
{
	return GetBaselineCapsuleRadiusUnits() * GetFactionSilhouette(Faction).CapsuleRadiusMultiplier;
}
