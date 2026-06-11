#include "ArchonValleyBlockActor.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "PhysicsEngine/BodySetup.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	UStaticMesh* LoadFirstMesh(const TArray<const TCHAR*>& MeshPaths, FString& OutAssetPath)
	{
		for (const TCHAR* MeshPath : MeshPaths)
		{
			if (UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, MeshPath))
			{
				OutAssetPath = MeshPath;
				return Mesh;
			}
		}
		OutAssetPath.Reset();
		return nullptr;
	}

	FString OwnerName(const UStaticMeshComponent& Block)
	{
		return Block.GetOwner() ? Block.GetOwner()->GetName() : TEXT("none");
	}

	constexpr double PlayerHeightUnits = 192.0;

	FVector GetDesiredVisualDimensions(const FArchonValleyPlacement& Placement)
	{
		FVector DesiredDimensions = Placement.Scale * 100.0;
		switch (Placement.Piece)
		{
		case EArchonValleyPiece::RidgeSlab:
			DesiredDimensions.Y = FMath::Max(DesiredDimensions.Y, PlayerHeightUnits * 6.0);
			DesiredDimensions.Z = FMath::Max(DesiredDimensions.Z, PlayerHeightUnits * 8.0);
			break;
		case EArchonValleyPiece::KinwildCamp:
			DesiredDimensions.X = FMath::Max(DesiredDimensions.X, PlayerHeightUnits * 12.0);
			DesiredDimensions.Y = FMath::Max(DesiredDimensions.Y, PlayerHeightUnits * 12.0);
			DesiredDimensions.Z = FMath::Max(DesiredDimensions.Z, PlayerHeightUnits * 5.0);
			break;
		case EArchonValleyPiece::CoverStone:
			DesiredDimensions.X = FMath::Max(DesiredDimensions.X, PlayerHeightUnits * 1.4);
			DesiredDimensions.Y = FMath::Max(DesiredDimensions.Y, PlayerHeightUnits * 1.4);
			DesiredDimensions.Z = FMath::Max(DesiredDimensions.Z, PlayerHeightUnits * 1.0);
			break;
		default:
			break;
		}
		return DesiredDimensions;
	}

	void ApplyPlacementCollision(UStaticMeshComponent& Block, UStaticMesh& Mesh, const FArchonValleyPlacement& Placement)
	{
		if (Placement.Piece == EArchonValleyPiece::ResourceSite)
		{
			Block.SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Block.SetCanEverAffectNavigation(false);
			Block.RecreatePhysicsState();
			UE_LOG(
				LogTemp,
				Display,
				TEXT("ArchonFactoryCanary: ValleyFabCollision piece=%s id=%s mode=nonblocking_objective playerHeight=%.0f owner=%s"),
				*StaticEnum<EArchonValleyPiece>()->GetNameStringByValue(static_cast<int64>(Placement.Piece)),
				*Placement.PieceId.ToString(),
				PlayerHeightUnits,
				*OwnerName(Block));
			return;
		}

		if (UBodySetup* BodySetup = Mesh.GetBodySetup())
		{
			BodySetup->CollisionTraceFlag = CTF_UseComplexAsSimple;
		}
		Block.SetCollisionProfileName(TEXT("BlockAll"));
		Block.SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Block.SetCanEverAffectNavigation(true);
		Block.RecreatePhysicsState();
		UE_LOG(
			LogTemp,
			Display,
			TEXT("ArchonFactoryCanary: ValleyFabCollision piece=%s id=%s mode=visual_complex playerHeight=%.0f owner=%s"),
			*StaticEnum<EArchonValleyPiece>()->GetNameStringByValue(static_cast<int64>(Placement.Piece)),
			*Placement.PieceId.ToString(),
			PlayerHeightUnits,
			*OwnerName(Block));
	}

	FVector FitMeshScaleToPlacement(const UStaticMesh& Mesh, const FArchonValleyPlacement& Placement, bool bUniformFit)
	{
		const FVector DesiredDimensions = GetDesiredVisualDimensions(Placement);
		const FVector MeshDimensions = Mesh.GetBounds().BoxExtent * 2.0;
		FVector FitScale(
			MeshDimensions.X > 1.0 ? DesiredDimensions.X / MeshDimensions.X : Placement.Scale.X,
			MeshDimensions.Y > 1.0 ? DesiredDimensions.Y / MeshDimensions.Y : Placement.Scale.Y,
			MeshDimensions.Z > 1.0 ? DesiredDimensions.Z / MeshDimensions.Z : Placement.Scale.Z);

		if (bUniformFit)
		{
			const double UniformScale = FMath::Clamp(
				FMath::Min3(FitScale.X, FitScale.Y, FitScale.Z),
				0.05,
				250.0);
			FitScale = FVector(UniformScale);
		}
		else
		{
			FitScale.X = FMath::Clamp(FitScale.X, 0.05, 250.0);
			FitScale.Y = FMath::Clamp(FitScale.Y, 0.05, 250.0);
			FitScale.Z = FMath::Clamp(FitScale.Z, 0.05, 250.0);
		}

		return FitScale;
	}

	bool TryApplyFabVisual(UStaticMeshComponent& Block, const FArchonValleyPlacement& Placement)
	{
		TArray<const TCHAR*> MeshPaths;
		bool bUniformFit = true;

		switch (Placement.Piece)
		{
		case EArchonValleyPiece::RidgeSlab:
			bUniformFit = false;
			MeshPaths = {
				TEXT("/Game/Fab/Library/Wall_of_ice/wall_of_ice/StaticMeshes/Object_10.Object_10"),
				TEXT("/Game/Fab/Library/Rock_terrain_with_light_snow/rock_terrain_with_light_snow/StaticMeshes/Object_10.Object_10"),
				TEXT("/Game/Fab/Castle_Rocca_Calascio/castle_of_rocca_calascio/StaticMeshes/Object_10.Object_10")
			};
			break;
		case EArchonValleyPiece::CentralTreeTrunk:
			MeshPaths = {
				TEXT("/Game/Fab/Library/Old_red_wood_tree_stump/old_red_stump.old_red_stump"),
				TEXT("/Game/Fab/Library/Old_green_stump/old_green_stump.old_green_stump")
			};
			break;
		case EArchonValleyPiece::CentralTreeCanopy:
			MeshPaths = {
				TEXT("/Game/Fab/Library/Old_green_stump/old_green_stump.old_green_stump"),
				TEXT("/Game/Fab/Library/Old_red_wood_tree_stump/old_red_stump.old_red_stump")
			};
			break;
		case EArchonValleyPiece::RootButtress:
			MeshPaths = {
				TEXT("/Game/Fab/Library/Old_red_wood_tree_stump/old_red_stump.old_red_stump"),
				TEXT("/Game/Fab/Library/Old_green_stump/old_green_stump.old_green_stump")
			};
			break;
		case EArchonValleyPiece::CoverStone:
			bUniformFit = false;
			MeshPaths = {
				TEXT("/Game/Fab/Library/Rock_terrain_with_light_snow/rock_terrain_with_light_snow/StaticMeshes/Object_6.Object_6"),
				TEXT("/Game/Fab/Library/Rock_terrain_with_light_snow/rock_terrain_with_light_snow/StaticMeshes/Object_4.Object_4"),
				TEXT("/Game/Fab/Library/Rock_terrain_with_light_snow/rock_terrain_with_light_snow/StaticMeshes/Object_8.Object_8"),
				TEXT("/Game/Fab/Library/Rock_terrain_with_light_snow/rock_terrain_with_light_snow/StaticMeshes/Object_9.Object_9"),
				TEXT("/Game/Fab/Castle_Rocca_Calascio/castle_of_rocca_calascio/StaticMeshes/Object_12.Object_12"),
				TEXT("/Game/Fab/Library/Rock_terrain_with_light_snow/rock_terrain_with_light_snow/StaticMeshes/Object_2.Object_2")
			};
			break;
		case EArchonValleyPiece::ResourceSite:
			MeshPaths = {
				TEXT("/Game/Fab/Library/Medieval_Fountain_Empty_Free/medieval_fountain_empty_free/StaticMeshes/Object_2.Object_2"),
				TEXT("/Game/Fab/Castle_Rocca_Calascio/castle_of_rocca_calascio/StaticMeshes/Object_11.Object_11")
			};
			break;
		case EArchonValleyPiece::KinwildCamp:
			MeshPaths = {
				TEXT("/Game/Fab/Large_Medieval_Building/large_medieval_tavern.large_medieval_tavern"),
				TEXT("/Game/Fab/Library/Tavern/tavern/StaticMeshes/Object_4.Object_4"),
				TEXT("/Game/Fab/FANTASTIC_Village_Pack/SM_BLD_body_v07_01.SM_BLD_body_v07_01"),
				TEXT("/Game/Fab/FANTASTIC_Village_Pack/SM_BLD_body_v04_01.SM_BLD_body_v04_01")
			};
			break;
		default:
			return false;
		}

		FString AssetPath;
		UStaticMesh* VisualMesh = LoadFirstMesh(MeshPaths, AssetPath);
		if (!VisualMesh)
		{
			return false;
		}

		const FVector DesiredDimensions = GetDesiredVisualDimensions(Placement);
		const FVector MeshDimensions = VisualMesh->GetBounds().BoxExtent * 2.0;
		Block.SetStaticMesh(VisualMesh);
		Block.SetRelativeScale3D(FitMeshScaleToPlacement(*VisualMesh, Placement, bUniformFit));
		ApplyPlacementCollision(Block, *VisualMesh, Placement);
		UE_LOG(
			LogTemp,
			Display,
			TEXT("ArchonFactoryCanary: ValleyFabVisual piece=%s id=%s owner=%s asset=%s desired=%s meshBounds=%s actorScale=%s meshScale=%s"),
			*StaticEnum<EArchonValleyPiece>()->GetNameStringByValue(static_cast<int64>(Placement.Piece)),
			*Placement.PieceId.ToString(),
			*OwnerName(Block),
			*AssetPath,
			*DesiredDimensions.ToCompactString(),
			*MeshDimensions.ToCompactString(),
			Block.GetOwner() ? *Block.GetOwner()->GetActorScale3D().ToCompactString() : TEXT("none"),
			*Block.GetRelativeScale3D().ToCompactString());
		return true;
	}
}

AArchonValleyBlockActor::AArchonValleyBlockActor()
{
	Block = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ValleyBlock"));
	RootComponent = Block;

	ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeFinder.Succeeded())
	{
		Block->SetStaticMesh(CubeFinder.Object);
	}
	ConstructorHelpers::FObjectFinder<UMaterialInterface> MaterialFinder(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (MaterialFinder.Succeeded())
	{
		Block->SetMaterial(0, MaterialFinder.Object);
	}
	Block->SetCollisionProfileName(TEXT("BlockAll"));

	Tags.Add(TEXT("ArchonValleyBlock"));
}

void AArchonValleyBlockActor::ConfigureBlock(const FArchonValleyPlacement& Placement)
{
	PieceId = Placement.PieceId;
	Piece = Placement.Piece;
	DebugColor = Placement.DebugColor;

	SetActorScale3D(FVector::OneVector);
	Block->SetRelativeScale3D(Placement.Scale);

	if (!Block)
	{
		return;
	}

	if (TryApplyFabVisual(*Block, Placement))
	{
		return;
	}

	if (Placement.Piece == EArchonValleyPiece::ResourceSite)
	{
		Block->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Block->SetCanEverAffectNavigation(false);
		Block->RecreatePhysicsState();
		UE_LOG(
			LogTemp,
			Display,
			TEXT("ArchonFactoryCanary: ValleyFabCollision piece=%s id=%s mode=nonblocking_objective_fallback playerHeight=%.0f owner=%s"),
			*StaticEnum<EArchonValleyPiece>()->GetNameStringByValue(static_cast<int64>(Placement.Piece)),
			*Placement.PieceId.ToString(),
			PlayerHeightUnits,
			*OwnerName(*Block));
	}

	// Prefer imported terrain material; keep fallbacks while asset packs
	// are still being sorted.
	if (PieceId == TEXT("valley_floor"))
	{
		const TCHAR* GroundMaterialPaths[] = {
			TEXT("/Game/Fab/Forest_Ground_Soil_Pine/Material_u1_v1.Material_u1_v1"),
			TEXT("/Game/StandIns/Ground/M_StandInGround.M_StandInGround")
		};
		for (const TCHAR* GroundMaterialPath : GroundMaterialPaths)
		{
			if (UMaterialInterface* GroundMaterial = LoadObject<UMaterialInterface>(nullptr, GroundMaterialPath))
			{
				Block->SetMaterial(0, GroundMaterial);
				UE_LOG(
					LogTemp,
					Display,
					TEXT("ArchonFactoryCanary: ValleyFloorMaterialLoaded loaded=true asset=%s"),
					GroundMaterialPath);
				return;
			}
		}
		UE_LOG(LogTemp, Warning, TEXT("ArchonFactoryCanary: ValleyFloorMaterialLoaded loaded=false fallback=tint"));
	}

	if (UMaterialInstanceDynamic* DynamicMaterial = Block->CreateDynamicMaterialInstance(0))
	{
		DynamicMaterial->SetVectorParameterValue(TEXT("Color"), DebugColor);
		DynamicMaterial->SetVectorParameterValue(TEXT("BaseColor"), DebugColor);
	}
}
