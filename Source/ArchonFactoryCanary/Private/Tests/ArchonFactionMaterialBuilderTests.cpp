#if WITH_DEV_AUTOMATION_TESTS

#include "ArchonFactionMaterialBuilder.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonMaterialBuilderParameterNamesAreStableTest,
	"ArchonFactory.MaterialBuilder.ParameterNamesAreStable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonMaterialBuilderParameterNamesAreStableTest::RunTest(const FString& Parameters)
{
	// BP-side materials match these names. Rename and you break every faction-tinted actor.
	TestEqual(TEXT("Base color parameter is 'BaseColor'"),
		UArchonFactionMaterialBuilder::GetBaseColorParameterName(), FName(TEXT("BaseColor")));
	TestEqual(TEXT("Metallic parameter is 'Metallic'"),
		UArchonFactionMaterialBuilder::GetMetallicParameterName(), FName(TEXT("Metallic")));
	TestEqual(TEXT("Roughness parameter is 'Roughness'"),
		UArchonFactionMaterialBuilder::GetRoughnessParameterName(), FName(TEXT("Roughness")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonMaterialBuilderRejectsNullBaseMaterialTest,
	"ArchonFactory.MaterialBuilder.RejectsNullBaseMaterial",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonMaterialBuilderRejectsNullBaseMaterialTest::RunTest(const FString& Parameters)
{
	UObject* TransientOuter = GetTransientPackage();
	UMaterialInstanceDynamic* MID = UArchonFactionMaterialBuilder::CreateFactionMaterial(
		TransientOuter,
		/*BaseMaterial*/ nullptr,
		EArchonFaction::VerdantChoir,
		EArchonFactionPaletteSlot::Primary);
	TestNull(TEXT("Null base material returns null MID (no crash)"), MID);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonMaterialBuilderRejectsNullOuterTest,
	"ArchonFactory.MaterialBuilder.RejectsNullOuter",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonMaterialBuilderRejectsNullOuterTest::RunTest(const FString& Parameters)
{
	UMaterialInstanceDynamic* MID = UArchonFactionMaterialBuilder::CreateFactionMaterial(
		/*Outer*/ nullptr,
		/*BaseMaterial*/ nullptr,
		EArchonFaction::VerdantChoir,
		EArchonFactionPaletteSlot::Primary);
	TestNull(TEXT("Null outer returns null MID (no crash)"), MID);
	return true;
}

#endif
