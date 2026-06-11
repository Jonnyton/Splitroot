#include "ArchonWorldHealthBarComponent.h"
#include "ArchonHealthBarPolicyLibrary.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	// Engine cube is 100 units on a side at scale 1.
	constexpr float CubeUnits = 100.0f;
}

UArchonWorldHealthBarComponent::UArchonWorldHealthBarComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> MaterialFinder(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

	BackPlate = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HealthBarBack"));
	BackPlate->SetupAttachment(this);
	Fill = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HealthBarFill"));
	Fill->SetupAttachment(this);

	for (UStaticMeshComponent* Piece : { BackPlate.Get(), Fill.Get() })
	{
		if (!Piece)
		{
			continue;
		}
		if (CubeFinder.Succeeded())
		{
			Piece->SetStaticMesh(CubeFinder.Object);
		}
		if (MaterialFinder.Succeeded())
		{
			Piece->SetMaterial(0, MaterialFinder.Object);
		}
		// Pure presentation: never blocks arrows, bots, or players.
		Piece->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Piece->SetCastShadow(false);
	}
}

void UArchonWorldHealthBarComponent::ConfigureBarSize(float InWidthUnits, float InThicknessUnits)
{
	WidthUnits = FMath::Max(1.0f, InWidthUnits);
	ThicknessUnits = FMath::Max(1.0f, InThicknessUnits);

	if (BackPlate)
	{
		BackMaterial = BackPlate->CreateDynamicMaterialInstance(0);
		if (BackMaterial)
		{
			const FLinearColor Backing(0.02f, 0.02f, 0.02f);
			BackMaterial->SetVectorParameterValue(TEXT("Color"), Backing);
			BackMaterial->SetVectorParameterValue(TEXT("BaseColor"), Backing);
		}
	}
	if (Fill)
	{
		FillMaterial = Fill->CreateDynamicMaterialInstance(0);
	}
	RefreshBar();
}

void UArchonWorldHealthBarComponent::SetHealthFraction(float InHealthFraction)
{
	HealthFraction = InHealthFraction;
	RefreshBar();
}

void UArchonWorldHealthBarComponent::RefreshBar()
{
	const float FillScale = UArchonHealthBarPolicyLibrary::ComputeHealthBarFillScale(HealthFraction);
	const float Depth = ThicknessUnits / CubeUnits;

	if (BackPlate)
	{
		BackPlate->SetRelativeScale3D(FVector(WidthUnits / CubeUnits, Depth * 0.5f, Depth));
		BackPlate->SetRelativeLocation(FVector::ZeroVector);
	}
	if (Fill)
	{
		// Anchor the fill's left edge to the bar's left edge so it drains
		// toward the right, WC3-style, instead of shrinking from both ends.
		const float FillWidth = WidthUnits * FillScale;
		Fill->SetRelativeScale3D(FVector(FMath::Max(FillWidth, KINDA_SMALL_NUMBER) / CubeUnits, Depth * 0.6f, Depth * 0.8f));
		Fill->SetRelativeLocation(FVector((FillWidth - WidthUnits) * 0.5f, -ThicknessUnits * 0.05f, 0.0f));
		Fill->SetVisibility(FillScale > 0.0f);
	}
	if (FillMaterial)
	{
		const FLinearColor BandColor = UArchonHealthBarPolicyLibrary::ComputeHealthBarColor(HealthFraction);
		FillMaterial->SetVectorParameterValue(TEXT("Color"), BandColor);
		FillMaterial->SetVectorParameterValue(TEXT("BaseColor"), BandColor);
	}
}

void UArchonWorldHealthBarComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Billboard yaw toward the local viewer (cheap: few structures per
	// map; no per-pixel work; headless smokes simply have no camera).
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	APlayerController* Viewer = World->GetFirstPlayerController();
	if (!Viewer || !Viewer->PlayerCameraManager)
	{
		return;
	}
	const FVector CameraLocation = Viewer->PlayerCameraManager->GetCameraLocation();
	const FVector ToCamera = CameraLocation - GetComponentLocation();
	if (ToCamera.IsNearlyZero())
	{
		return;
	}
	// The bar's width runs along X; face its Y (depth) axis at the viewer.
	const float Yaw = ToCamera.Rotation().Yaw + 90.0f;
	SetWorldRotation(FRotator(0.0f, Yaw, 0.0f));
}
