#include "ArchonPressureBoltProjectile.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"

AArchonPressureBoltProjectile::AArchonPressureBoltProjectile()
{
	if (UProjectileMovementComponent* ProjectileMovement = GetProjectileMovement())
	{
		ProjectileMovement->InitialSpeed = 6000.0f;
		ProjectileMovement->MaxSpeed = 6000.0f;
		ProjectileMovement->ProjectileGravityScale = 0.18f;
	}

	if (UStaticMeshComponent* Mesh = GetProjectileMesh())
	{
		Mesh->SetWorldScale3D(FVector(0.06f, 0.06f, 0.50f));
		Mesh->SetRenderCustomDepth(true);
		Mesh->SetCustomDepthStencilValue(3);
	}
}
