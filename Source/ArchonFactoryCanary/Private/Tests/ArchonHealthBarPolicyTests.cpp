#include "ArchonHealthBarPolicyLibrary.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonHealthBarFillScaleTest,
	"ArchonFactory.HealthBar.FillScaleClampsToUnitRange",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FArchonHealthBarFillScaleTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("Full health fills the bar"),
		FMath::IsNearlyEqual(UArchonHealthBarPolicyLibrary::ComputeHealthBarFillScale(1.0f), 1.0f));
	TestTrue(TEXT("Half health half fills"),
		FMath::IsNearlyEqual(UArchonHealthBarPolicyLibrary::ComputeHealthBarFillScale(0.5f), 0.5f));
	TestTrue(TEXT("Dead is empty"),
		FMath::IsNearlyEqual(UArchonHealthBarPolicyLibrary::ComputeHealthBarFillScale(0.0f), 0.0f));
	TestTrue(TEXT("Overheal clamps to full"),
		FMath::IsNearlyEqual(UArchonHealthBarPolicyLibrary::ComputeHealthBarFillScale(1.7f), 1.0f));
	TestTrue(TEXT("Negative clamps to empty"),
		FMath::IsNearlyEqual(UArchonHealthBarPolicyLibrary::ComputeHealthBarFillScale(-0.3f), 0.0f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonHealthBarColorBandsTest,
	"ArchonFactory.HealthBar.ColorBandsGreenYellowRed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FArchonHealthBarColorBandsTest::RunTest(const FString& Parameters)
{
	const FLinearColor Healthy = UArchonHealthBarPolicyLibrary::ComputeHealthBarColor(1.0f);
	const FLinearColor Worn = UArchonHealthBarPolicyLibrary::ComputeHealthBarColor(0.5f);
	const FLinearColor Critical = UArchonHealthBarPolicyLibrary::ComputeHealthBarColor(0.2f);

	TestTrue(TEXT("Healthy band is green-dominant"), Healthy.G > Healthy.R && Healthy.G > Healthy.B);
	TestTrue(TEXT("Worn band is yellow (red+green, little blue)"), Worn.R > 0.5f && Worn.G > 0.5f && Worn.B < 0.3f);
	TestTrue(TEXT("Critical band is red-dominant"), Critical.R > Critical.G && Critical.R > Critical.B);
	TestEqual(TEXT("Band edges: just above 2/3 still green"),
		UArchonHealthBarPolicyLibrary::ComputeHealthBarColor(0.67f), Healthy);
	TestEqual(TEXT("Exactly 2/3 drops to yellow"),
		UArchonHealthBarPolicyLibrary::ComputeHealthBarColor(2.0f / 3.0f), Worn);
	TestEqual(TEXT("Exactly 1/3 drops to red"),
		UArchonHealthBarPolicyLibrary::ComputeHealthBarColor(1.0f / 3.0f), Critical);
	TestEqual(TEXT("Out-of-range clamps into the bands"),
		UArchonHealthBarPolicyLibrary::ComputeHealthBarColor(5.0f), Healthy);
	return true;
}
