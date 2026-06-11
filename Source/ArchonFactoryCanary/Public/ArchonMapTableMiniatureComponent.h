#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "ArchonMapTableMiniatureComponent.generated.h"

class UStaticMeshComponent;
class UArchonTeamVisibilityStateComponent;

/**
 * The live diorama on the Archon map table (WC3 hill: issuing orders
 * at the table must feel like playing RTS). A pooled set of tiny
 * engine-shape markers — squads, cores, towers, resource sites —
 * repositioned a few times a second from world state. Enemy markers
 * obey the team's fog (UArchonMapTableMiniatureLibrary::
 * IsWorldPointLit); friendly markers always show. Engine shapes only,
 * so headless proofs never depend on content.
 */
UCLASS(ClassGroup=(Archon), meta=(BlueprintSpawnableComponent))
class ARCHONFACTORYCANARY_API UArchonMapTableMiniatureComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	UArchonMapTableMiniatureComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category = "Archon|MapTable")
	void ConfigureMiniature(int32 InViewingTeamId, UArchonTeamVisibilityStateComponent* InVisibilityState);

	UFUNCTION(BlueprintPure, Category = "Archon|MapTable")
	int32 GetVisibleMarkerCount() const { return VisibleMarkerCount; }

private:
	void RefreshBoard();
	UStaticMeshComponent* AcquireMarker(int32& PoolCursor, const TCHAR* MeshPath, float UniformScale);
	void PlaceMarker(UStaticMeshComponent* Marker, const FVector& WorldPoint, const FLinearColor& Color, float ZLift);

	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> MarkerPool;

	UPROPERTY()
	TObjectPtr<UArchonTeamVisibilityStateComponent> VisibilityState;

	int32 ViewingTeamId = INDEX_NONE;
	int32 VisibleMarkerCount = 0;
	float RefreshAccumulator = 0.0f;
	bool bLoggedFirstRefresh = false;

	// Board geometry: world valley (+/-20000 x +/-12000) onto the table
	// felt (+/-95 x +/-57 uu in actor space — inside the 2.2-scaled
	// wooden table top).
	FVector WorldOrigin = FVector::ZeroVector;
	FVector WorldExtents = FVector(20000.0f, 12000.0f, 0.0f);
	float TableHalfX = 95.0f;
	float TableHalfY = 57.0f;
	float RefreshIntervalSeconds = 0.25f;
};
