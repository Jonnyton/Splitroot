#pragma once

#include "CoreMinimal.h"
#include "ArchonRangedWeaponComponent.h"
#include "ArchonVerdantThornsproutBow.generated.h"

UCLASS(ClassGroup=(Archon), Blueprintable, meta=(BlueprintSpawnableComponent))
class ARCHONFACTORYCANARY_API UArchonVerdantThornsproutBow : public UArchonRangedWeaponComponent
{
	GENERATED_BODY()

public:
	UArchonVerdantThornsproutBow();
};
