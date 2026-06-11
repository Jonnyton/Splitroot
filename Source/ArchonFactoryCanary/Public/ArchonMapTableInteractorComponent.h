#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ArchonMapTableInteractionTypes.h"
#include "ArchonMapTableInteractorComponent.generated.h"

class AArchonMapTableActor;

UCLASS(ClassGroup=(Archon), Blueprintable, meta=(BlueprintSpawnableComponent))
class ARCHONFACTORYCANARY_API UArchonMapTableInteractorComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UArchonMapTableInteractorComponent();

	UFUNCTION(BlueprintCallable, Category = "Archon|Interaction")
	void ConfigureInteractor(const FArchonMapTableInteractorConfig& InConfig);

	UFUNCTION(BlueprintCallable, Category = "Archon|Interaction")
	bool PreviewMapTable(AArchonMapTableActor* MapTable, FArchonMapTableInteractionResult& OutResult) const;

	UFUNCTION(BlueprintCallable, Category = "Archon|Interaction")
	bool SubmitRtsOrder(
		AArchonMapTableActor* MapTable,
		FArchonRtsCommandIntent Intent,
		FArchonMapTableInteractionResult& OutResult) const;

	UFUNCTION(BlueprintPure, Category = "Archon|Interaction")
	FArchonMapTableInteractorConfig GetInteractorConfig() const { return Config; }

private:
	FArchonMapTableCommandContext BuildContextForTable(const AArchonMapTableActor* MapTable) const;
	void FillIntentDefaults(FArchonRtsCommandIntent& Intent) const;

	UPROPERTY(EditAnywhere, Category = "Archon|Interaction")
	FArchonMapTableInteractorConfig Config;
};
