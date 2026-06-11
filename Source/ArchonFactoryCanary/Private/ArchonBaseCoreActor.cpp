#include "ArchonBaseCoreActor.h"
#include "ArchonCombatHealthComponent.h"
#include "ArchonWorldHealthBarComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

AArchonBaseCoreActor::AArchonBaseCoreActor()
{
	CoreMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BaseCore"));
	RootComponent = CoreMesh;

	ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeFinder.Succeeded())
	{
		CoreMesh->SetStaticMesh(CubeFinder.Object);
	}
	ConstructorHelpers::FObjectFinder<UMaterialInterface> MaterialFinder(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (MaterialFinder.Succeeded())
	{
		CoreMesh->SetMaterial(0, MaterialFinder.Object);
	}
	CoreMesh->SetCollisionProfileName(TEXT("BlockAll"));

	Health = CreateDefaultSubobject<UArchonCombatHealthComponent>(TEXT("CoreHealth"));

	// Core pressure reads at a glance from anywhere on the field (WC3
	// structure-bar readability; chars-toward-black tint stays too).
	HealthBar = CreateDefaultSubobject<UArchonWorldHealthBarComponent>(TEXT("CoreHealthBar"));
	HealthBar->SetupAttachment(CoreMesh);
	HealthBar->SetUsingAbsoluteScale(true);
	HealthBar->SetUsingAbsoluteRotation(true);
	HealthBar->SetRelativeLocation(FVector(0.0f, 0.0f, 130.0f));

	bReplicates = true;
	bAlwaysRelevant = true;

	Tags.Add(TEXT("ArchonBaseCore"));
}

void AArchonBaseCoreActor::ConfigureCore(const FArchonValleyPlacement& Placement, float MaxHealth)
{
	PieceId = Placement.PieceId;
	TeamId = Placement.TeamId;
	HealthyColor = Placement.DebugColor;

	SetActorScale3D(Placement.Scale);

	if (CoreMesh)
	{
		CoreMaterial = CoreMesh->CreateDynamicMaterialInstance(0);
	}

	if (Health)
	{
		Health->ConfigureHealth(Placement.TeamId, MaxHealth, 1.0f);
		Health->OnHealthChanged.AddUObject(this, &AArchonBaseCoreActor::HandleHealthChanged);
	}

	if (HealthBar)
	{
		HealthBar->ConfigureBarSize(340.0f, 30.0f);
		HealthBar->SetHealthFraction(1.0f);
	}

	RefreshDamageTint();
}

void AArchonBaseCoreActor::HandleHealthChanged(const FArchonDamageApplicationResult& DamageResult)
{
	if (HasAuthority() && Health && DamageResult.bAccepted && DamageResult.DamageApplied > 0.0f)
	{
		UE_LOG(
			LogTemp,
			Display,
			TEXT("ArchonFactoryCanary: BaseCoreDamaged team=%d damage=%.0f health=%.0f alive=%s"),
			TeamId,
			DamageResult.DamageApplied,
			Health->GetCurrentHealth(),
			Health->IsAlive() ? TEXT("true") : TEXT("false"));
	}

	if (HealthBar && Health)
	{
		HealthBar->SetHealthFraction(Health->GetHealthFraction());
	}

	RefreshDamageTint();
}

void AArchonBaseCoreActor::RefreshDamageTint()
{
	if (!CoreMaterial || !Health)
	{
		return;
	}

	// Char toward near-black as the core dies; keep a hint of faction hue
	// so the burning core still reads as ours/theirs.
	const float HealthFraction = Health->GetHealthFraction();
	const FLinearColor Charred = HealthyColor * 0.08f;
	const FLinearColor Tint = FMath::Lerp(Charred, HealthyColor, HealthFraction);
	CoreMaterial->SetVectorParameterValue(TEXT("Color"), Tint);
	CoreMaterial->SetVectorParameterValue(TEXT("BaseColor"), Tint);
}
