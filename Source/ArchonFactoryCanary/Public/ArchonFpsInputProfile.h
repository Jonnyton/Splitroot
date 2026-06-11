#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ArchonFpsInputProfile.generated.h"

UENUM(BlueprintType)
enum class EArchonFpsInputAction : uint8
{
	MoveForward,
	MoveBackward,
	MoveLeft,
	MoveRight,
	LookYaw,
	LookPitch,
	Jump,
	Sprint,
	Interact,
	PrimaryFire,
	SecondaryFire,
	Reload,
	Crouch,
	ToggleMapTable,
	Pause
};

USTRUCT(BlueprintType)
struct FArchonFpsInputBinding
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Archon|Input")
	EArchonFpsInputAction Action = EArchonFpsInputAction::MoveForward;

	UPROPERTY(BlueprintReadOnly, Category = "Archon|Input")
	FKey Key;

	UPROPERTY(BlueprintReadOnly, Category = "Archon|Input")
	float Scale = 1.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Archon|Input")
	bool bMouseAxis = false;
};

UCLASS()
class ARCHONFACTORYCANARY_API UArchonFpsInputProfile : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Archon|Input")
	static TArray<FArchonFpsInputBinding> GetDefaultKeyboardMouseBindings();

	UFUNCTION(BlueprintPure, Category = "Archon|Input")
	static bool HasDefaultKeyboardMouseBinding(EArchonFpsInputAction Action, FKey Key, float ExpectedScale);

	UFUNCTION(BlueprintPure, Category = "Archon|Input")
	static bool UsesMouseAxisForLook(EArchonFpsInputAction Action);

	UFUNCTION(BlueprintPure, Category = "Archon|Input")
	static bool IsMouseSmoothingAllowedByArchonStandard();
};
