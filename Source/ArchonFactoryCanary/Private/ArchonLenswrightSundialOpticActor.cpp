#include "ArchonLenswrightSundialOpticActor.h"
#include "ArchonAiCombatBehaviorComponent.h"
#include "ArchonCombatHealthComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	UStaticMesh* TryFindSundialBasicShape(const TCHAR* Path)
	{
		ConstructorHelpers::FObjectFinder<UStaticMesh> Finder(Path);
		return Finder.Succeeded() ? Finder.Object : nullptr;
	}

	UMaterialInterface* TryFindSundialBasicMaterial()
	{
		ConstructorHelpers::FObjectFinder<UMaterialInterface> Finder(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
		return Finder.Succeeded() ? Finder.Object : nullptr;
	}

	void ApplySundialDebugMaterial(UStaticMeshComponent* Component, const FLinearColor& Color)
	{
		if (!Component)
		{
			return;
		}

		if (UMaterialInterface* BasicMaterial = TryFindSundialBasicMaterial())
		{
			Component->SetMaterial(0, BasicMaterial);
		}

		if (UMaterialInstanceDynamic* DynamicMaterial = Component->CreateDynamicMaterialInstance(0))
		{
			DynamicMaterial->SetVectorParameterValue(TEXT("Color"), Color);
			DynamicMaterial->SetVectorParameterValue(TEXT("BaseColor"), Color);
		}
	}
}

AArchonLenswrightSundialOpticActor::AArchonLenswrightSundialOpticActor()
{
	PrimaryActorTick.bCanEverTick = false;

	GetCapsuleComponent()->InitCapsuleSize(34.0f, 105.0f);
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->MaxWalkSpeed = 420.0f;
	}

	PlaceholderMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SundialOpticPlaceholderMesh"));
	PlaceholderMesh->SetupAttachment(GetCapsuleComponent());
	PlaceholderMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 8.0f));
	PlaceholderMesh->SetRelativeScale3D(FVector(0.45f, 0.45f, 1.55f));
	PlaceholderMesh->SetCollisionProfileName(TEXT("NoCollision"));
	if (UStaticMesh* Cylinder = TryFindSundialBasicShape(TEXT("/Engine/BasicShapes/Cylinder.Cylinder")))
	{
		PlaceholderMesh->SetStaticMesh(Cylinder);
	}
	ApplySundialDebugMaterial(PlaceholderMesh, DebugColor);

	Health = CreateDefaultSubobject<UArchonCombatHealthComponent>(TEXT("Health"));
	CombatBehavior = CreateDefaultSubobject<UArchonAiCombatBehaviorComponent>(TEXT("CombatBehavior"));
	ConfigureSundial(TeamId, UnitId);
}

void AArchonLenswrightSundialOpticActor::ConfigureSundial(int32 InTeamId, FName InUnitId)
{
	TeamId = InTeamId;
	UnitId = InUnitId.IsNone() ? FName(TEXT("lenswright_sundial_optic")) : InUnitId;
	VisionRadius = 4500.0f;

	if (Health)
	{
		Health->ConfigureHealth(TeamId, 60.0f, 1.0f);
	}

	if (CombatBehavior)
	{
		CombatBehavior->ConfigureBehavior(TeamId, EArchonAiCombatRole::ScoutNoWeapon);
	}
}
