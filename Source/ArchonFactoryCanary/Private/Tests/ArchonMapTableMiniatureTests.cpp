#include "ArchonMapTableMiniatureLibrary.h"
#include "ArchonTeamVisibilityTypes.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonMiniatureWorldToTableTest,
	"ArchonFactory.MapTable.MiniatureWorldToTableMapping",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FArchonMiniatureWorldToTableTest::RunTest(const FString& Parameters)
{
	const FVector Origin = FVector::ZeroVector;
	const FVector Extents(20000.0f, 12000.0f, 0.0f);

	const FVector Center = UArchonMapTableMiniatureLibrary::WorldToTableLocal(
		FVector::ZeroVector, Origin, Extents, 95.0f, 57.0f);
	TestTrue(TEXT("World center maps to board center"), Center.Equals(FVector::ZeroVector, 0.01f));

	const FVector Corner = UArchonMapTableMiniatureLibrary::WorldToTableLocal(
		FVector(20000.0f, -12000.0f, 300.0f), Origin, Extents, 95.0f, 57.0f);
	TestTrue(TEXT("World corner maps to board corner, Z flattened"),
		Corner.Equals(FVector(95.0f, -57.0f, 0.0f), 0.01f));

	const FVector OffMap = UArchonMapTableMiniatureLibrary::WorldToTableLocal(
		FVector(99999.0f, 50.0f, 0.0f), Origin, Extents, 95.0f, 57.0f);
	TestTrue(TEXT("Off-map point pins to the board rim"),
		FMath::IsNearlyEqual(OffMap.X, 95.0f, 0.01f));

	const FVector Halfway = UArchonMapTableMiniatureLibrary::WorldToTableLocal(
		FVector(10000.0f, 6000.0f, 0.0f), Origin, Extents, 95.0f, 57.0f);
	TestTrue(TEXT("Halfway point maps proportionally"),
		Halfway.Equals(FVector(47.5f, 28.5f, 0.0f), 0.01f));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonMiniatureFogReadTest,
	"ArchonFactory.MapTable.MiniatureRespectsFog",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FArchonMiniatureFogReadTest::RunTest(const FString& Parameters)
{
	TArray<FArchonVisibilityGridCell> Cells;
	FArchonVisibilityGridCell LitCell;
	LitCell.Cell = FIntPoint(0, 0);
	LitCell.WorldCenter = FVector(0.0f, 0.0f, 0.0f);
	LitCell.State = EArchonTeamVisibilityState::Lit;
	Cells.Add(LitCell);

	FArchonVisibilityGridCell FogCell;
	FogCell.Cell = FIntPoint(1, 0);
	FogCell.WorldCenter = FVector(4000.0f, 0.0f, 0.0f);
	FogCell.State = EArchonTeamVisibilityState::Fog;
	Cells.Add(FogCell);

	TestTrue(TEXT("Point in the lit cell is visible"),
		UArchonMapTableMiniatureLibrary::IsWorldPointLit(Cells, FVector(100.0f, 50.0f, 0.0f)));
	TestFalse(TEXT("Point nearest the fog cell is hidden"),
		UArchonMapTableMiniatureLibrary::IsWorldPointLit(Cells, FVector(3900.0f, 0.0f, 0.0f)));
	TestFalse(TEXT("No cells means nothing is visible"),
		UArchonMapTableMiniatureLibrary::IsWorldPointLit(TArray<FArchonVisibilityGridCell>(), FVector::ZeroVector));

	return true;
}
