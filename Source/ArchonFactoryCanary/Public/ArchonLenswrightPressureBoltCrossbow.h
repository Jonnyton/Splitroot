#pragma once

#include "CoreMinimal.h"
#include "ArchonRangedWeaponComponent.h"
#include "ArchonLenswrightPressureBoltCrossbow.generated.h"

UCLASS(ClassGroup=(Archon), Blueprintable, meta=(BlueprintSpawnableComponent))
class ARCHONFACTORYCANARY_API UArchonLenswrightPressureBoltCrossbow : public UArchonRangedWeaponComponent
{
	GENERATED_BODY()

public:
	UArchonLenswrightPressureBoltCrossbow();
};
