#include "ArchonCanaryFpsCharacter.h"
#include "ArchonCanaryWorldSubsystem.h"
#include "ArchonCombatHealthComponent.h"
#include "ArchonFactionMovementComponent.h"
#include "ArchonVerdantThornsproutBow.h"
#include "Animation/AnimSequence.h"
#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "NavigationInvokerComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	FGenericTeamId ToArchonGenericTeamId(int32 TeamId)
	{
		if (TeamId < 0 || TeamId >= FGenericTeamId::NoTeam.GetId())
		{
			return FGenericTeamId::NoTeam;
		}
		return FGenericTeamId(static_cast<uint8>(TeamId));
	}
}

AArchonCanaryFpsCharacter::AArchonCanaryFpsCharacter()
{
	PrimaryActorTick.bCanEverTick = false;

	GetCapsuleComponent()->InitCapsuleSize(42.0f, 96.0f);
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = true;
	bUseControllerRotationRoll = false;

	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->bOrientRotationToMovement = false;
	Movement->MaxWalkSpeed = 600.0f;
	Movement->MaxWalkSpeedCrouched = 300.0f;
	Movement->JumpZVelocity = 420.0f;
	Movement->AirControl = 0.35f;
	Movement->GetNavAgentPropertiesRef().bCanCrouch = true;

	FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCamera->SetupAttachment(GetCapsuleComponent());
	FirstPersonCamera->SetRelativeLocation(FVector(0.0f, 0.0f, 64.0f));
	FirstPersonCamera->bUsePawnControlRotation = true;

	FactionMovement = CreateDefaultSubobject<UArchonFactionMovementComponent>(TEXT("FactionMovement"));
	FactionMovement->ConfigureFaction(EArchonFaction::VerdantChoir);

	VerdantBow = CreateDefaultSubobject<UArchonVerdantThornsproutBow>(TEXT("VerdantThornsproutBow"));
	VerdantBow->ConfigureWeapon(0, 0, VerdantBow->GetStats());

	Health = CreateDefaultSubobject<UArchonCombatHealthComponent>(TEXT("Health"));
	Health->ConfigureHealth(0, 150.0f, 1.0f);

	// Every player body invokes dynamic NavMesh generation around itself
	// so stock UE pathfinding works on the code-spawned valley (bots were
	// piling up on edge blocks with hand-rolled steering, 2026-06-10).
	NavInvoker = CreateDefaultSubobject<UNavigationInvokerComponent>(TEXT("NavInvoker"));
	NavInvoker->SetGenerationRadii(6000.0f, 8000.0f);

	// Third-person view (V toggle): over-the-shoulder boom, inactive until
	// toggled. The engine uses the first ACTIVE camera component.
	ThirdPersonBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("ThirdPersonBoom"));
	ThirdPersonBoom->SetupAttachment(GetCapsuleComponent());
	ThirdPersonBoom->TargetArmLength = 350.0f;
	ThirdPersonBoom->SocketOffset = FVector(0.0f, 50.0f, 70.0f);
	ThirdPersonBoom->bUsePawnControlRotation = true;

	ThirdPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("ThirdPersonCamera"));
	ThirdPersonCamera->SetupAttachment(ThirdPersonBoom, USpringArmComponent::SocketName);
	ThirdPersonCamera->bUsePawnControlRotation = false;
	ThirdPersonCamera->SetAutoActivate(false);

	// Actual character body (Jonathan 2026-06-10: "characters should be
	// actual characters") — the engine's own mannequin + locomotion ABP,
	// copied from the engine template pack into /Game/Characters.
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> MannyFinder(TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple"));
	if (MannyFinder.Succeeded())
	{
		GetMesh()->SetSkeletalMesh(MannyFinder.Object);
		GetMesh()->SetRelativeLocation(FVector(0.0f, 0.0f, -96.0f));
		GetMesh()->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
	}
	static ConstructorHelpers::FClassFinder<UAnimInstance> UnarmedAbpFinder(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed"));
	if (UnarmedAbpFinder.Succeeded())
	{
		GetMesh()->SetAnimInstanceClass(UnarmedAbpFinder.Class);
	}
	// First person never shows your own body (Jonathan 2026-06-10);
	// ToggleCameraView flips this for third person.
	GetMesh()->SetOwnerNoSee(true);

	// Team identity orb above the head: faction color readable from the
	// match view and through crowds (mannequin materials carry no team).
	BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TeamOrb"));
	BodyMesh->SetupAttachment(GetCapsuleComponent());
	static ConstructorHelpers::FObjectFinder<UStaticMesh> OrbFinder(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (OrbFinder.Succeeded())
	{
		BodyMesh->SetStaticMesh(OrbFinder.Object);
	}
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> OrbMaterialFinder(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (OrbMaterialFinder.Succeeded())
	{
		BodyMesh->SetMaterial(0, OrbMaterialFinder.Object);
	}
	BodyMesh->SetRelativeScale3D(FVector(0.22f, 0.22f, 0.22f));
	BodyMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 122.0f));
	BodyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BodyMesh->SetOwnerNoSee(true);
}

void AArchonCanaryFpsCharacter::SetGenericTeamId(const FGenericTeamId& NewTeamId)
{
	if (Health && NewTeamId.GetId() != FGenericTeamId::NoTeam.GetId())
	{
		Health->SetTeamId(static_cast<int32>(NewTeamId.GetId()));
	}
}

FGenericTeamId AArchonCanaryFpsCharacter::GetGenericTeamId() const
{
	return Health ? ToArchonGenericTeamId(Health->GetTeamId()) : FGenericTeamId::NoTeam;
}

ETeamAttitude::Type AArchonCanaryFpsCharacter::GetTeamAttitudeTowards(const AActor& Other) const
{
	const IGenericTeamAgentInterface* OtherTeamAgent = Cast<const IGenericTeamAgentInterface>(&Other);
	return OtherTeamAgent
		? FGenericTeamId::GetAttitude(GetGenericTeamId(), OtherTeamAgent->GetGenericTeamId())
		: ETeamAttitude::Neutral;
}

void AArchonCanaryFpsCharacter::SetBodyDead(bool bDead)
{
	USkeletalMeshComponent* Body = GetMesh();
	if (!Body)
	{
		return;
	}

	if (bDead)
	{
		// Authored death anim first (the mannequin pack ships six; they
		// end grounded and the single-node instance holds the last frame
		// through the respawn linger). Ragdoll stays as the fallback if
		// the anims are ever missing — proofs never depend on content.
		static const TCHAR* DeathAnimPaths[] = {
			TEXT("/Game/Characters/Mannequins/Anims/Death/MM_Death_Front_01.MM_Death_Front_01"),
			TEXT("/Game/Characters/Mannequins/Anims/Death/MM_Death_Front_02.MM_Death_Front_02"),
			TEXT("/Game/Characters/Mannequins/Anims/Death/MM_Death_Front_03.MM_Death_Front_03"),
			TEXT("/Game/Characters/Mannequins/Anims/Death/MM_Death_Back_01.MM_Death_Back_01"),
			TEXT("/Game/Characters/Mannequins/Anims/Death/MM_Death_Left_01.MM_Death_Left_01"),
			TEXT("/Game/Characters/Mannequins/Anims/Death/MM_Death_Right_01.MM_Death_Right_01")
		};
		static uint32 DeathVariety = 0;
		UAnimSequence* DeathAnim = LoadObject<UAnimSequence>(
			nullptr, DeathAnimPaths[(DeathVariety++ + GetTypeHash(GetFName())) % 6]);
		if (DeathAnim)
		{
			Body->SetSimulatePhysics(false);
			Body->PlayAnimation(DeathAnim, /*bLooping=*/false);
		}
		else
		{
			// Fallback: PA_Mannequin physics flop.
			Body->SetCollisionProfileName(TEXT("Ragdoll"));
			Body->SetSimulatePhysics(true);
		}
		GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	}
	else
	{
		Body->SetSimulatePhysics(false);
		Body->SetCollisionProfileName(TEXT("CharacterMesh"));
		Body->AttachToComponent(GetCapsuleComponent(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		Body->SetRelativeLocation(FVector(0.0f, 0.0f, -96.0f));
		Body->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
		// Hand the body back to the locomotion ABP (PlayAnimation flips
		// the mesh into single-node mode; the instance class is intact).
		Body->SetAnimationMode(EAnimationMode::AnimationBlueprint);
		GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	}
}

void AArchonCanaryFpsCharacter::SetBodyTeamColor(const FLinearColor& TeamColor)
{
	if (!BodyMesh)
	{
		return;
	}
	if (!BodyMaterial)
	{
		BodyMaterial = BodyMesh->CreateDynamicMaterialInstance(0);
	}
	if (BodyMaterial)
	{
		BodyMaterial->SetVectorParameterValue(TEXT("Color"), TeamColor);
		BodyMaterial->SetVectorParameterValue(TEXT("BaseColor"), TeamColor);
	}
}

bool AArchonCanaryFpsCharacter::ToggleCameraView()
{
	if (!FirstPersonCamera || !ThirdPersonCamera)
	{
		return bFirstPersonView;
	}

	bFirstPersonView = !bFirstPersonView;
	FirstPersonCamera->SetActive(bFirstPersonView);
	ThirdPersonCamera->SetActive(!bFirstPersonView);
	// Own body hidden in first person, visible over the shoulder.
	GetMesh()->SetOwnerNoSee(bFirstPersonView);
	if (BodyMesh)
	{
		BodyMesh->SetOwnerNoSee(bFirstPersonView);
	}

	UE_LOG(
		LogTemp,
		Display,
		TEXT("ArchonFactoryCanary: CameraViewToggled view=%s pawn=%s"),
		bFirstPersonView ? TEXT("first") : TEXT("third"),
		*GetName());
	return bFirstPersonView;
}

void AArchonCanaryFpsCharacter::ServerFireBow_Implementation(FVector Origin, FVector Direction)
{
	const bool bFired = VerdantBow && VerdantBow->TryFire(Origin, Direction);
	UE_LOG(
		LogTemp,
		Display,
		TEXT("ArchonFactoryCanary: ServerWeaponFireReceived pawn=%s fired=%s ammo=%d"),
		*GetName(),
		bFired ? TEXT("true") : TEXT("false"),
		VerdantBow ? VerdantBow->GetState().CurrentAmmo : -1);
}

void AArchonCanaryFpsCharacter::ServerReloadBow_Implementation()
{
	const bool bReloaded = VerdantBow && VerdantBow->TryReload();
	UE_LOG(
		LogTemp,
		Display,
		TEXT("ArchonFactoryCanary: ServerWeaponReloadReceived pawn=%s reloaded=%s"),
		*GetName(),
		bReloaded ? TEXT("true") : TEXT("false"));
}

void AArchonCanaryFpsCharacter::ServerSubmitRtsOrder_Implementation(EArchonRtsOrderKind OrderKind)
{
	UWorld* World = GetWorld();
	UArchonCanaryWorldSubsystem* Subsystem = World ? World->GetSubsystem<UArchonCanaryWorldSubsystem>() : nullptr;
	const bool bSubmitted = Subsystem && Subsystem->SubmitNetworkedRtsOrder(OrderKind);
	UE_LOG(
		LogTemp,
		Display,
		TEXT("ArchonFactoryCanary: ServerRtsOrderReceived pawn=%s order=%d submitted=%s"),
		*GetName(),
		static_cast<int32>(OrderKind),
		bSubmitted ? TEXT("true") : TEXT("false"));
}
