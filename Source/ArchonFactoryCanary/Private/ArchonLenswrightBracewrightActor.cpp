#include "ArchonLenswrightBracewrightActor.h"
#include "ArchonAiCombatBehaviorComponent.h"
#include "ArchonCombatHealthComponent.h"
#include "ArchonLenswrightPressureBoltCrossbow.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	UStaticMesh* TryFindLenswrightBasicShape(const TCHAR* Path)
	{
		ConstructorHelpers::FObjectFinder<UStaticMesh> Finder(Path);
		return Finder.Succeeded() ? Finder.Object : nullptr;
	}

	UMaterialInterface* TryFindLenswrightBasicMaterial()
	{
		ConstructorHelpers::FObjectFinder<UMaterialInterface> Finder(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
		return Finder.Succeeded() ? Finder.Object : nullptr;
	}

	void ApplyLenswrightDebugMaterial(UStaticMeshComponent* Component, const FLinearColor& Color)
	{
		if (!Component)
		{
			return;
		}

		if (UMaterialInterface* BasicMaterial = TryFindLenswrightBasicMaterial())
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

AArchonLenswrightBracewrightActor::AArchonLenswrightBracewrightActor()
{
	PrimaryActorTick.bCanEverTick = false;

	GetCapsuleComponent()->InitCapsuleSize(38.0f, 92.0f);
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->MaxWalkSpeed = 360.0f;
	}

	PlaceholderMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BracewrightPlaceholderMesh"));
	PlaceholderMesh->SetupAttachment(GetCapsuleComponent());
	PlaceholderMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -10.0f));
	PlaceholderMesh->SetRelativeScale3D(FVector(0.65f, 0.45f, 1.25f));
	PlaceholderMesh->SetCollisionProfileName(TEXT("NoCollision"));
	if (UStaticMesh* Cube = TryFindLenswrightBasicShape(TEXT("/Engine/BasicShapes/Cube.Cube")))
	{
		PlaceholderMesh->SetStaticMesh(Cube);
	}
	ApplyLenswrightDebugMaterial(PlaceholderMesh, DebugColor);

	Health = CreateDefaultSubobject<UArchonCombatHealthComponent>(TEXT("Health"));
	Crossbow = CreateDefaultSubobject<UArchonLenswrightPressureBoltCrossbow>(TEXT("PressureBoltCrossbow"));
	CombatBehavior = CreateDefaultSubobject<UArchonAiCombatBehaviorComponent>(TEXT("CombatBehavior"));
	ConfigureBracewright(TeamId, UnitId);
}

void AArchonLenswrightBracewrightActor::ConfigureBracewright(int32 InTeamId, FName InUnitId)
{
	TeamId = InTeamId;
	UnitId = InUnitId.IsNone() ? FName(TEXT("lenswright_bracewright")) : InUnitId;

	if (Health)
	{
		Health->ConfigureHealth(TeamId, 80.0f, 1.0f);
	}

	if (Crossbow)
	{
		Crossbow->ConfigureWeapon(TeamId, INDEX_NONE, Crossbow->GetStats());
	}

	if (CombatBehavior)
	{
		CombatBehavior->ConfigureBehavior(TeamId, EArchonAiCombatRole::DefensiveRanged);
	}
}
