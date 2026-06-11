#include "ArchonRespawnObserverPawn.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/FloatingPawnMovement.h"

AArchonRespawnObserverPawn::AArchonRespawnObserverPawn()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	ObserverCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("ObserverCamera"));
	ObserverCamera->SetupAttachment(SceneRoot);
	ObserverCamera->bUsePawnControlRotation = true;

	FloatingMovement = CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("FloatingMovement"));
	SetActorEnableCollision(false);
}

void AArchonRespawnObserverPawn::ConfigureObserverFromDeathLocation(const FVector& DeathLocation)
{
	const FVector ObserverLocation = DeathLocation + FVector(-200.0f, 0.0f, 200.0f);
	const FRotator ObserverRotation = (DeathLocation - ObserverLocation).Rotation();
	SetActorLocationAndRotation(ObserverLocation, ObserverRotation, false, nullptr, ETeleportType::TeleportPhysics);
}
