#include "ArchonTechBuildingActor.h"
#include "ArchonCombatHealthComponent.h"
#include "ArchonMatchStateActor.h"
#include "ArchonWorldHealthBarComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

AArchonTechBuildingActor::AArchonTechBuildingActor()
{
	BuildingMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TechBuilding"));
	RootComponent = BuildingMesh;

	ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeFinder.Succeeded())
	{
		BuildingMesh->SetStaticMesh(CubeFinder.Object);
	}
	ConstructorHelpers::FObjectFinder<UMaterialInterface> MaterialFinder(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (MaterialFinder.Succeeded())
	{
		BuildingMesh->SetMaterial(0, MaterialFinder.Object);
	}
	BuildingMesh->SetCollisionProfileName(TEXT("BlockAll"));

	Health = CreateDefaultSubobject<UArchonCombatHealthComponent>(TEXT("TechBuildingHealth"));

	HealthBar = CreateDefaultSubobject<UArchonWorldHealthBarComponent>(TEXT("TechBuildingHealthBar"));
	HealthBar->SetupAttachment(BuildingMesh);
	HealthBar->SetUsingAbsoluteScale(true);
	HealthBar->SetUsingAbsoluteRotation(true);
	HealthBar->SetRelativeLocation(FVector(0.0f, 0.0f, 140.0f));

	bReplicates = true;
	bAlwaysRelevant = true;

	Tags.Add(TEXT("ArchonTechBuilding"));
}

void AArchonTechBuildingActor::ConfigureTechBuilding(
	int32 InTeamId,
	FName InTechId,
	AArchonMatchStateActor* InMatchState,
	FLinearColor InTeamColor)
{
	TeamId = InTeamId;
	TechId = InTechId;
	MatchState = InMatchState;
	TeamColor = InTeamColor;
	SetActorScale3D(FVector(5.0f, 4.0f, 3.0f));

	if (BuildingMesh)
	{
		BuildingMaterial = BuildingMesh->CreateDynamicMaterialInstance(0);
	}

	if (Health)
	{
		Health->ConfigureHealth(TeamId, MaxHealth, 1.0f);
		Health->OnHealthChanged.AddUObject(this, &AArchonTechBuildingActor::HandleHealthChanged);
	}

	if (HealthBar)
	{
		HealthBar->ConfigureBarSize(280.0f, 26.0f);
		HealthBar->SetHealthFraction(1.0f);
	}

	RefreshDamageTint();
	UE_LOG(
		LogTemp,
		Display,
		TEXT("ArchonFactoryCanary: TechBuildingPlaced team=%d tech=%s health=%.0f location=%s"),
		TeamId,
		*TechId.ToString(),
		MaxHealth,
		*GetActorLocation().ToCompactString());
}

void AArchonTechBuildingActor::HandleHealthChanged(const FArchonDamageApplicationResult& DamageResult)
{
	if (HasAuthority() && Health && DamageResult.bAccepted && DamageResult.DamageApplied > 0.0f)
	{
		UE_LOG(
			LogTemp,
			Display,
			TEXT("ArchonFactoryCanary: TechBuildingDamaged team=%d tech=%s damage=%.0f health=%.0f alive=%s"),
			TeamId,
			*TechId.ToString(),
			DamageResult.DamageApplied,
			Health->GetCurrentHealth(),
			Health->IsAlive() ? TEXT("true") : TEXT("false"));

		if (DamageResult.bCausedDeath && !bRemovedTechAfterDeath)
		{
			bRemovedTechAfterDeath = true;
			if (MatchState)
			{
				MatchState->RemoveBuiltTech(TeamId, TechId);
			}
			UE_LOG(
				LogTemp,
				Display,
				TEXT("ArchonFactoryCanary: TechBuildingDestroyed team=%d tech=%s"),
				TeamId,
				*TechId.ToString());
		}
	}

	if (HealthBar && Health)
	{
		HealthBar->SetHealthFraction(Health->GetHealthFraction());
	}

	RefreshDamageTint();
}

void AArchonTechBuildingActor::RefreshDamageTint()
{
	if (!BuildingMaterial || !Health)
	{
		return;
	}

	const FLinearColor Charred = TeamColor * 0.08f;
	const FLinearColor Tint = FMath::Lerp(Charred, TeamColor, Health->GetHealthFraction());
	BuildingMaterial->SetVectorParameterValue(TEXT("Color"), Tint);
	BuildingMaterial->SetVectorParameterValue(TEXT("BaseColor"), Tint);
}
