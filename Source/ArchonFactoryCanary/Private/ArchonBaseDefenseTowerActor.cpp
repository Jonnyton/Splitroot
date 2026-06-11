#include "ArchonBaseDefenseTowerActor.h"
#include "ArchonArrowProjectile.h"
#include "ArchonCanaryFpsCharacter.h"
#include "ArchonCombatHealthComponent.h"
#include "ArchonCombatTypes.h"
#include "ArchonMatchStateActor.h"
#include "ArchonWorldHealthBarComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

AArchonBaseDefenseTowerActor::AArchonBaseDefenseTowerActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	TowerMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DefenseTower"));
	RootComponent = TowerMesh;

	// Temp-asset rule: a castle tower stands in for the auto-defense
	// (Kenney Castle Kit, CC0); cylinder fallback keeps proofs alive.
	ConstructorHelpers::FObjectFinder<UStaticMesh> StandInTower(TEXT("/Game/StandIns/Castle/tower-square.tower-square"));
	ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderFinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (StandInTower.Succeeded())
	{
		TowerMesh->SetStaticMesh(StandInTower.Object);
		bUsingStandInMesh = true;
	}
	else if (CylinderFinder.Succeeded())
	{
		TowerMesh->SetStaticMesh(CylinderFinder.Object);
	}
	ConstructorHelpers::FObjectFinder<UMaterialInterface> MaterialFinder(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (!bUsingStandInMesh && MaterialFinder.Succeeded())
	{
		TowerMesh->SetMaterial(0, MaterialFinder.Object);
	}
	TowerMesh->SetCollisionProfileName(TEXT("BlockAll"));

	Health = CreateDefaultSubobject<UArchonCombatHealthComponent>(TEXT("TowerHealth"));

	// WC3 readability: the siege state of a tower is visible at a glance.
	// Absolute scale/rotation — the actor is scaled ~7x for the stand-in
	// mesh and the bar billboards independently.
	HealthBar = CreateDefaultSubobject<UArchonWorldHealthBarComponent>(TEXT("TowerHealthBar"));
	HealthBar->SetupAttachment(TowerMesh);
	HealthBar->SetUsingAbsoluteScale(true);
	HealthBar->SetUsingAbsoluteRotation(true);
	HealthBar->SetRelativeLocation(FVector(0.0f, 0.0f, 115.0f));

	bReplicates = true;
	bAlwaysRelevant = true;

	Tags.Add(TEXT("ArchonDefenseTower"));
}

void AArchonBaseDefenseTowerActor::ConfigureTower(const FArchonValleyPlacement& Placement)
{
	TeamId = Placement.TeamId;
	HealthyColor = Placement.DebugColor;
	// The Kenney tower is ~1m-scale base geometry; the layout's pillar
	// scale (1.2, 1.2, 5) was authored for a 100uu cylinder. Use a chunky
	// uniform scale for the stand-in so it reads as a guard tower.
	SetActorScale3D(bUsingStandInMesh ? FVector(7.0f, 7.0f, 7.0f) : Placement.Scale);

	if (TowerMesh)
	{
		TowerMaterial = TowerMesh->CreateDynamicMaterialInstance(0);
		if (TowerMaterial)
		{
			TowerMaterial->SetVectorParameterValue(TEXT("Color"), HealthyColor);
			TowerMaterial->SetVectorParameterValue(TEXT("BaseColor"), HealthyColor);
		}
	}

	if (Health)
	{
		Health->ConfigureHealth(TeamId, TowerMaxHealth, 1.0f);
		Health->OnHealthChanged.AddUObject(this, &AArchonBaseDefenseTowerActor::HandleHealthChanged);
	}

	if (HealthBar)
	{
		HealthBar->ConfigureBarSize(260.0f, 26.0f);
		HealthBar->SetHealthFraction(1.0f);
	}

	UE_LOG(
		LogTemp,
		Display,
		TEXT("ArchonFactoryCanary: DefenseTowerConfigured team=%d damagePerShot=%.1f range=%.0f"),
		TeamId,
		DamagePerShot,
		TargetRange);
}

void AArchonBaseDefenseTowerActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!HasAuthority() || !Health || !Health->IsAlive())
	{
		return;
	}

	FireCooldownRemaining -= DeltaSeconds;
	if (FireCooldownRemaining > 0.0f)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// Nearest living enemy body in reach (Renegade AGT pattern, gentle).
	const FVector TowerLocation = GetActorLocation();
	AArchonCanaryFpsCharacter* Target = nullptr;
	float NearestDistance = TargetRange;
	for (TActorIterator<AArchonCanaryFpsCharacter> It(World); It; ++It)
	{
		AArchonCanaryFpsCharacter* Candidate = *It;
		UArchonCombatHealthComponent* CandidateHealth = Candidate ? Candidate->GetHealth() : nullptr;
		if (!CandidateHealth || !CandidateHealth->IsAlive() || CandidateHealth->GetTeamId() == TeamId)
		{
			continue;
		}
		const float Distance = FVector::Dist2D(TowerLocation, Candidate->GetActorLocation());
		if (Distance < NearestDistance)
		{
			NearestDistance = Distance;
			Target = Candidate;
		}
	}
	if (!Target)
	{
		return;
	}

	FireCooldownRemaining = FireCycleSeconds;
	++ShotsFired;

	// A real arrow with a deliberately pitiful damage profile.
	const FVector MuzzleLocation = TowerLocation + FVector(0.0f, 0.0f, GetActorScale3D().Z * 50.0f);
	const FVector AimPoint = Target->GetActorLocation() + FVector(0.0f, 0.0f, 40.0f);
	const FVector Direction = (AimPoint - MuzzleLocation).GetSafeNormal();

	FArchonShotPayload Shot;
	Shot.InstigatorTeamId = TeamId;
	Shot.InstigatorPlayerId = INDEX_NONE;
	Shot.WeaponClass = EArchonWeaponClass::None;
	Shot.DamageType = EArchonDamageType::Generic;
	Shot.ShotOrigin = MuzzleLocation;
	Shot.ShotDirection = Direction;
	Shot.HitLocation = AimPoint;
	Shot.DistanceTraveled = NearestDistance;
	Shot.HitType = EArchonHitType::Body;

	FArchonWeaponDamageProfile TowerProfile;
	TowerProfile.BodyDamage = DamagePerShot;
	TowerProfile.HeadDamage = DamagePerShot;
	TowerProfile.LimbDamage = DamagePerShot;
	TowerProfile.MinDamage = DamagePerShot;
	TowerProfile.FalloffStartUnits = TargetRange;
	TowerProfile.FalloffEndUnits = TargetRange * 2.0f;

	FActorSpawnParameters Params;
	Params.Owner = this;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AArchonArrowProjectile* Projectile = GetWorld()->SpawnActor<AArchonArrowProjectile>(
		AArchonArrowProjectile::StaticClass(), MuzzleLocation, Direction.Rotation(), Params);
	if (Projectile)
	{
		Projectile->ConfigureShot(Shot, TowerProfile);
	}

	// One marker per shot keeps the replay timeline honest about tower
	// pressure without flooding: slow cycle bounds the rate.
	UE_LOG(
		LogTemp,
		Display,
		TEXT("ArchonFactoryCanary: TowerFired team=%d target=%s distance=%.0f shots=%d"),
		TeamId,
		*Target->GetName(),
		NearestDistance,
		ShotsFired);
}

void AArchonBaseDefenseTowerActor::HandleHealthChanged(const FArchonDamageApplicationResult& DamageResult)
{
	if (HasAuthority() && Health && DamageResult.bAccepted && DamageResult.DamageApplied > 0.0f)
	{
		UE_LOG(
			LogTemp,
			Display,
			TEXT("ArchonFactoryCanary: DefenseTowerDamaged team=%d health=%.0f alive=%s"),
			TeamId,
			Health->GetCurrentHealth(),
			Health->IsAlive() ? TEXT("true") : TEXT("false"));

		// Siege pressure scores (stalemate fix, 2026-06-11): report the
		// hit so the attacker earns discounted match points.
		if (!MatchState)
		{
			for (TActorIterator<AArchonMatchStateActor> It(GetWorld()); It; ++It)
			{
				MatchState = *It;
				break;
			}
		}
		if (MatchState)
		{
			MatchState->NotifyTowerDamaged(this, DamageResult);
		}
	}

	if (TowerMaterial && Health)
	{
		const FLinearColor Charred = HealthyColor * 0.08f;
		const FLinearColor Tint = FMath::Lerp(Charred, HealthyColor, Health->GetHealthFraction());
		TowerMaterial->SetVectorParameterValue(TEXT("Color"), Tint);
		TowerMaterial->SetVectorParameterValue(TEXT("BaseColor"), Tint);
	}

	if (HealthBar && Health)
	{
		HealthBar->SetHealthFraction(Health->GetHealthFraction());
	}
}
