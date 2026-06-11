#include "ArchonArrowProjectile.h"
#include "ArchonCombatHealthComponent.h"
#include "ArchonCombatPolicyLibrary.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "UObject/ConstructorHelpers.h"

AArchonArrowProjectile::AArchonArrowProjectile()
{
	PrimaryActorTick.bCanEverTick = false;

	ArrowMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ArrowMesh"));
	SetRootComponent(ArrowMesh);
	ArrowMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	ArrowMesh->SetCollisionObjectType(ECC_WorldDynamic);
	ArrowMesh->SetCollisionResponseToAllChannels(ECR_Block);
	ArrowMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	ArrowMesh->SetGenerateOverlapEvents(true);
	ArrowMesh->SetNotifyRigidBodyCollision(true);
	// Fast small projectile: continuous collision so arrows never tunnel
	// through thin geometry (Jonathan 2026-06-10: "arrows should not
	// phase through buildings").
	ArrowMesh->BodyInstance.bUseCCD = true;
	ArrowMesh->OnComponentHit.AddDynamic(this, &AArchonArrowProjectile::HandleComponentHit);
	ArrowMesh->OnComponentBeginOverlap.AddDynamic(this, &AArchonArrowProjectile::HandleComponentOverlap);

	// Reads as an actual arrow now (Jonathan 2026-06-10): a long shaft
	// with a distinct head, scaled up for visibility. The root collider
	// stays the (bigger) head cone; the shaft is cosmetic.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> ConeMesh(TEXT("/Engine/BasicShapes/Cone.Cone"));
	if (ConeMesh.Succeeded())
	{
		ArrowMesh->SetStaticMesh(ConeMesh.Object);
		ArrowMesh->SetWorldScale3D(FVector(0.16f, 0.16f, 0.6f));
	}

	ShaftMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ArrowShaft"));
	ShaftMesh->SetupAttachment(ArrowMesh);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> ShaftFinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (ShaftFinder.Succeeded())
	{
		ShaftMesh->SetStaticMesh(ShaftFinder.Object);
	}
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> ShaftMaterialFinder(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (ShaftMaterialFinder.Succeeded())
	{
		ShaftMesh->SetMaterial(0, ShaftMaterialFinder.Object);
	}
	// Parent (cone) scale is (0.16, 0.16, 0.6); relative values below are
	// in that squashed space: a thin meter-long shaft trailing the head.
	ShaftMesh->SetRelativeScale3D(FVector(0.22f, 0.22f, 1.6f));
	ShaftMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -1.1f));
	ShaftMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	Movement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	// 30% slower through the air (Jonathan 2026-06-10): easier to see,
	// easier to dodge, more bow-like flight.
	Movement->InitialSpeed = 5600.0f;
	Movement->MaxSpeed = 5600.0f;
	Movement->bRotationFollowsVelocity = true;
	Movement->ProjectileGravityScale = 0.15f;
}

void AArchonArrowProjectile::ConfigureShot(
	const FArchonShotPayload& InShot,
	const FArchonWeaponDamageProfile& InWeaponProfile)
{
	Shot = InShot;
	WeaponProfile = InWeaponProfile;

	// The muzzle (camera) sits INSIDE the shooter's capsule: without this
	// every arrow detonates on the archer's own face. Found by the first
	// all-bot match (2026-06-10) — two teams fired for two minutes with
	// zero deaths.
	if (AActor* OwnerActor = GetOwner())
	{
		if (ArrowMesh)
		{
			ArrowMesh->IgnoreActorWhenMoving(OwnerActor, true);
		}
	}

	const FVector SafeDirection = Shot.ShotDirection.GetSafeNormal().IsNearlyZero()
		? FVector::ForwardVector
		: Shot.ShotDirection.GetSafeNormal();
	Shot.ShotDirection = SafeDirection;

	if (Movement)
	{
		Movement->Velocity = SafeDirection * Movement->InitialSpeed;
	}
}

void AArchonArrowProjectile::HandleHit(
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	FVector NormalImpulse,
	const FHitResult& Hit)
{
	if (!OtherActor || OtherActor == this || OtherActor == GetOwner())
	{
		return;
	}

	if (UArchonCombatHealthComponent* Health = OtherActor->FindComponentByClass<UArchonCombatHealthComponent>())
	{
		FArchonShotPayload HitShot = Shot;
		const FVector ImpactPoint = Hit.ImpactPoint;
		HitShot.HitLocation = ImpactPoint.IsNearlyZero() ? GetActorLocation() : ImpactPoint;
		HitShot.DistanceTraveled = FVector::Dist(HitShot.ShotOrigin, HitShot.HitLocation);
		if (HitShot.HitType == EArchonHitType::None)
		{
			HitShot.HitType = EArchonHitType::Body;
		}

		const FArchonHitResult CombatResult = UArchonCombatPolicyLibrary::ResolveShot(
			HitShot,
			Health->GetTeamId(),
			Health->IsAlive(),
			Health->GetArmorModifier(),
			WeaponProfile);
		const FArchonDamageApplicationResult DamageResult = Health->ApplyHit(CombatResult);
		if (!DamageResult.bAccepted)
		{
			return;
		}
	}

	Destroy();
}

void AArchonArrowProjectile::HandleComponentHit(
	UPrimitiveComponent* HitComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	FVector NormalImpulse,
	const FHitResult& Hit)
{
	HandleHit(OtherActor, OtherComp, NormalImpulse, Hit);
}

void AArchonArrowProjectile::HandleComponentOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	HandleHit(OtherActor, OtherComp, FVector::ZeroVector, SweepResult);
}
