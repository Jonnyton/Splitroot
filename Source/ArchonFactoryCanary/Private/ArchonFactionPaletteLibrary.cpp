#include "ArchonFactionPaletteLibrary.h"

namespace
{
	// Verdant Choir — green-cream
	constexpr FLinearColor kVerdantPrimary  (0.30f, 0.55f, 0.25f, 1.0f);
	constexpr FLinearColor kVerdantSecondary(0.96f, 0.91f, 0.78f, 1.0f);
	constexpr FLinearColor kVerdantTertiary (0.11f, 0.23f, 0.12f, 1.0f);
	constexpr FLinearColor kVerdantAccent   (0.66f, 0.82f, 0.38f, 1.0f);

	// Lenswright Compact — brass-oxblood (Accent is alchemical cyan, NOT muzzle-flash)
	constexpr FLinearColor kLenswrightPrimary  (0.55f, 0.30f, 0.15f, 1.0f);
	constexpr FLinearColor kLenswrightSecondary(0.85f, 0.66f, 0.42f, 1.0f);
	constexpr FLinearColor kLenswrightTertiary (0.24f, 0.12f, 0.10f, 1.0f);
	constexpr FLinearColor kLenswrightAccent   (0.37f, 0.78f, 0.88f, 1.0f);

	// Kinwild Covenant — ochre-grey
	constexpr FLinearColor kKinwildPrimary  (0.65f, 0.50f, 0.24f, 1.0f);
	constexpr FLinearColor kKinwildSecondary(0.49f, 0.54f, 0.55f, 1.0f);
	constexpr FLinearColor kKinwildTertiary (0.23f, 0.16f, 0.06f, 1.0f);
	constexpr FLinearColor kKinwildAccent   (0.25f, 0.43f, 0.48f, 1.0f);

	// Faction-agnostic fallback (also returned for EArchonFaction::None).
	constexpr FLinearColor kNeutralFallback (0.50f, 0.50f, 0.50f, 1.0f);

	constexpr FLinearColor kNeutralCoverStone    (0.35f, 0.35f, 0.35f, 1.0f);
	constexpr FLinearColor kNeutralSplitrootWood (0.45f, 0.30f, 0.15f, 1.0f);
	// Retuned 2026-06-09 from rendered valley iteration 5: 0.35/0.27/0.19
	// lifted to desert sand under the warm sun + EV 2.6 lock. Loam, not sand.
	constexpr FLinearColor kNeutralGroundEarth   (0.16f, 0.13f, 0.08f, 1.0f);
	constexpr FLinearColor kNeutralSkyHorizon    (0.58f, 0.64f, 0.66f, 1.0f);
}

FLinearColor UArchonFactionPaletteLibrary::GetFactionColor(EArchonFaction Faction, EArchonFactionPaletteSlot Slot)
{
	switch (Faction)
	{
	case EArchonFaction::VerdantChoir:
		switch (Slot)
		{
		case EArchonFactionPaletteSlot::Primary:   return kVerdantPrimary;
		case EArchonFactionPaletteSlot::Secondary: return kVerdantSecondary;
		case EArchonFactionPaletteSlot::Tertiary:  return kVerdantTertiary;
		case EArchonFactionPaletteSlot::Accent:    return kVerdantAccent;
		}
		break;
	case EArchonFaction::LenswrightCompact:
		switch (Slot)
		{
		case EArchonFactionPaletteSlot::Primary:   return kLenswrightPrimary;
		case EArchonFactionPaletteSlot::Secondary: return kLenswrightSecondary;
		case EArchonFactionPaletteSlot::Tertiary:  return kLenswrightTertiary;
		case EArchonFactionPaletteSlot::Accent:    return kLenswrightAccent;
		}
		break;
	case EArchonFaction::KinwildCovenant:
		switch (Slot)
		{
		case EArchonFactionPaletteSlot::Primary:   return kKinwildPrimary;
		case EArchonFactionPaletteSlot::Secondary: return kKinwildSecondary;
		case EArchonFactionPaletteSlot::Tertiary:  return kKinwildTertiary;
		case EArchonFactionPaletteSlot::Accent:    return kKinwildAccent;
		}
		break;
	case EArchonFaction::None:
	default:
		break;
	}
	return kNeutralFallback;
}

float UArchonFactionPaletteLibrary::GetFactionMetallic(EArchonFaction Faction)
{
	switch (Faction)
	{
	case EArchonFaction::VerdantChoir:      return 0.1f;
	case EArchonFaction::LenswrightCompact: return 0.6f;
	case EArchonFaction::KinwildCovenant:   return 0.2f;
	case EArchonFaction::None:
	default:                                return 0.0f;
	}
}

float UArchonFactionPaletteLibrary::GetFactionRoughness(EArchonFaction Faction)
{
	switch (Faction)
	{
	case EArchonFaction::VerdantChoir:      return 0.7f;
	case EArchonFaction::LenswrightCompact: return 0.3f;
	case EArchonFaction::KinwildCovenant:   return 0.5f;
	case EArchonFaction::None:
	default:                                return 0.5f;
	}
}

FLinearColor UArchonFactionPaletteLibrary::GetNeutralColor(EArchonNeutralPaletteSlot Slot)
{
	switch (Slot)
	{
	case EArchonNeutralPaletteSlot::CoverStone:    return kNeutralCoverStone;
	case EArchonNeutralPaletteSlot::SplitrootWood: return kNeutralSplitrootWood;
	case EArchonNeutralPaletteSlot::GroundEarth:   return kNeutralGroundEarth;
	case EArchonNeutralPaletteSlot::SkyHorizon:    return kNeutralSkyHorizon;
	default:                                       return kNeutralFallback;
	}
}

FArchonLightingAnchor UArchonFactionPaletteLibrary::GetLightingAnchor()
{
	return FArchonLightingAnchor();
}
