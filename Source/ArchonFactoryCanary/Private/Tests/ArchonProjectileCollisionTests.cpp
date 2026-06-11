#if WITH_DEV_AUTOMATION_TESTS

#include "ArchonArrowProjectile.h"
#include "Components/PrimitiveComponent.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FArchonProjectilePawnsOverlapButWorldBlocksTest,
	"ArchonFactory.Projectile.PawnsOverlapButWorldBlocks",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FArchonProjectilePawnsOverlapButWorldBlocksTest::RunTest(const FString& Parameters)
{
	AArchonArrowProjectile* Projectile = NewObject<AArchonArrowProjectile>();
	UPrimitiveComponent* Collision = Cast<UPrimitiveComponent>(Projectile->GetRootComponent());

	TestNotNull(TEXT("Arrow projectile owns primitive root collision"), Collision);
	if (!Collision)
	{
		return false;
	}

	TestEqual(TEXT("Pawns are overlapped for hit application without movement blocking"),
		Collision->GetCollisionResponseToChannel(ECC_Pawn),
		ECR_Overlap);
	TestEqual(TEXT("World static still blocks for terrain/building impact"),
		Collision->GetCollisionResponseToChannel(ECC_WorldStatic),
		ECR_Block);
	TestTrue(TEXT("Pawn overlap events are enabled"), Collision->GetGenerateOverlapEvents());
	return true;
}

#endif
