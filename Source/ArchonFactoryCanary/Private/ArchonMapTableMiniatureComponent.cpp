#include "ArchonMapTableMiniatureComponent.h"
#include "ArchonBaseCoreActor.h"
#include "ArchonBaseDefenseTowerActor.h"
#include "ArchonCanaryFpsCharacter.h"
#include "ArchonCanaryRtsSquadActor.h"
#include "ArchonCombatHealthComponent.h"
#include "ArchonMapTableMiniatureLibrary.h"
#include "ArchonMatchStateActor.h"
#include "ArchonTeamVisibilityStateComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace
{
	const TCHAR* SpherePath = TEXT("/Engine/BasicShapes/Sphere.Sphere");
	const TCHAR* CubePath = TEXT("/Engine/BasicShapes/Cube.Cube");
	const TCHAR* CylinderPath = TEXT("/Engine/BasicShapes/Cylinder.Cylinder");

	const FLinearColor FriendlyColor(0.15f, 0.9f, 0.25f);
	const FLinearColor EnemyColor(0.95f, 0.15f, 0.1f);
	const FLinearColor NeutralColor(0.65f, 0.65f, 0.6f);
}

UArchonMapTableMiniatureComponent::UArchonMapTableMiniatureComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UArchonMapTableMiniatureComponent::ConfigureMiniature(
	int32 InViewingTeamId,
	UArchonTeamVisibilityStateComponent* InVisibilityState)
{
	ViewingTeamId = InViewingTeamId;
	VisibilityState = InVisibilityState;
}

void UArchonMapTableMiniatureComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	RefreshAccumulator += DeltaTime;
	if (RefreshAccumulator < RefreshIntervalSeconds)
	{
		return;
	}
	RefreshAccumulator = 0.0f;
	RefreshBoard();
}

UStaticMeshComponent* UArchonMapTableMiniatureComponent::AcquireMarker(
	int32& PoolCursor,
	const TCHAR* MeshPath,
	float UniformScale)
{
	UStaticMeshComponent* Marker = nullptr;
	if (MarkerPool.IsValidIndex(PoolCursor))
	{
		Marker = MarkerPool[PoolCursor];
	}
	else
	{
		Marker = NewObject<UStaticMeshComponent>(GetOwner());
		Marker->SetupAttachment(this);
		Marker->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Marker->SetCastShadow(false);
		Marker->RegisterComponent();
		MarkerPool.Add(Marker);
	}
	++PoolCursor;

	if (Marker)
	{
		if (UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, MeshPath))
		{
			if (Marker->GetStaticMesh() != Mesh)
			{
				Marker->SetStaticMesh(Mesh);
			}
		}
		Marker->SetRelativeScale3D(FVector(UniformScale));
		Marker->SetVisibility(true);
	}
	return Marker;
}

void UArchonMapTableMiniatureComponent::PlaceMarker(
	UStaticMeshComponent* Marker,
	const FVector& WorldPoint,
	const FLinearColor& Color,
	float ZLift)
{
	if (!Marker)
	{
		return;
	}
	FVector Local = UArchonMapTableMiniatureLibrary::WorldToTableLocal(
		WorldPoint, WorldOrigin, WorldExtents, TableHalfX, TableHalfY);
	Local.Z = ZLift;
	Marker->SetRelativeLocation(Local);
	if (UMaterialInstanceDynamic* Material = Marker->CreateDynamicMaterialInstance(0))
	{
		Material->SetVectorParameterValue(TEXT("Color"), Color);
		Material->SetVectorParameterValue(TEXT("BaseColor"), Color);
	}
	++VisibleMarkerCount;
}

void UArchonMapTableMiniatureComponent::RefreshBoard()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	VisibleMarkerCount = 0;
	int32 PoolCursor = 0;
	const TArray<FArchonVisibilityGridCell> Cells =
		VisibilityState ? VisibilityState->GetVisibilityCells() : TArray<FArchonVisibilityGridCell>();
	const auto EnemyVisible = [&Cells](const FVector& Location)
	{
		return UArchonMapTableMiniatureLibrary::IsWorldPointLit(Cells, Location);
	};

	// Cores: cubes, always shown (WC3 building snapshots; bases are
	// known landmarks even under fog).
	for (TActorIterator<AArchonBaseCoreActor> It(World); It; ++It)
	{
		const bool bFriendly = It->GetTeamId() == ViewingTeamId;
		PlaceMarker(
			AcquireMarker(PoolCursor, CubePath, 0.10f),
			It->GetActorLocation(), bFriendly ? FriendlyColor : EnemyColor, 5.0f);
	}

	// Towers: cylinders; landmarks too, but dead ones leave the board.
	for (TActorIterator<AArchonBaseDefenseTowerActor> It(World); It; ++It)
	{
		if (!It->GetHealth() || !It->GetHealth()->IsAlive())
		{
			continue;
		}
		const bool bFriendly = It->GetTeamId() == ViewingTeamId;
		PlaceMarker(
			AcquireMarker(PoolCursor, CylinderPath, 0.07f),
			It->GetActorLocation(), bFriendly ? FriendlyColor : EnemyColor, 7.0f);
	}

	// Sites: flat neutral/owner-tinted pads.
	for (TActorIterator<AArchonMatchStateActor> It(World); It; ++It)
	{
		const int32 SiteCount = It->GetSiteCount();
		for (int32 Index = 0; Index < SiteCount; ++Index)
		{
			FVector SiteLocation;
			int32 OwningTeam = INDEX_NONE;
			if (!It->GetSiteInfo(Index, SiteLocation, OwningTeam))
			{
				continue;
			}
			const FLinearColor PadColor =
				OwningTeam == INDEX_NONE ? NeutralColor :
				OwningTeam == ViewingTeamId ? FriendlyColor : EnemyColor;
			UStaticMeshComponent* Marker = AcquireMarker(PoolCursor, CubePath, 0.08f);
			if (Marker)
			{
				Marker->SetRelativeScale3D(FVector(0.08f, 0.08f, 0.012f));
			}
			PlaceMarker(Marker, SiteLocation, PadColor, 1.5f);
		}
		break;
	}

	// Bodies on the field: spheres. Friendly always; enemy only if a
	// lit cell covers them (live fog read — the RTS truth source).
	for (TActorIterator<AArchonCanaryFpsCharacter> It(World); It; ++It)
	{
		UArchonCombatHealthComponent* Health = It->GetHealth();
		if (!Health || !Health->IsAlive())
		{
			continue;
		}
		const bool bFriendly = Health->GetTeamId() == ViewingTeamId;
		if (!bFriendly && !EnemyVisible(It->GetActorLocation()))
		{
			continue;
		}
		PlaceMarker(
			AcquireMarker(PoolCursor, SpherePath, 0.045f),
			It->GetActorLocation(), bFriendly ? FriendlyColor : EnemyColor, 4.0f);
	}

	// Ordered RTS squads: slightly larger spheres (the things the
	// table commands).
	for (TActorIterator<AArchonCanaryRtsSquadActor> It(World); It; ++It)
	{
		const bool bFriendly = It->GetTeamId() == ViewingTeamId;
		if (!bFriendly && !EnemyVisible(It->GetActorLocation()))
		{
			continue;
		}
		PlaceMarker(
			AcquireMarker(PoolCursor, SpherePath, 0.06f),
			It->GetActorLocation(), bFriendly ? FriendlyColor : EnemyColor, 5.0f);
	}

	// Park the unused tail of the pool.
	for (int32 Index = PoolCursor; Index < MarkerPool.Num(); ++Index)
	{
		if (MarkerPool[Index])
		{
			MarkerPool[Index]->SetVisibility(false);
		}
	}

	if (!bLoggedFirstRefresh && VisibleMarkerCount > 0)
	{
		bLoggedFirstRefresh = true;
		UE_LOG(
			LogTemp,
			Display,
			TEXT("ArchonFactoryCanary: MapTableMiniatureLive team=%d markers=%d"),
			ViewingTeamId,
			VisibleMarkerCount);
	}
}
