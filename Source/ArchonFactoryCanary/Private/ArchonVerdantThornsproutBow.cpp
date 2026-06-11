#include "ArchonVerdantThornsproutBow.h"
#include "ArchonArrowProjectile.h"

UArchonVerdantThornsproutBow::UArchonVerdantThornsproutBow()
{
	FArchonWeaponStats DefaultStats;
	DefaultStats.WeaponClass = EArchonWeaponClass::VerdantThornsproutBow;
	// Effectively unlimited (Jonathan 2026-06-10: "just make it
	// unlimited") — the fire cycle stays the rhythm, ammo never gates.
	DefaultStats.QuiverCapacity = 999;
	DefaultStats.FireCycleSeconds = 1.2f;
	DefaultStats.ReloadSeconds = 1.8f;
	// -30% flight speed (Jonathan 2026-06-10) — visibility + dodgeability.
	DefaultStats.ProjectileSpeed = 5600.0f;
	DefaultStats.MaxRangeUnits = 8000.0f;

	ProjectileClass = AArchonArrowProjectile::StaticClass();
	ConfigureWeapon(INDEX_NONE, INDEX_NONE, DefaultStats);
}
