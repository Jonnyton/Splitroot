#include "ArchonSplitrootValleyLayoutLibrary.h"
#include "ArchonFactionSilhouetteLibrary.h"
#include "ArchonValleyLayoutTypes.h"
#include "Misc/AutomationTest.h"

namespace
{
	bool FindSingle(
		const TArray<FArchonValleyPlacement>& Layout,
		const TCHAR* PieceId,
		FArchonValleyPlacement& OutPlacement)
	{
		return UArchonSplitrootValleyLayoutLibrary::FindPlacementById(Layout, PieceId, OutPlacement);
	}

	double DistanceToNearestMapReadHotZone(const FVector& Location)
	{
		static const FVector2D HotZones[] = {
			FVector2D(-6000.0, 7000.0),
			FVector2D(-6000.0, 5000.0),
			FVector2D(-5000.0, 5000.0),
			FVector2D(5000.0, 4000.0),
			FVector2D(5000.0, 5000.0),
			FVector2D(11000.0, 8000.0),
			FVector2D(10000.0, 9000.0),
			FVector2D(12000.0, 9000.0)
		};

		double Nearest = TNumericLimits<double>::Max();
		const FVector2D Point(Location.X, Location.Y);
		for (const FVector2D& HotZone : HotZones)
		{
			Nearest = FMath::Min(Nearest, FVector2D::Distance(Point, HotZone));
		}
		return Nearest;
	}

	double DistanceToRouteSegment(const FVector& Location, const FVector2D& A, const FVector2D& B)
	{
		const FVector2D Point(Location.X, Location.Y);
		const FVector2D AB = B - A;
		const double LengthSquared = AB.SizeSquared();
		if (LengthSquared <= 1.0)
		{
			return FVector2D::Distance(Point, A);
		}

		const double Alpha = FMath::Clamp(FVector2D::DotProduct(Point - A, AB) / LengthSquared, 0.0, 1.0);
		const FVector2D Closest = A + AB * Alpha;
		return FVector2D::Distance(Point, Closest);
	}

	double DistanceToNearestPrimaryRoute(const FVector& Location)
	{
		struct FRouteSegment
		{
			FVector2D A;
			FVector2D B;
		};
		static const FRouteSegment RouteSegments[] = {
			{ FVector2D(-15000.0, 0.0), FVector2D(-2400.0, 0.0) },
			{ FVector2D(-15000.0, 0.0), FVector2D(-4000.0, -7000.0) },
			{ FVector2D(14500.0, 5200.0), FVector2D(7800.0, 1200.0) },
			{ FVector2D(7800.0, 1200.0), FVector2D(2000.0, 1400.0) },
			{ FVector2D(12000.0, -10000.0), FVector2D(2000.0, -1400.0) },
			{ FVector2D(2000.0, 1400.0), FVector2D(0.0, -3000.0) },
			{ FVector2D(2000.0, -1400.0), FVector2D(0.0, -3000.0) },
			{ FVector2D(0.0, 7000.0), FVector2D(0.0, -3000.0) }
		};

		double Nearest = TNumericLimits<double>::Max();
		for (const FRouteSegment& Segment : RouteSegments)
		{
			Nearest = FMath::Min(Nearest, DistanceToRouteSegment(Location, Segment.A, Segment.B));
		}
		return Nearest;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonValleyLayoutSingletonPiecesTest,
	"ArchonFactory.Valley.LayoutHasSingletonAnchors",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FArchonValleyLayoutSingletonPiecesTest::RunTest(const FString& Parameters)
{
	const TArray<FArchonValleyPlacement> Layout = UArchonSplitrootValleyLayoutLibrary::BuildSplitrootValleyLayout();

	TestEqual(
		TEXT("Exactly one trunk"),
		UArchonSplitrootValleyLayoutLibrary::FilterPlacementsByPiece(Layout, EArchonValleyPiece::CentralTreeTrunk).Num(),
		1);
	TestEqual(
		TEXT("Exactly one canopy"),
		UArchonSplitrootValleyLayoutLibrary::FilterPlacementsByPiece(Layout, EArchonValleyPiece::CentralTreeCanopy).Num(),
		1);
	TestEqual(
		TEXT("Exactly one map table"),
		UArchonSplitrootValleyLayoutLibrary::FilterPlacementsByPiece(Layout, EArchonValleyPiece::MapTable).Num(),
		1);
	TestEqual(
		TEXT("Exactly one player spawn"),
		UArchonSplitrootValleyLayoutLibrary::FilterPlacementsByPiece(Layout, EArchonValleyPiece::PlayerSpawn).Num(),
		1);
	TestEqual(
		TEXT("Exactly one squad spawn"),
		UArchonSplitrootValleyLayoutLibrary::FilterPlacementsByPiece(Layout, EArchonValleyPiece::SquadSpawn).Num(),
		1);
	TestEqual(
		TEXT("Two ridge walls"),
		UArchonSplitrootValleyLayoutLibrary::FilterPlacementsByPiece(Layout, EArchonValleyPiece::RidgeSlab).Num(),
		2);
	TestEqual(
		TEXT("Six root buttresses"),
		UArchonSplitrootValleyLayoutLibrary::FilterPlacementsByPiece(Layout, EArchonValleyPiece::RootButtress).Num(),
		6);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonValleyTrunkIsLandmarkTest,
	"ArchonFactory.Valley.CentralTreeOwnsTheSkyline",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FArchonValleyTrunkIsLandmarkTest::RunTest(const FString& Parameters)
{
	const TArray<FArchonValleyPlacement> Layout = UArchonSplitrootValleyLayoutLibrary::BuildSplitrootValleyLayout();

	FArchonValleyPlacement Trunk;
	TestTrue(TEXT("Trunk exists"), FindSingle(Layout, TEXT("central_splitroot_trunk"), Trunk));
	TestTrue(
		TEXT("Trunk height matches the landmark constant"),
		FMath::IsNearlyEqual(Trunk.Scale.Z * 100.0, UArchonSplitrootValleyLayoutLibrary::CentralTreeTrunkHeight, 1.0));

	// No structure may stand taller than the tree (canopy rides on the trunk).
	for (const FArchonValleyPlacement& Placement : Layout)
	{
		if (Placement.Piece == EArchonValleyPiece::CentralTreeTrunk ||
			Placement.Piece == EArchonValleyPiece::CentralTreeCanopy)
		{
			continue;
		}
		const double TopZ = Placement.Location.Z + Placement.Scale.Z * 50.0;
		TestTrue(
			*FString::Printf(TEXT("%s stays under the canopy line"), *Placement.PieceId.ToString()),
			TopZ < UArchonSplitrootValleyLayoutLibrary::CentralTreeTrunkHeight);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonValleyFactionTriangleTest,
	"ArchonFactory.Valley.ThreeFactionBasesFormTriangle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FArchonValleyFactionTriangleTest::RunTest(const FString& Parameters)
{
	const TArray<FArchonValleyPlacement> Layout = UArchonSplitrootValleyLayoutLibrary::BuildSplitrootValleyLayout();

	FArchonValleyPlacement Verdant;
	FArchonValleyPlacement Lenswright;
	FArchonValleyPlacement Kinwild;
	TestTrue(TEXT("Verdant base exists"), FindSingle(Layout, TEXT("verdant_base"), Verdant));
	TestTrue(TEXT("Lenswright base exists"), FindSingle(Layout, TEXT("lenswright_base"), Lenswright));
	TestTrue(TEXT("Kinwild base exists"), FindSingle(Layout, TEXT("kinwild_base"), Kinwild));

	TestEqual(TEXT("Verdant team id"), Verdant.TeamId, 0);
	TestEqual(TEXT("Lenswright team id"), Lenswright.TeamId, 1);
	TestEqual(TEXT("Kinwild team id"), Kinwild.TeamId, 2);

	const double MinBaseSeparation = 15000.0;
	TestTrue(
		TEXT("Verdant and Lenswright far apart"),
		FVector::Dist2D(Verdant.Location, Lenswright.Location) >= MinBaseSeparation);
	TestTrue(
		TEXT("Verdant and Kinwild far apart"),
		FVector::Dist2D(Verdant.Location, Kinwild.Location) >= MinBaseSeparation);
	TestTrue(
		TEXT("Lenswright and Kinwild far apart"),
		FVector::Dist2D(Lenswright.Location, Kinwild.Location) >= MinBaseSeparation);

	// Every base sees the tree from a different third of the compass.
	const FVector Tree(0.0, 0.0, 0.0);
	TestTrue(TEXT("Verdant west of tree"), Verdant.Location.X < Tree.X);
	TestTrue(TEXT("Lenswright northeast of tree"), Lenswright.Location.X > 0.0 && Lenswright.Location.Y > 0.0);
	TestTrue(TEXT("Kinwild southeast of tree"), Kinwild.Location.X > 0.0 && Kinwild.Location.Y < 0.0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonValleyLandmarkScaleRelativeToPlayerTest,
	"ArchonFactory.Valley.LandmarksScaleRelativeToPlayer",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FArchonValleyLandmarkScaleRelativeToPlayerTest::RunTest(const FString& Parameters)
{
	const TArray<FArchonValleyPlacement> Layout = UArchonSplitrootValleyLayoutLibrary::BuildSplitrootValleyLayout();
	const double PlayerHeight = UArchonFactionSilhouetteLibrary::GetBaselineCapsuleHeightUnits();

	FArchonValleyPlacement KinwildCamp;
	TestTrue(TEXT("Kinwild camp exists"), FindSingle(Layout, TEXT("kinwild_base"), KinwildCamp));
	TestTrue(
		TEXT("Kinwild camp is at least five players tall"),
		KinwildCamp.Scale.Z * 100.0 >= PlayerHeight * 5.0);
	TestTrue(
		TEXT("Kinwild camp footprint reads as a structure, not a prop"),
		FMath::Min(KinwildCamp.Scale.X, KinwildCamp.Scale.Y) * 100.0 >= PlayerHeight * 10.0);

	const TArray<FArchonValleyPlacement> Ridges =
		UArchonSplitrootValleyLayoutLibrary::FilterPlacementsByPiece(Layout, EArchonValleyPiece::RidgeSlab);
	for (const FArchonValleyPlacement& Ridge : Ridges)
	{
		TestTrue(
			*FString::Printf(TEXT("%s ridge is taller than eight players"), *Ridge.PieceId.ToString()),
			Ridge.Scale.Z * 100.0 >= PlayerHeight * 8.0);
	}

	const TArray<FArchonValleyPlacement> CoverStones =
		UArchonSplitrootValleyLayoutLibrary::FilterPlacementsByPiece(Layout, EArchonValleyPiece::CoverStone);
	for (const FArchonValleyPlacement& Stone : CoverStones)
	{
		TestTrue(
			*FString::Printf(TEXT("%s cover stone is vault-size, not mountain-size"), *Stone.PieceId.ToString()),
			Stone.Scale.Z * 100.0 >= PlayerHeight * 1.0 &&
			Stone.Scale.Z * 100.0 <= PlayerHeight * 3.0);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonValleyCoverLaneSpacingTest,
	"ArchonFactory.Valley.CoverLanesSpacedForMovementVerbs",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FArchonValleyCoverLaneSpacingTest::RunTest(const FString& Parameters)
{
	const TArray<FArchonValleyPlacement> Layout = UArchonSplitrootValleyLayoutLibrary::BuildSplitrootValleyLayout();
	const TArray<FArchonValleyPlacement> Stones =
		UArchonSplitrootValleyLayoutLibrary::FilterPlacementsByPiece(Layout, EArchonValleyPiece::CoverStone);

	TestTrue(TEXT("Three lanes contribute stones"), Stones.Num() >= 15);

	const TCHAR* LanePrefixes[3] = { TEXT("lane_west"), TEXT("lane_northeast"), TEXT("lane_southeast") };
	for (const TCHAR* LanePrefix : LanePrefixes)
	{
		TArray<FArchonValleyPlacement> LaneStones;
		for (const FArchonValleyPlacement& Stone : Stones)
		{
			if (Stone.PieceId.ToString().StartsWith(LanePrefix))
			{
				LaneStones.Add(Stone);
			}
		}
		TestTrue(
			*FString::Printf(TEXT("%s has at least 4 stones"), LanePrefix),
			LaneStones.Num() >= 4);

		// Adjacent stones stay within one movement-verb hop of each other.
		for (int32 Index = 1; Index < LaneStones.Num(); ++Index)
		{
			const double Gap = FVector::Dist2D(LaneStones[Index - 1].Location, LaneStones[Index].Location);
			TestTrue(
				*FString::Printf(TEXT("%s gap %d within hop range"), LanePrefix, Index),
				Gap >= 0.5 * UArchonSplitrootValleyLayoutLibrary::CoverStoneLaneSpacing &&
				Gap <= 1.4 * UArchonSplitrootValleyLayoutLibrary::CoverStoneLaneSpacing);
		}
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonValleyMapReadHotZonesStayOpenTest,
	"ArchonFactory.Valley.MapReadHotZonesStayOpen",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FArchonValleyMapReadHotZonesStayOpenTest::RunTest(const FString& Parameters)
{
	const TArray<FArchonValleyPlacement> Layout = UArchonSplitrootValleyLayoutLibrary::BuildSplitrootValleyLayout();

	for (const FArchonValleyPlacement& Placement : Layout)
	{
		if (Placement.Piece == EArchonValleyPiece::DecorTree || Placement.Piece == EArchonValleyPiece::DecorRock)
		{
			TestTrue(
				*FString::Printf(TEXT("%s leaves replay hot zone visually open"), *Placement.PieceId.ToString()),
				DistanceToNearestMapReadHotZone(Placement.Location) >= 1700.0);
		}
		if (Placement.Piece == EArchonValleyPiece::CoverStone)
		{
			TestTrue(
				*FString::Printf(TEXT("%s stays clear of replay stuck center"), *Placement.PieceId.ToString()),
				DistanceToNearestMapReadHotZone(Placement.Location) >= 1100.0);
		}
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonValleyDressingStaysOffPrimaryRoutesTest,
	"ArchonFactory.Valley.DressingStaysOffPrimaryRoutes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FArchonValleyDressingStaysOffPrimaryRoutesTest::RunTest(const FString& Parameters)
{
	const TArray<FArchonValleyPlacement> Layout = UArchonSplitrootValleyLayoutLibrary::BuildSplitrootValleyLayout();
	for (const FArchonValleyPlacement& Placement : Layout)
	{
		if (Placement.Piece == EArchonValleyPiece::DecorTree || Placement.Piece == EArchonValleyPiece::DecorRock)
		{
			TestTrue(
				*FString::Printf(TEXT("%s leaves the primary travel corridor readable"), *Placement.PieceId.ToString()),
				DistanceToNearestPrimaryRoute(Placement.Location) >= 1450.0);
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonValleyPrimaryRoutesAvoidReplayHotZonesTest,
	"ArchonFactory.Valley.PrimaryRoutesAvoidReplayHotZones",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FArchonValleyPrimaryRoutesAvoidReplayHotZonesTest::RunTest(const FString& Parameters)
{
	struct FRouteSegment
	{
		const TCHAR* Name;
		FVector2D A;
		FVector2D B;
	};
	static const FRouteSegment RouteSegments[] = {
		{ TEXT("west_to_center"), FVector2D(-15000.0, 0.0), FVector2D(-2400.0, 0.0) },
		{ TEXT("west_to_south_site"), FVector2D(-15000.0, 0.0), FVector2D(-4000.0, -7000.0) },
		{ TEXT("lens_to_lower_bend"), FVector2D(14500.0, 5200.0), FVector2D(7800.0, 1200.0) },
		{ TEXT("lens_lower_bend_to_center"), FVector2D(7800.0, 1200.0), FVector2D(2000.0, 1400.0) },
		{ TEXT("kinwild_to_center"), FVector2D(12000.0, -10000.0), FVector2D(2000.0, -1400.0) },
		{ TEXT("lens_mid_to_center"), FVector2D(2000.0, 1400.0), FVector2D(0.0, -3000.0) },
		{ TEXT("kinwild_mid_to_center"), FVector2D(2000.0, -1400.0), FVector2D(0.0, -3000.0) },
		{ TEXT("north_site_to_center"), FVector2D(0.0, 7000.0), FVector2D(0.0, -3000.0) }
	};
	static const FVector2D HotZones[] = {
		FVector2D(-6000.0, 7000.0),
		FVector2D(-6000.0, 5000.0),
		FVector2D(-5000.0, 5000.0),
		FVector2D(5000.0, 4000.0),
		FVector2D(5000.0, 5000.0),
		FVector2D(11000.0, 8000.0),
		FVector2D(10000.0, 9000.0),
		FVector2D(12000.0, 9000.0)
	};

	for (const FRouteSegment& Segment : RouteSegments)
	{
		for (const FVector2D& HotZone : HotZones)
		{
			const double Distance = DistanceToRouteSegment(FVector(HotZone.X, HotZone.Y, 0.0), Segment.A, Segment.B);
			TestTrue(
				*FString::Printf(TEXT("%s route clears replay hot zone %.0f,%.0f"), Segment.Name, HotZone.X, HotZone.Y),
				Distance >= 1800.0);
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonValleyCommandSurfaceInVerdantCornerTest,
	"ArchonFactory.Valley.CommandSurfaceAndSpawnsInVerdantCorner",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FArchonValleyCommandSurfaceInVerdantCornerTest::RunTest(const FString& Parameters)
{
	const TArray<FArchonValleyPlacement> Layout = UArchonSplitrootValleyLayoutLibrary::BuildSplitrootValleyLayout();

	FArchonValleyPlacement VerdantBase;
	FArchonValleyPlacement MapTable;
	FArchonValleyPlacement PlayerSpawn;
	FArchonValleyPlacement SquadSpawn;
	TestTrue(TEXT("Verdant base exists"), FindSingle(Layout, TEXT("verdant_base"), VerdantBase));
	TestTrue(TEXT("Map table exists"), FindSingle(Layout, TEXT("valley_map_table"), MapTable));
	TestTrue(TEXT("Player spawn exists"), FindSingle(Layout, TEXT("valley_player_spawn"), PlayerSpawn));
	TestTrue(TEXT("Squad spawn exists"), FindSingle(Layout, TEXT("valley_squad_spawn"), SquadSpawn));

	TestEqual(TEXT("Map table belongs to team 0"), MapTable.TeamId, 0);
	TestTrue(
		TEXT("Map table within the Verdant corner"),
		FVector::Dist2D(MapTable.Location, VerdantBase.Location) <= 2600.0);
	TestTrue(
		TEXT("Player spawn within the Verdant corner"),
		FVector::Dist2D(PlayerSpawn.Location, VerdantBase.Location) <= 3400.0);
	TestTrue(
		TEXT("Squad spawn within the Verdant corner"),
		FVector::Dist2D(SquadSpawn.Location, VerdantBase.Location) <= 3400.0);

	// The first thing a spawning player sees must be the tree.
	const FVector ToTree = (FVector::ZeroVector - PlayerSpawn.Location).GetSafeNormal2D();
	const FVector SpawnForward = PlayerSpawn.Rotation.Vector().GetSafeNormal2D();
	TestTrue(
		TEXT("Player spawn faces the Central Splitroot"),
		FVector::DotProduct(ToTree, SpawnForward) > 0.95);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonValleyMatchObjectivesTest,
	"ArchonFactory.Valley.MatchObjectivesPlacedForTwoTeamV0",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FArchonValleyMatchObjectivesTest::RunTest(const FString& Parameters)
{
	const TArray<FArchonValleyPlacement> Layout = UArchonSplitrootValleyLayoutLibrary::BuildSplitrootValleyLayout();

	// v0 spec: two-team match, win by destroying the enemy base core.
	TestEqual(
		TEXT("Exactly two base cores"),
		UArchonSplitrootValleyLayoutLibrary::FilterPlacementsByPiece(Layout, EArchonValleyPiece::BaseCore).Num(),
		2);

	FArchonValleyPlacement VerdantBase;
	FArchonValleyPlacement VerdantCore;
	FArchonValleyPlacement LenswrightBase;
	FArchonValleyPlacement LenswrightCore;
	TestTrue(TEXT("Verdant base exists"), FindSingle(Layout, TEXT("verdant_base"), VerdantBase));
	TestTrue(TEXT("Verdant core exists"), FindSingle(Layout, TEXT("verdant_base_core"), VerdantCore));
	TestTrue(TEXT("Lenswright base exists"), FindSingle(Layout, TEXT("lenswright_base"), LenswrightBase));
	TestTrue(TEXT("Lenswright core exists"), FindSingle(Layout, TEXT("lenswright_base_core"), LenswrightCore));

	TestEqual(TEXT("Verdant core team id"), VerdantCore.TeamId, 0);
	TestEqual(TEXT("Lenswright core team id"), LenswrightCore.TeamId, 1);
	TestTrue(
		TEXT("Verdant core inside its base corner"),
		FVector::Dist2D(VerdantCore.Location, VerdantBase.Location) <= 2500.0);
	TestTrue(
		TEXT("Lenswright core inside its base corner"),
		FVector::Dist2D(LenswrightCore.Location, LenswrightBase.Location) <= 2500.0);
	TestTrue(TEXT("Cores on opposite halves"), VerdantCore.Location.X < 0.0 && LenswrightCore.Location.X > 0.0);

	// v0 spec: three neutral resource sites force teams out of base.
	const TArray<FArchonValleyPlacement> Sites =
		UArchonSplitrootValleyLayoutLibrary::FilterPlacementsByPiece(Layout, EArchonValleyPiece::ResourceSite);
	TestEqual(TEXT("Exactly three resource sites"), Sites.Num(), 3);
	for (const FArchonValleyPlacement& Site : Sites)
	{
		TestEqual(
			*FString::Printf(TEXT("%s is neutral"), *Site.PieceId.ToString()),
			Site.TeamId,
			static_cast<int32>(INDEX_NONE));
	}

	FArchonValleyPlacement Central;
	FArchonValleyPlacement North;
	FArchonValleyPlacement South;
	TestTrue(TEXT("Central site exists"), FindSingle(Layout, TEXT("resource_site_central"), Central));
	TestTrue(TEXT("North site exists"), FindSingle(Layout, TEXT("resource_site_north"), North));
	TestTrue(TEXT("South site exists"), FindSingle(Layout, TEXT("resource_site_south"), South));
	TestTrue(
		TEXT("Central site contests the tree"),
		FVector::Dist2D(Central.Location, FVector::ZeroVector) <= 4000.0);
	TestTrue(TEXT("North site holds the north flank"), North.Location.Y >= 5000.0);
	TestTrue(TEXT("South site holds the south flank"), South.Location.Y <= -5000.0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonValleyAuthoredDressingTest,
	"ArchonFactory.Valley.AuthoredDressingBreaksUpTheField",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FArchonValleyAuthoredDressingTest::RunTest(const FString& Parameters)
{
	const TArray<FArchonValleyPlacement> Layout = UArchonSplitrootValleyLayoutLibrary::BuildSplitrootValleyLayout();
	const TArray<FArchonValleyPlacement> Trees =
		UArchonSplitrootValleyLayoutLibrary::FilterPlacementsByPiece(Layout, EArchonValleyPiece::DecorTree);
	const TArray<FArchonValleyPlacement> Rocks =
		UArchonSplitrootValleyLayoutLibrary::FilterPlacementsByPiece(Layout, EArchonValleyPiece::DecorRock);
	const TArray<FArchonValleyPlacement> PlaceholderFoliage =
		UArchonSplitrootValleyLayoutLibrary::FilterPlacementsByPiece(Layout, EArchonValleyPiece::DecorFoliage);

	TestTrue(TEXT("Authored deadwood clusters: at least 40 tree placements"), Trees.Num() >= 40);
	TestTrue(TEXT("Authored stone and ice clusters: at least 70 rock placements"), Rocks.Num() >= 70);
	TestEqual(TEXT("No placeholder foliage scatter"), PlaceholderFoliage.Num(), 0);

	const auto TestDressingPlacement = [this](const FArchonValleyPlacement& Placement)
	{
		TestTrue(
			*FString::Printf(TEXT("%s sits on the valley floor"), *Placement.PieceId.ToString()),
			FMath::Abs(Placement.Location.X) <= UArchonSplitrootValleyLayoutLibrary::ValleyFloorHalfLengthX &&
			FMath::Abs(Placement.Location.Y) <= UArchonSplitrootValleyLayoutLibrary::ValleyFloorHalfWidthY &&
			FMath::IsNearlyZero(Placement.Location.Z));
	};
	for (const FArchonValleyPlacement& Placement : Trees)
	{
		TestDressingPlacement(Placement);
	}
	for (const FArchonValleyPlacement& Placement : Rocks)
	{
		TestDressingPlacement(Placement);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonValleyPiecesInsideValleyTest,
	"ArchonFactory.Valley.AllPiecesInsideValleyBounds",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FArchonValleyPiecesInsideValleyTest::RunTest(const FString& Parameters)
{
	const TArray<FArchonValleyPlacement> Layout = UArchonSplitrootValleyLayoutLibrary::BuildSplitrootValleyLayout();

	for (const FArchonValleyPlacement& Placement : Layout)
	{
		if (Placement.Piece == EArchonValleyPiece::RidgeSlab)
		{
			continue; // Ridges are the valley walls; they sit on the boundary.
		}
		TestTrue(
			*FString::Printf(TEXT("%s inside valley X bounds"), *Placement.PieceId.ToString()),
			FMath::Abs(Placement.Location.X) <= UArchonSplitrootValleyLayoutLibrary::ValleyFloorHalfLengthX);
		TestTrue(
			*FString::Printf(TEXT("%s inside valley Y bounds"), *Placement.PieceId.ToString()),
			FMath::Abs(Placement.Location.Y) <= UArchonSplitrootValleyLayoutLibrary::ValleyFloorHalfWidthY);
	}

	return true;
}
