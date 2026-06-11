#include "ArchonFactionTintedWidget.h"
#include "ArchonFactionPaletteLibrary.h"

void UArchonFactionTintedWidget::ConfigureFactionTint(EArchonFaction InFaction)
{
	if (Faction == InFaction)
	{
		return;
	}
	Faction = InFaction;
	OnFactionTintChangedBP();
}

FLinearColor UArchonFactionTintedWidget::GetPrimaryTint() const
{
	return UArchonFactionPaletteLibrary::GetFactionColor(Faction, EArchonFactionPaletteSlot::Primary);
}

FLinearColor UArchonFactionTintedWidget::GetSecondaryTint() const
{
	return UArchonFactionPaletteLibrary::GetFactionColor(Faction, EArchonFactionPaletteSlot::Secondary);
}

FLinearColor UArchonFactionTintedWidget::GetTertiaryTint() const
{
	return UArchonFactionPaletteLibrary::GetFactionColor(Faction, EArchonFactionPaletteSlot::Tertiary);
}

FLinearColor UArchonFactionTintedWidget::GetAccentTint() const
{
	return UArchonFactionPaletteLibrary::GetFactionColor(Faction, EArchonFactionPaletteSlot::Accent);
}

FLinearColor UArchonFactionTintedWidget::GetTintForSlot(EArchonFactionPaletteSlot PaletteSlot) const
{
	return UArchonFactionPaletteLibrary::GetFactionColor(Faction, PaletteSlot);
}
