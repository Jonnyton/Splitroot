#if WITH_DEV_AUTOMATION_TESTS

#include "ArchonBlockoutActors.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonBlockoutDebugColorsDistinguishFactionActorsTest,
	"ArchonFactory.Blockout.DebugColorsDistinguishFactionActors",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonBlockoutDebugColorsDistinguishFactionActorsTest::RunTest(const FString& Parameters)
{
	const AArchonVerdantOutpostActor* VerdantOutpost = NewObject<AArchonVerdantOutpostActor>();
	const AArchonSplitrootTreeCentralActor* SplitrootTree = NewObject<AArchonSplitrootTreeCentralActor>();
	const AArchonLenswrightOutpostGhostActor* LenswrightGhost = NewObject<AArchonLenswrightOutpostGhostActor>();

	const FLinearColor VerdantColor = VerdantOutpost->GetDebugColor();
	const FLinearColor SplitrootColor = SplitrootTree->GetDebugColor();
	const FLinearColor LenswrightColor = LenswrightGhost->GetDebugColor();

	TestTrue(TEXT("Verdant debug color reads green-dominant"), VerdantColor.G > VerdantColor.R && VerdantColor.G > VerdantColor.B);
	TestTrue(TEXT("Splitroot debug color reads warm-brown"), SplitrootColor.R > SplitrootColor.G && SplitrootColor.G >= SplitrootColor.B);
	TestTrue(TEXT("Lenswright debug color reads brass-oxblood"), LenswrightColor.R > LenswrightColor.G && LenswrightColor.G > LenswrightColor.B);
	TestTrue(TEXT("Faction blockout colors are intentionally distinct"), !VerdantColor.Equals(SplitrootColor) && !VerdantColor.Equals(LenswrightColor) && !SplitrootColor.Equals(LenswrightColor));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonBlockoutCoverStoneDebugColorNeutralTest,
	"ArchonFactory.Blockout.CoverStoneDebugColorNeutral",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonBlockoutCoverStoneDebugColorNeutralTest::RunTest(const FString& Parameters)
{
	const AArchonCoverStoneRootVaultActor* CoverStone = NewObject<AArchonCoverStoneRootVaultActor>();
	const FLinearColor CoverColor = CoverStone->GetDebugColor();

	TestTrue(TEXT("Cover stones remain neutral grey"), FMath::IsNearlyEqual(CoverColor.R, CoverColor.G) && FMath::IsNearlyEqual(CoverColor.G, CoverColor.B));
	TestTrue(TEXT("Cover stones stay visually separate from dark/black placeholders"), CoverColor.R > 0.20f);
	return true;
}

#endif
