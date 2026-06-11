#include "ArchonRangedWeaponComponent.h"
#include "ArchonArrowProjectile.h"
#include "ArchonCombatPolicyLibrary.h"
#include "ArchonWeaponPolicyLibrary.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"

UArchonRangedWeaponComponent::UArchonRangedWeaponComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	SetIsReplicatedByDefault(true);
	State = UArchonWeaponPolicyLibrary::MakeInitialState(Stats);
}

void UArchonRangedWeaponComponent::BeginPlay()
{
	Super::BeginPlay();
	State.CurrentAmmo = FMath::Clamp(State.CurrentAmmo, 0, FMath::Max(0, Stats.QuiverCapacity));
}

void UArchonRangedWeaponComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	if (IsRegistered())
	{
		Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	}

	if (!HasWeaponAuthority())
	{
		return;
	}

	const EArchonWeaponFireState PreviousFireState = State.FireState;
	State = UArchonWeaponPolicyLibrary::AdvanceWeaponState(State, Stats, DeltaTime, bAutoReloadOnEmpty);

	if (PreviousFireState == EArchonWeaponFireState::Reloading && State.FireState == EArchonWeaponFireState::Ready)
	{
		++ReloadCount;
		OnReloadCompleted.Broadcast();
		UE_LOG(
			LogTemp,
			Display,
			TEXT("ArchonFactoryCanary: WeaponReloadCompleted weapon=%d ammo=%d reloadCount=%d"),
			static_cast<int32>(Stats.WeaponClass),
			State.CurrentAmmo,
			ReloadCount);
	}
}

void UArchonRangedWeaponComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UArchonRangedWeaponComponent, OwnerTeamId);
	DOREPLIFETIME(UArchonRangedWeaponComponent, OwnerPlayerId);
	DOREPLIFETIME(UArchonRangedWeaponComponent, Stats);
	DOREPLIFETIME(UArchonRangedWeaponComponent, State);
	DOREPLIFETIME(UArchonRangedWeaponComponent, ShotsFired);
	DOREPLIFETIME(UArchonRangedWeaponComponent, ReloadCount);
}

void UArchonRangedWeaponComponent::ConfigureWeapon(
	int32 InOwnerTeamId,
	int32 InOwnerPlayerId,
	const FArchonWeaponStats& InStats)
{
	OwnerTeamId = InOwnerTeamId;
	OwnerPlayerId = InOwnerPlayerId;
	Stats = InStats;
	Stats.QuiverCapacity = FMath::Max(0, Stats.QuiverCapacity);
	Stats.FireCycleSeconds = FMath::Max(0.0f, Stats.FireCycleSeconds);
	Stats.ReloadSeconds = FMath::Max(0.0f, Stats.ReloadSeconds);
	Stats.ProjectileSpeed = FMath::Max(0.0f, Stats.ProjectileSpeed);
	Stats.MaxRangeUnits = FMath::Max(0.0f, Stats.MaxRangeUnits);
	State = UArchonWeaponPolicyLibrary::MakeInitialState(Stats);
	ShotsFired = 0;
	ReloadCount = 0;
}

bool UArchonRangedWeaponComponent::TryFire(const FVector& Origin, const FVector& Direction)
{
	if (!HasWeaponAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("ArchonFactoryCanary: WeaponFireRejected reason=not_authority"));
		return false;
	}

	const FArchonWeaponFireDecision Decision = UArchonWeaponPolicyLibrary::EvaluateFire(State, Stats, true);
	if (!Decision.bShouldFire)
	{
		State = Decision.NewState;
		// VeryVerbose: bots hold the trigger every tick — cycle-gate
		// rejections hit ~195k lines per match at Display (2026-06-10).
		UE_LOG(
			LogTemp,
			VeryVerbose,
			TEXT("ArchonFactoryCanary: WeaponFireRejected reason=%s ammo=%d state=%d"),
			*Decision.Reason.ToString(),
			State.CurrentAmmo,
			static_cast<int32>(State.FireState));
		return false;
	}

	State = Decision.NewState;
	const FArchonShotPayload Shot = MakeShotPayload(Origin, Direction);
	SpawnProjectile(Shot);
	++ShotsFired;
	OnWeaponFired.Broadcast(Shot);

	UE_LOG(
		LogTemp,
		Display,
		TEXT("ArchonFactoryCanary: WeaponFired weapon=%d team=%d player=%d ammo=%d shots=%d"),
		static_cast<int32>(Stats.WeaponClass),
		OwnerTeamId,
		OwnerPlayerId,
		State.CurrentAmmo,
		ShotsFired);

	return true;
}

bool UArchonRangedWeaponComponent::TryReload()
{
	if (!HasWeaponAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("ArchonFactoryCanary: WeaponReloadRejected reason=not_authority"));
		return false;
	}

	const FArchonWeaponReloadDecision Decision = UArchonWeaponPolicyLibrary::EvaluateReloadStart(State, Stats, true);
	if (!Decision.bShouldReload)
	{
		State = Decision.NewState;
		UE_LOG(
			LogTemp,
			Display,
			TEXT("ArchonFactoryCanary: WeaponReloadRejected reason=%s ammo=%d state=%d"),
			*Decision.Reason.ToString(),
			State.CurrentAmmo,
			static_cast<int32>(State.FireState));
		return false;
	}

	State = Decision.NewState;
	OnReloadStarted.Broadcast();

	UE_LOG(
		LogTemp,
		Display,
		TEXT("ArchonFactoryCanary: WeaponReloadStarted weapon=%d ammo=%d seconds=%.2f"),
		static_cast<int32>(Stats.WeaponClass),
		State.CurrentAmmo,
		State.SecondsUntilReady);

	return true;
}

void UArchonRangedWeaponComponent::SpawnProjectile(const FArchonShotPayload& Shot)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	UClass* ClassToSpawn = ProjectileClass ? ProjectileClass.Get() : AArchonArrowProjectile::StaticClass();
	if (!ClassToSpawn)
	{
		return;
	}

	const FRotator SpawnRotation = Shot.ShotDirection.Rotation();
	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = GetOwner();
	SpawnParameters.Instigator = Cast<APawn>(GetOwner());

	AArchonArrowProjectile* Projectile = World->SpawnActor<AArchonArrowProjectile>(
		ClassToSpawn,
		Shot.ShotOrigin,
		SpawnRotation,
		SpawnParameters);

	if (Projectile)
	{
		Projectile->ConfigureShot(
			Shot,
			UArchonCombatPolicyLibrary::GetDefaultWeaponProfile(Stats.WeaponClass));
	}
}

FArchonShotPayload UArchonRangedWeaponComponent::MakeShotPayload(const FVector& Origin, const FVector& Direction) const
{
	const FVector SafeDirection = Direction.GetSafeNormal().IsNearlyZero()
		? FVector::ForwardVector
		: Direction.GetSafeNormal();

	FArchonShotPayload Shot;
	Shot.InstigatorTeamId = OwnerTeamId;
	Shot.InstigatorPlayerId = OwnerPlayerId;
	Shot.WeaponClass = Stats.WeaponClass;
	Shot.DamageType = GetDamageTypeForWeaponClass(Stats.WeaponClass);
	Shot.ShotOrigin = Origin;
	Shot.ShotDirection = SafeDirection;
	Shot.HitLocation = Origin + SafeDirection * Stats.MaxRangeUnits;
	Shot.DistanceTraveled = Stats.MaxRangeUnits;
	Shot.HitType = EArchonHitType::Body;
	return Shot;
}

EArchonDamageType UArchonRangedWeaponComponent::GetDamageTypeForWeaponClass(EArchonWeaponClass WeaponClass)
{
	switch (WeaponClass)
	{
	case EArchonWeaponClass::VerdantThornsproutBow:
		return EArchonDamageType::VerdantLivingArrow;

	case EArchonWeaponClass::LenswrightPressureBoltCrossbow:
		return EArchonDamageType::LenswrightPressureBolt;

	case EArchonWeaponClass::None:
	default:
		return EArchonDamageType::Generic;
	}
}

bool UArchonRangedWeaponComponent::HasWeaponAuthority() const
{
	const AActor* Owner = GetOwner();
	return !Owner || Owner->HasAuthority();
}
