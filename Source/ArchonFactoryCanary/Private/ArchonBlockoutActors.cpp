#include "ArchonBlockoutActors.h"
#include "ArchonFactionPaletteLibrary.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	UStaticMesh* TryFindBasicShape(const TCHAR* Path)
	{
		ConstructorHelpers::FObjectFinder<UStaticMesh> Finder(Path);
		return Finder.Succeeded() ? Finder.Object : nullptr;
	}

	UMaterialInterface* TryFindBasicShapeMaterial()
	{
		ConstructorHelpers::FObjectFinder<UMaterialInterface> Finder(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
		return Finder.Succeeded() ? Finder.Object : nullptr;
	}

	void ApplyBlockoutDebugMaterial(UStaticMeshComponent* Component, const FLinearColor& Color)
	{
		if (!Component)
		{
			return;
		}

		if (UMaterialInterface* BasicMaterial = TryFindBasicShapeMaterial())
		{
			Component->SetMaterial(0, BasicMaterial);
		}

		UMaterialInstanceDynamic* DynamicMaterial = Component->CreateDynamicMaterialInstance(0);
		if (!DynamicMaterial)
		{
			return;
		}

		DynamicMaterial->SetVectorParameterValue(TEXT("Color"), Color);
		DynamicMaterial->SetVectorParameterValue(TEXT("BaseColor"), Color);
	}
}

AArchonVerdantOutpostActor::AArchonVerdantOutpostActor()
{
	// Source DebugColor from the palette library so all faction tints flow through
	// one place. faction_palette.json + ArchonFactionPaletteLibrary stay aligned.
	DebugColor = UArchonFactionPaletteLibrary::GetFactionColor(
		EArchonFaction::VerdantChoir, EArchonFactionPaletteSlot::Primary);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VerdantOutpostMesh"));
	RootComponent = Mesh;
	Mesh->SetRelativeScale3D(FVector(1.4f, 1.4f, 1.4f));
	if (UStaticMesh* Stump = LoadObject<UStaticMesh>(
		nullptr, TEXT("/Game/Fab/Library/Old_green_stump/old_green_stump.old_green_stump")))
	{
		Mesh->SetStaticMesh(Stump);
	}
	else if (UStaticMesh* Cube = TryFindBasicShape(TEXT("/Engine/BasicShapes/Cube.Cube")))
	{
		Mesh->SetStaticMesh(Cube);
		Mesh->SetRelativeScale3D(FVector(8.0f, 8.0f, 4.0f));
	}
	// The imported stump is a visual landmark until it gets authored collision.
	// The base core and tower carry gameplay blocking/objective behavior.
	Mesh->SetCollisionProfileName(TEXT("NoCollision"));
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetCanEverAffectNavigation(false);
	ApplyBlockoutDebugMaterial(Mesh, DebugColor);

	Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("VerdantOutpostLabel"));
	Label->SetupAttachment(Mesh);
	Label->SetRelativeLocation(FVector(0.0f, 0.0f, 60.0f));
	Label->SetRelativeRotation(FRotator(0.0f, 180.0f, 0.0f));
	Label->SetHorizontalAlignment(EHTA_Center);
	Label->SetTextRenderColor(FColor(120, 200, 130));
	Label->SetWorldSize(40.0f);
	Label->SetText(FText::FromString(TEXT("VERDANT OUTPOST")));
	Label->SetHiddenInGame(true);

	Tags.Add(TEXT("ArchonBlockout"));
	Tags.Add(TEXT("VerdantOutpost"));
}

AArchonSplitrootTreeCentralActor::AArchonSplitrootTreeCentralActor()
{
	// Splitroot wood is pre-faction; reads as "the land," sourced from the
	// neutral palette so it cannot drift into a faction color.
	DebugColor = UArchonFactionPaletteLibrary::GetNeutralColor(
		EArchonNeutralPaletteSlot::SplitrootWood);

	Trunk = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SplitrootTrunk"));
	RootComponent = Trunk;
	Trunk->SetRelativeScale3D(FVector(6.0f, 6.0f, 12.0f));
	if (UStaticMesh* Cylinder = TryFindBasicShape(TEXT("/Engine/BasicShapes/Cylinder.Cylinder")))
	{
		Trunk->SetStaticMesh(Cylinder);
	}
	Trunk->SetCollisionProfileName(TEXT("BlockAll"));
	ApplyBlockoutDebugMaterial(Trunk, DebugColor);

	Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("SplitrootLabel"));
	Label->SetupAttachment(Trunk);
	Label->SetRelativeLocation(FVector(0.0f, 0.0f, 80.0f));
	Label->SetRelativeRotation(FRotator(0.0f, 180.0f, 0.0f));
	Label->SetHorizontalAlignment(EHTA_Center);
	Label->SetTextRenderColor(FColor(180, 120, 60));
	Label->SetWorldSize(80.0f);
	Label->SetText(FText::FromString(TEXT("CENTRAL SPLITROOT")));
	Label->SetHiddenInGame(true);

	Tags.Add(TEXT("ArchonBlockout"));
	Tags.Add(TEXT("SplitrootTreeCentral"));
}

AArchonLenswrightOutpostGhostActor::AArchonLenswrightOutpostGhostActor()
{
	// Brass-oxblood from the palette library — no gunpowder hill is enforced
	// because the Lenswright Primary slot is the only tint applied here.
	DebugColor = UArchonFactionPaletteLibrary::GetFactionColor(
		EArchonFaction::LenswrightCompact, EArchonFactionPaletteSlot::Primary);

	Silhouette = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LenswrightSilhouette"));
	RootComponent = Silhouette;
	Silhouette->SetRelativeScale3D(FVector(0.35f, 0.35f, 0.35f));
	if (UStaticMesh* StoneHall = LoadObject<UStaticMesh>(
		nullptr, TEXT("/Game/Fab/Library/Sacre_Coeur/Sacre_Coeur.Sacre_Coeur")))
	{
		Silhouette->SetStaticMesh(StoneHall);
	}
	else if (UStaticMesh* Cube = TryFindBasicShape(TEXT("/Engine/BasicShapes/Cube.Cube")))
	{
		Silhouette->SetStaticMesh(Cube);
		Silhouette->SetRelativeScale3D(FVector(6.0f, 6.0f, 6.0f));
	}
	// Ghost: no collision so player and squad can pass through; visual only.
	Silhouette->SetCollisionProfileName(TEXT("NoCollision"));
	ApplyBlockoutDebugMaterial(Silhouette, DebugColor);

	Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("LenswrightGhostLabel"));
	Label->SetupAttachment(Silhouette);
	Label->SetRelativeLocation(FVector(0.0f, 0.0f, 60.0f));
	Label->SetRelativeRotation(FRotator(0.0f, 180.0f, 0.0f));
	Label->SetHorizontalAlignment(EHTA_Center);
	Label->SetTextRenderColor(FColor(150, 80, 40));
	Label->SetWorldSize(40.0f);
	Label->SetText(FText::FromString(TEXT("LENSWRIGHT GHOST")));
	Label->SetHiddenInGame(true);

	Tags.Add(TEXT("ArchonBlockout"));
	Tags.Add(TEXT("LenswrightOutpostGhost"));
}

AArchonCoverStoneRootVaultActor::AArchonCoverStoneRootVaultActor()
{
	// Cover stones stay neutral grey to NOT compete with faction colors.
	DebugColor = UArchonFactionPaletteLibrary::GetNeutralColor(
		EArchonNeutralPaletteSlot::CoverStone);

	Stone = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CoverStone"));
	RootComponent = Stone;
	Stone->SetRelativeScale3D(FVector(2.0f, 2.0f, 1.5f));
	if (UStaticMesh* Cube = TryFindBasicShape(TEXT("/Engine/BasicShapes/Cube.Cube")))
	{
		Stone->SetStaticMesh(Cube);
	}
	Stone->SetCollisionProfileName(TEXT("BlockAll"));
	ApplyBlockoutDebugMaterial(Stone, DebugColor);

	Tags.Add(TEXT("ArchonBlockout"));
	Tags.Add(TEXT("CoverStoneRootVault"));
}
