#include "ArchonSplitrootValleyLayoutLibrary.h"
#include "ArchonFactionPaletteLibrary.h"
#include "ArchonFactionTypes.h"

namespace
{
	FArchonValleyPlacement MakeValleyPlacement(
		const TCHAR* PieceId,
		EArchonValleyPiece Piece,
		const FVector& Location,
		const FRotator& Rotation,
		const FVector& Scale,
		const FLinearColor& DebugColor,
		int32 TeamId = INDEX_NONE)
	{
		FArchonValleyPlacement Placement;
		Placement.PieceId = PieceId;
		Placement.Piece = Piece;
		Placement.Location = Location;
		Placement.Rotation = Rotation;
		Placement.Scale = Scale;
		Placement.DebugColor = DebugColor;
		Placement.TeamId = TeamId;
		return Placement;
	}

	FVector2D PushAwayFromMapReadHotZones(const FVector2D& Point, double SafeRadius)
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

		FVector2D Result = Point;
		for (const FVector2D& HotZone : HotZones)
		{
			const FVector2D Offset = Result - HotZone;
			const double Distance = Offset.Size();
			if (Distance < SafeRadius)
			{
				const FVector2D Direction = Distance > 1.0 ? Offset / Distance : FVector2D(1.0, 0.0);
				Result = HotZone + Direction * SafeRadius;
			}
		}

		Result.X = FMath::Clamp(
			Result.X,
			-UArchonSplitrootValleyLayoutLibrary::ValleyFloorHalfLengthX + 500.0,
			UArchonSplitrootValleyLayoutLibrary::ValleyFloorHalfLengthX - 500.0);
		Result.Y = FMath::Clamp(
			Result.Y,
			-UArchonSplitrootValleyLayoutLibrary::ValleyFloorHalfWidthY + 500.0,
			UArchonSplitrootValleyLayoutLibrary::ValleyFloorHalfWidthY - 500.0);
		return Result;
	}

	FVector2D PushAwayFromRouteCorridors(const FVector2D& Point, double SafeRadius)
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
			{ FVector2D(12000.0, -8500.0), FVector2D(2000.0, -1400.0) },
			{ FVector2D(2000.0, 1400.0), FVector2D(0.0, -3000.0) },
			{ FVector2D(2000.0, -1400.0), FVector2D(0.0, -3000.0) },
			{ FVector2D(0.0, 7000.0), FVector2D(0.0, -3000.0) }
		};

		FVector2D Result = Point;
		for (const FRouteSegment& Segment : RouteSegments)
		{
			const FVector2D AB = Segment.B - Segment.A;
			const double LengthSquared = AB.SizeSquared();
			if (LengthSquared <= 1.0)
			{
				continue;
			}

			const double Alpha = FMath::Clamp(FVector2D::DotProduct(Result - Segment.A, AB) / LengthSquared, 0.0, 1.0);
			const FVector2D Closest = Segment.A + AB * Alpha;
			const FVector2D Offset = Result - Closest;
			const double Distance = Offset.Size();
			if (Distance < SafeRadius)
			{
				const FVector2D Direction = Distance > 1.0 ? Offset / Distance : FVector2D(-AB.Y, AB.X).GetSafeNormal();
				Result = Closest + Direction * SafeRadius;
			}
		}

		Result.X = FMath::Clamp(
			Result.X,
			-UArchonSplitrootValleyLayoutLibrary::ValleyFloorHalfLengthX + 500.0,
			UArchonSplitrootValleyLayoutLibrary::ValleyFloorHalfLengthX - 500.0);
		Result.Y = FMath::Clamp(
			Result.Y,
			-UArchonSplitrootValleyLayoutLibrary::ValleyFloorHalfWidthY + 500.0,
			UArchonSplitrootValleyLayoutLibrary::ValleyFloorHalfWidthY - 500.0);
		return Result;
	}

	FVector2D KeepDecorOutOfGameplayReads(const FVector2D& Point)
	{
		FVector2D Result = PushAwayFromMapReadHotZones(Point, 1800.0);
		Result = PushAwayFromRouteCorridors(Result, 1500.0);
		Result = PushAwayFromMapReadHotZones(Result, 1800.0);
		return Result;
	}

	// Interior breadcrumb stones along a straight lane, endpoints excluded.
	// Alternating perpendicular jitter keeps the lane organic without breaking
	// the one-hop spacing the root-vault verb is tuned for.
	void AppendCoverStoneLane(
		TArray<FArchonValleyPlacement>& Layout,
		const TCHAR* LanePrefix,
		const FVector2D& From,
		const FVector2D& To,
		const FLinearColor& StoneColor)
	{
		const FVector2D Delta = To - From;
		const double LaneLength = Delta.Size();
		const int32 Steps = FMath::Max(2, FMath::CeilToInt(
			LaneLength / UArchonSplitrootValleyLayoutLibrary::CoverStoneLaneSpacing));
		const FVector2D Direction = Delta / LaneLength;
		const FVector2D Perpendicular(-Direction.Y, Direction.X);

		for (int32 Step = 1; Step < Steps; ++Step)
		{
			const double Alpha = static_cast<double>(Step) / static_cast<double>(Steps);
			const double Jitter = (Step % 2 == 0) ? 400.0 : -400.0;
			const FVector2D Point = PushAwayFromMapReadHotZones(
				From + Delta * Alpha + Perpendicular * Jitter,
				1200.0);
			Layout.Add(MakeValleyPlacement(
				*FString::Printf(TEXT("%s_stone_%d"), LanePrefix, Step),
				EArchonValleyPiece::CoverStone,
				FVector(Point.X, Point.Y, 175.0),
				FRotator(0.0, Jitter > 0.0 ? 15.0 : -15.0, 0.0),
				FVector(2.4, 2.4, 2.2),
			StoneColor));
		}
	}

	void AppendDecorCluster(
		TArray<FArchonValleyPlacement>& Layout,
		const TCHAR* Prefix,
		EArchonValleyPiece Piece,
		const FVector2D& Center,
		const FVector2D& Radius,
		int32 Count,
		double Seed,
		double BaseScale,
		const FLinearColor& Color)
	{
		for (int32 Index = 0; Index < Count; ++Index)
		{
			const double T = static_cast<double>(Index + 1);
			const double Angle = FMath::DegreesToRadians(FMath::Fmod(T * 137.5 + Seed, 360.0));
			const double Radial = 0.25 + 0.75 * (FMath::Fmod(T * 37.0 + Seed, 100.0) / 100.0);
			const double X = FMath::Clamp(
				Center.X + FMath::Cos(Angle) * Radius.X * Radial,
				-UArchonSplitrootValleyLayoutLibrary::ValleyFloorHalfLengthX + 500.0,
				UArchonSplitrootValleyLayoutLibrary::ValleyFloorHalfLengthX - 500.0);
			const double Y = FMath::Clamp(
				Center.Y + FMath::Sin(Angle) * Radius.Y * Radial,
				-UArchonSplitrootValleyLayoutLibrary::ValleyFloorHalfWidthY + 500.0,
				UArchonSplitrootValleyLayoutLibrary::ValleyFloorHalfWidthY - 500.0);
			const FVector2D Point = KeepDecorOutOfGameplayReads(FVector2D(X, Y));
			const double ScaleJitter = FMath::Fmod(T * 11.0 + Seed, 100.0) / 100.0;
			const double Yaw = FMath::Fmod(T * 73.0 + Seed, 360.0);
			Layout.Add(MakeValleyPlacement(
				*FString::Printf(TEXT("%s_%02d"), Prefix, Index),
				Piece,
				FVector(Point.X, Point.Y, 0.0),
				FRotator(0.0, Yaw, 0.0),
				FVector(BaseScale + ScaleJitter * 0.55),
				Color));
		}
	}
}

TArray<FArchonValleyPlacement> UArchonSplitrootValleyLayoutLibrary::BuildSplitrootValleyLayout()
{
	TArray<FArchonValleyPlacement> Layout;

	const FLinearColor GroundColor = UArchonFactionPaletteLibrary::GetNeutralColor(EArchonNeutralPaletteSlot::GroundEarth);
	const FLinearColor StoneColor = UArchonFactionPaletteLibrary::GetNeutralColor(EArchonNeutralPaletteSlot::CoverStone);
	const FLinearColor WoodColor = UArchonFactionPaletteLibrary::GetNeutralColor(EArchonNeutralPaletteSlot::SplitrootWood);
	const FLinearColor VerdantPrimary = UArchonFactionPaletteLibrary::GetFactionColor(EArchonFaction::VerdantChoir, EArchonFactionPaletteSlot::Primary);
	const FLinearColor LenswrightTertiary = UArchonFactionPaletteLibrary::GetFactionColor(EArchonFaction::LenswrightCompact, EArchonFactionPaletteSlot::Tertiary);
	const FLinearColor KinwildPrimary = UArchonFactionPaletteLibrary::GetFactionColor(EArchonFaction::KinwildCovenant, EArchonFactionPaletteSlot::Primary);

	// Valley floor and walls.
	Layout.Add(MakeValleyPlacement(
		TEXT("valley_floor"), EArchonValleyPiece::GroundPlane,
		FVector(0.0, 0.0, -50.0), FRotator::ZeroRotator,
		FVector(ValleyFloorHalfLengthX / 50.0, ValleyFloorHalfWidthY / 50.0, 1.0),
		GroundColor));
	Layout.Add(MakeValleyPlacement(
		TEXT("ridge_north"), EArchonValleyPiece::RidgeSlab,
		FVector(0.0, ValleyFloorHalfWidthY + 600.0, 850.0), FRotator::ZeroRotator,
		FVector(2.0 * ValleyFloorHalfLengthX / 100.0 + 20.0, 14.0, 18.0),
		StoneColor));
	Layout.Add(MakeValleyPlacement(
		TEXT("ridge_south"), EArchonValleyPiece::RidgeSlab,
		FVector(0.0, -(ValleyFloorHalfWidthY + 600.0), 850.0), FRotator::ZeroRotator,
		FVector(2.0 * ValleyFloorHalfLengthX / 100.0 + 20.0, 14.0, 18.0),
		StoneColor));

	// The Central Splitroot — the landmark the whole valley is named for.
	// Trunk owns the skyline; canopy gives it a silhouette from every base.
	Layout.Add(MakeValleyPlacement(
		TEXT("central_splitroot_trunk"), EArchonValleyPiece::CentralTreeTrunk,
		FVector(0.0, 0.0, CentralTreeTrunkHeight * 0.5), FRotator::ZeroRotator,
		FVector(12.0, 12.0, CentralTreeTrunkHeight / 100.0),
		WoodColor));
	// Canopy must read as foliage from every base. VerdantSecondary is the
	// cream slot — rendered iteration 4 proved it reads as a mushroom-table.
	Layout.Add(MakeValleyPlacement(
		TEXT("central_splitroot_canopy"), EArchonValleyPiece::CentralTreeCanopy,
		FVector(0.0, 0.0, CentralTreeTrunkHeight + 400.0), FRotator::ZeroRotator,
		FVector(55.0, 55.0, 9.0),
		VerdantPrimary));
	for (int32 ButtressIndex = 0; ButtressIndex < 6; ++ButtressIndex)
	{
		const double YawDegrees = 60.0 * static_cast<double>(ButtressIndex);
		const double YawRadians = FMath::DegreesToRadians(YawDegrees);
		const FVector Direction(FMath::Cos(YawRadians), FMath::Sin(YawRadians), 0.0);
		Layout.Add(MakeValleyPlacement(
			*FString::Printf(TEXT("central_splitroot_root_%d"), ButtressIndex),
			EArchonValleyPiece::RootButtress,
			Direction * 1500.0 + FVector(0.0, 0.0, 220.0),
			FRotator(0.0, YawDegrees, 0.0),
			FVector(22.0, 4.0, 4.4),
			WoodColor));
	}

	// Three faction corners. Verdant holds the west; Lenswright surveys from
	// the northeast; Kinwild dens in the southeast. Pads tint the ground so
	// each corner reads as owned territory before real art exists.
	const FVector VerdantBase(-15000.0, 0.0, 0.0);
	const FVector LenswrightBase(14500.0, 5200.0, 0.0);
	const FVector KinwildBase(12000.0, -8500.0, 0.0);

	Layout.Add(MakeValleyPlacement(
		TEXT("verdant_base"), EArchonValleyPiece::VerdantOutpost,
		VerdantBase + FVector(0.0, 0.0, 200.0), FRotator::ZeroRotator,
		FVector::OneVector, VerdantPrimary, 0));
	Layout.Add(MakeValleyPlacement(
		TEXT("verdant_base_core"), EArchonValleyPiece::BaseCore,
		VerdantBase + FVector(-1400.0, 1200.0, 600.0), FRotator::ZeroRotator,
		FVector(6.0, 6.0, 12.0), VerdantPrimary, 0));
	// Renegade-style auto-defense (decision 5): present and testable but
	// tuned almost-worthless; it must still eventually kill a unit.
	Layout.Add(MakeValleyPlacement(
		TEXT("verdant_defense_tower"), EArchonValleyPiece::DefenseTower,
		VerdantBase + FVector(-200.0, 1000.0, 250.0), FRotator::ZeroRotator,
		FVector(1.2, 1.2, 5.0), VerdantPrimary, 0));

	Layout.Add(MakeValleyPlacement(
		TEXT("lenswright_base"), EArchonValleyPiece::LenswrightOutpost,
		LenswrightBase + FVector(0.0, 0.0, 300.0), FRotator(0.0, -135.0, 0.0),
		FVector::OneVector, LenswrightTertiary, 1));
	const FLinearColor LenswrightPrimary = UArchonFactionPaletteLibrary::GetFactionColor(EArchonFaction::LenswrightCompact, EArchonFactionPaletteSlot::Primary);
	Layout.Add(MakeValleyPlacement(
		TEXT("lenswright_base_core"), EArchonValleyPiece::BaseCore,
		LenswrightBase + FVector(1400.0, 1100.0, 600.0), FRotator::ZeroRotator,
		FVector(6.0, 6.0, 12.0), LenswrightPrimary, 1));
	Layout.Add(MakeValleyPlacement(
		TEXT("lenswright_defense_tower"), EArchonValleyPiece::DefenseTower,
		LenswrightBase + FVector(600.0, 900.0, 250.0), FRotator::ZeroRotator,
		FVector(1.2, 1.2, 5.0), LenswrightPrimary, 1));

	Layout.Add(MakeValleyPlacement(
		TEXT("kinwild_base"), EArchonValleyPiece::KinwildCamp,
		KinwildBase + FVector(0.0, 0.0, 250.0), FRotator(0.0, 135.0, 0.0),
		FVector(24.0, 24.0, 12.0), KinwildPrimary, 2));

	// Breadcrumb cover lanes — one hop apart so each faction's movement verb
	// chains stone to stone toward the tree.
	AppendCoverStoneLane(Layout, TEXT("lane_west"), FVector2D(-12800.0, 0.0), FVector2D(-2400.0, 0.0), StoneColor);
	AppendCoverStoneLane(Layout, TEXT("lane_northeast"), FVector2D(12800.0, 4300.0), FVector2D(2200.0, 1300.0), StoneColor);
	AppendCoverStoneLane(Layout, TEXT("lane_southeast"), FVector2D(10400.0, -7200.0), FVector2D(2000.0, -1400.0), StoneColor);

	// Match objectives — v0 spec: win by destroying the enemy base core;
	// three neutral resource sites pull both teams out of base. Central site
	// sits at the tree's southern footing; each team gets one near flank.
	const FLinearColor SiteColor = UArchonFactionPaletteLibrary::GetNeutralColor(EArchonNeutralPaletteSlot::SkyHorizon);
	Layout.Add(MakeValleyPlacement(
		TEXT("resource_site_central"), EArchonValleyPiece::ResourceSite,
		FVector(0.0, -3000.0, 30.0), FRotator::ZeroRotator,
		FVector(12.0, 12.0, 0.6), SiteColor));
	Layout.Add(MakeValleyPlacement(
		TEXT("resource_site_north"), EArchonValleyPiece::ResourceSite,
		FVector(0.0, 7000.0, 30.0), FRotator::ZeroRotator,
		FVector(12.0, 12.0, 0.6), SiteColor));
	Layout.Add(MakeValleyPlacement(
		TEXT("resource_site_south"), EArchonValleyPiece::ResourceSite,
		FVector(-4000.0, -7000.0, 30.0), FRotator::ZeroRotator,
		FVector(12.0, 12.0, 0.6), SiteColor));

	// Command surface and spawns, all inside the Verdant corner.
	Layout.Add(MakeValleyPlacement(
		TEXT("valley_map_table"), EArchonValleyPiece::MapTable,
		VerdantBase + FVector(-1800.0, -1200.0, 120.0), FRotator::ZeroRotator,
		FVector::OneVector, VerdantPrimary, 0));

	// Quarry map pass: authored premade-asset clusters replace the old
	// uniform placeholder scatter. Keep lanes readable and let the edges
	// carry the visual weight.
	const FLinearColor DecorGreen(0.25f, 0.45f, 0.22f);
	const FLinearColor DecorGrey(0.45f, 0.44f, 0.42f);
	AppendDecorCluster(Layout, TEXT("deadwood_west"), EArchonValleyPiece::DecorTree, FVector2D(-11100.0, 4200.0), FVector2D(4200.0, 1800.0), 24, 11.0, 1.25, DecorGreen);
	AppendDecorCluster(Layout, TEXT("deadwood_southwest"), EArchonValleyPiece::DecorTree, FVector2D(-7800.0, -7700.0), FVector2D(3300.0, 1700.0), 16, 29.0, 1.05, DecorGreen);
	AppendDecorCluster(Layout, TEXT("lens_ruin_stones"), EArchonValleyPiece::DecorRock, FVector2D(10000.0, 6500.0), FVector2D(2900.0, 1700.0), 24, 43.0, 1.25, DecorGrey);
	AppendDecorCluster(Layout, TEXT("north_ice_shelf"), EArchonValleyPiece::DecorRock, FVector2D(-4700.0, 10100.0), FVector2D(7600.0, 1200.0), 22, 67.0, 1.5, DecorGrey);
	AppendDecorCluster(Layout, TEXT("kinwild_stonebreak"), EArchonValleyPiece::DecorRock, FVector2D(9200.0, -7700.0), FVector2D(3700.0, 1900.0), 24, 89.0, 1.25, DecorGrey);
	const FVector PlayerSpawnLocation = VerdantBase + FVector(-2600.0, -1600.0, 180.0);
	const FVector TreeFocus(0.0, 0.0, 0.0);
	const FRotator PlayerSpawnRotation = FRotator(
		0.0, (TreeFocus - PlayerSpawnLocation).Rotation().Yaw, 0.0);
	Layout.Add(MakeValleyPlacement(
		TEXT("valley_player_spawn"), EArchonValleyPiece::PlayerSpawn,
		PlayerSpawnLocation, PlayerSpawnRotation,
		FVector::OneVector, VerdantPrimary, 0));
	Layout.Add(MakeValleyPlacement(
		TEXT("valley_squad_spawn"), EArchonValleyPiece::SquadSpawn,
		VerdantBase + FVector(-1800.0, -2300.0, 120.0), PlayerSpawnRotation,
		FVector::OneVector, VerdantPrimary, 0));

	return Layout;
}

TArray<FArchonValleyPlacement> UArchonSplitrootValleyLayoutLibrary::FilterPlacementsByPiece(
	const TArray<FArchonValleyPlacement>& Layout,
	EArchonValleyPiece Piece)
{
	TArray<FArchonValleyPlacement> Filtered;
	for (const FArchonValleyPlacement& Placement : Layout)
	{
		if (Placement.Piece == Piece)
		{
			Filtered.Add(Placement);
		}
	}
	return Filtered;
}

bool UArchonSplitrootValleyLayoutLibrary::FindPlacementById(
	const TArray<FArchonValleyPlacement>& Layout,
	FName PieceId,
	FArchonValleyPlacement& OutPlacement)
{
	for (const FArchonValleyPlacement& Placement : Layout)
	{
		if (Placement.PieceId == PieceId)
		{
			OutPlacement = Placement;
			return true;
		}
	}
	return false;
}
