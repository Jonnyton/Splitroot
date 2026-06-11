#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ArchonBlockoutActors.generated.h"

class UStaticMeshComponent;
class UTextRenderComponent;

UCLASS(Blueprintable)
class ARCHONFACTORYCANARY_API AArchonVerdantOutpostActor : public AActor
{
	GENERATED_BODY()

public:
	AArchonVerdantOutpostActor();

	UFUNCTION(BlueprintPure, Category = "Archon|Blockout")
	int32 GetTeamId() const { return TeamId; }

	UFUNCTION(BlueprintPure, Category = "Archon|Blockout")
	float GetVisionRadius() const { return VisionRadius; }

	UFUNCTION(BlueprintPure, Category = "Archon|Blockout")
	FName GetBuildingId() const { return BuildingId; }

	UFUNCTION(BlueprintPure, Category = "Archon|Blockout")
	FLinearColor GetDebugColor() const { return DebugColor; }

private:
	UPROPERTY(VisibleAnywhere, Category = "Archon|Blockout")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(VisibleAnywhere, Category = "Archon|Blockout")
	TObjectPtr<UTextRenderComponent> Label;

	UPROPERTY(EditAnywhere, Category = "Archon|Blockout")
	int32 TeamId = 0;

	UPROPERTY(EditAnywhere, Category = "Archon|Blockout")
	float VisionRadius = 2400.0f;

	UPROPERTY(EditAnywhere, Category = "Archon|Blockout")
	FName BuildingId = TEXT("verdant_outpost_west");

	UPROPERTY(EditAnywhere, Category = "Archon|Blockout")
	FLinearColor DebugColor = FLinearColor(0.30f, 0.55f, 0.25f, 1.0f);
};

UCLASS(Blueprintable)
class ARCHONFACTORYCANARY_API AArchonSplitrootTreeCentralActor : public AActor
{
	GENERATED_BODY()

public:
	AArchonSplitrootTreeCentralActor();

	UFUNCTION(BlueprintPure, Category = "Archon|Blockout")
	FName GetResourceNodeId() const { return ResourceNodeId; }

	UFUNCTION(BlueprintPure, Category = "Archon|Blockout")
	int32 GetControllingTeamId() const { return ControllingTeamId; }

	UFUNCTION(BlueprintPure, Category = "Archon|Blockout")
	FLinearColor GetDebugColor() const { return DebugColor; }

private:
	UPROPERTY(VisibleAnywhere, Category = "Archon|Blockout")
	TObjectPtr<UStaticMeshComponent> Trunk;

	UPROPERTY(VisibleAnywhere, Category = "Archon|Blockout")
	TObjectPtr<UTextRenderComponent> Label;

	UPROPERTY(EditAnywhere, Category = "Archon|Blockout")
	FName ResourceNodeId = TEXT("central_splitroot");

	UPROPERTY(EditAnywhere, Category = "Archon|Blockout")
	int32 ControllingTeamId = INDEX_NONE;

	UPROPERTY(EditAnywhere, Category = "Archon|Blockout")
	FLinearColor DebugColor = FLinearColor(0.45f, 0.30f, 0.15f, 1.0f);
};

UCLASS(Blueprintable)
class ARCHONFACTORYCANARY_API AArchonLenswrightOutpostGhostActor : public AActor
{
	GENERATED_BODY()

public:
	AArchonLenswrightOutpostGhostActor();

	UFUNCTION(BlueprintPure, Category = "Archon|Blockout")
	int32 GetOwnerTeamId() const { return OwnerTeamId; }

	UFUNCTION(BlueprintPure, Category = "Archon|Blockout")
	bool IsGhost() const { return bIsGhost; }

	UFUNCTION(BlueprintPure, Category = "Archon|Blockout")
	FName GetBuildingId() const { return BuildingId; }

	UFUNCTION(BlueprintPure, Category = "Archon|Blockout")
	FLinearColor GetDebugColor() const { return DebugColor; }

private:
	UPROPERTY(VisibleAnywhere, Category = "Archon|Blockout")
	TObjectPtr<UStaticMeshComponent> Silhouette;

	UPROPERTY(VisibleAnywhere, Category = "Archon|Blockout")
	TObjectPtr<UTextRenderComponent> Label;

	UPROPERTY(EditAnywhere, Category = "Archon|Blockout")
	int32 OwnerTeamId = 1;

	UPROPERTY(EditAnywhere, Category = "Archon|Blockout")
	bool bIsGhost = true;

	UPROPERTY(EditAnywhere, Category = "Archon|Blockout")
	FName BuildingId = TEXT("lenswright_outpost_southeast");

	UPROPERTY(EditAnywhere, Category = "Archon|Blockout")
	FLinearColor DebugColor = FLinearColor(0.55f, 0.30f, 0.15f, 1.0f);
};

UCLASS(Blueprintable)
class ARCHONFACTORYCANARY_API AArchonCoverStoneRootVaultActor : public AActor
{
	GENERATED_BODY()

public:
	AArchonCoverStoneRootVaultActor();

	UFUNCTION(BlueprintPure, Category = "Archon|Blockout")
	FLinearColor GetDebugColor() const { return DebugColor; }

private:
	UPROPERTY(VisibleAnywhere, Category = "Archon|Blockout")
	TObjectPtr<UStaticMeshComponent> Stone;

	UPROPERTY(EditAnywhere, Category = "Archon|Blockout")
	FLinearColor DebugColor = FLinearColor(0.35f, 0.35f, 0.35f, 1.0f);
};
