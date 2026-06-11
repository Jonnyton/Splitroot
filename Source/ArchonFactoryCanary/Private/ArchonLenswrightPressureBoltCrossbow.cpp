#include "ArchonLenswrightPressureBoltCrossbow.h"
#include "ArchonPressureBoltProjectile.h"

UArchonLenswrightPressureBoltCrossbow::UArchonLenswrightPressureBoltCrossbow()
{
	FArchonWeaponStats DefaultStats;
	DefaultStats.WeaponClass = EArchonWeaponClass::LenswrightPressureBoltCrossbow;
	DefaultStats.QuiverCapacity = 1;
	DefaultStats.FireCycleSeconds = 1.5f;
	DefaultStats.ReloadSeconds = 2.2f;
	DefaultStats.ProjectileSpeed = 6000.0f;
	DefaultStats.MaxRangeUnits = 6000.0f;

	ProjectileClass = AArchonPressureBoltProjectile::StaticClass();
	ConfigureWeapon(INDEX_NONE, INDEX_NONE, DefaultStats);
}
