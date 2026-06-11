#pragma once

#include "CoreMinimal.h"
#include "ArchonSessionTypes.generated.h"

UENUM(BlueprintType)
enum class EArchonSessionRoute : uint8
{
	OfflineSkirmish UMETA(DisplayName = "Offline Skirmish"),
	LANHosted UMETA(DisplayName = "LAN Hosted"),
	PrivateHost UMETA(DisplayName = "Private Host"),
	SteamOnline UMETA(DisplayName = "Steam Online")
};

UENUM(BlueprintType)
enum class EArchonGameplayRight : uint8
{
	CoreCombat UMETA(DisplayName = "Core Combat"),
	Respawn UMETA(DisplayName = "Respawn"),
	DefaultHeroes UMETA(DisplayName = "Default Heroes"),
	AllHeroes UMETA(DisplayName = "All Heroes"),
	HorizontalPaidHeroes UMETA(DisplayName = "Horizontal Paid Heroes"),
	MapTablePreview UMETA(DisplayName = "Map Table Preview"),
	LiveRtsExecution UMETA(DisplayName = "Live RTS Execution")
};

USTRUCT(BlueprintType)
struct FArchonEffectiveAccess
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Archon|Entitlements")
	bool bCoreCombat = true;

	UPROPERTY(BlueprintReadOnly, Category = "Archon|Entitlements")
	bool bRespawn = true;

	UPROPERTY(BlueprintReadOnly, Category = "Archon|Entitlements")
	bool bDefaultHeroes = true;

	UPROPERTY(BlueprintReadOnly, Category = "Archon|Entitlements")
	bool bAllHeroes = false;

	UPROPERTY(BlueprintReadOnly, Category = "Archon|Entitlements")
	bool bHorizontalPaidHeroes = false;

	UPROPERTY(BlueprintReadOnly, Category = "Archon|Entitlements")
	bool bMapTablePreview = true;

	UPROPERTY(BlueprintReadOnly, Category = "Archon|Entitlements")
	bool bLiveRtsExecution = false;
};

USTRUCT(BlueprintType)
struct FArchonHeroEntitlement
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Archon|Entitlements")
	FName HeroId;

	UPROPERTY(BlueprintReadOnly, Category = "Archon|Entitlements")
	bool bIsDefaultHero = true;

	UPROPERTY(BlueprintReadOnly, Category = "Archon|Entitlements")
	bool bIsHorizontalPaidHero = false;

	UPROPERTY(BlueprintReadOnly, Category = "Archon|Entitlements")
	bool bImprovesCombatPower = false;
};
