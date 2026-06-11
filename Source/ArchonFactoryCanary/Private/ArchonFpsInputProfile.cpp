#include "ArchonFpsInputProfile.h"

namespace
{
	FArchonFpsInputBinding MakeBinding(
		EArchonFpsInputAction Action,
		const FKey& Key,
		float Scale = 1.0f,
		bool bMouseAxis = false)
	{
		FArchonFpsInputBinding Binding;
		Binding.Action = Action;
		Binding.Key = Key;
		Binding.Scale = Scale;
		Binding.bMouseAxis = bMouseAxis;
		return Binding;
	}
}

TArray<FArchonFpsInputBinding> UArchonFpsInputProfile::GetDefaultKeyboardMouseBindings()
{
	return {
		MakeBinding(EArchonFpsInputAction::MoveForward, EKeys::W, 1.0f),
		MakeBinding(EArchonFpsInputAction::MoveBackward, EKeys::S, -1.0f),
		MakeBinding(EArchonFpsInputAction::MoveLeft, EKeys::A, -1.0f),
		MakeBinding(EArchonFpsInputAction::MoveRight, EKeys::D, 1.0f),
		MakeBinding(EArchonFpsInputAction::LookYaw, EKeys::MouseX, 1.0f, true),
		MakeBinding(EArchonFpsInputAction::LookPitch, EKeys::MouseY, 1.0f, true),
		MakeBinding(EArchonFpsInputAction::Jump, EKeys::SpaceBar),
		MakeBinding(EArchonFpsInputAction::Sprint, EKeys::LeftShift),
		MakeBinding(EArchonFpsInputAction::Interact, EKeys::E),
		MakeBinding(EArchonFpsInputAction::PrimaryFire, EKeys::LeftMouseButton),
		MakeBinding(EArchonFpsInputAction::SecondaryFire, EKeys::RightMouseButton),
		MakeBinding(EArchonFpsInputAction::Reload, EKeys::R),
		MakeBinding(EArchonFpsInputAction::Crouch, EKeys::LeftControl),
		MakeBinding(EArchonFpsInputAction::Crouch, EKeys::C),
		MakeBinding(EArchonFpsInputAction::ToggleMapTable, EKeys::Tab),
		MakeBinding(EArchonFpsInputAction::Pause, EKeys::Escape)
	};
}

bool UArchonFpsInputProfile::HasDefaultKeyboardMouseBinding(
	EArchonFpsInputAction Action,
	FKey Key,
	float ExpectedScale)
{
	for (const FArchonFpsInputBinding& Binding : GetDefaultKeyboardMouseBindings())
	{
		if (Binding.Action == Action && Binding.Key == Key && FMath::IsNearlyEqual(Binding.Scale, ExpectedScale))
		{
			return true;
		}
	}

	return false;
}

bool UArchonFpsInputProfile::UsesMouseAxisForLook(EArchonFpsInputAction Action)
{
	for (const FArchonFpsInputBinding& Binding : GetDefaultKeyboardMouseBindings())
	{
		if (Binding.Action == Action)
		{
			return Binding.bMouseAxis;
		}
	}

	return false;
}

bool UArchonFpsInputProfile::IsMouseSmoothingAllowedByArchonStandard()
{
	return false;
}
