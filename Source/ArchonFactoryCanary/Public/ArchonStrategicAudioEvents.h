#pragma once

#include "CoreMinimal.h"
#include "ArchonStrategicAudioEvents.generated.h"

/**
 * Strategic-layer audio events. Position-agnostic, mix-priority highest.
 * Mirrors games/<game-id>/FactoryContracts/strategic_audio_events.json. Consumed by
 * UArchonFactionAudioLibrary::GetStrategicEventCue and the future
 * UArchonStrategicAudioBroadcasterComponent / UArchonStrategicEventBannerWidget.
 */
UENUM(BlueprintType)
enum class EArchonStrategicAudioEvent : uint8
{
	None UMETA(DisplayName = "None"),
	MatchStart UMETA(DisplayName = "Match Start"),
	SideResourceCapturedAlly UMETA(DisplayName = "Side Resource Captured (Ally)"),
	SideResourceCapturedEnemy UMETA(DisplayName = "Side Resource Captured (Enemy)"),
	CentralResourceFlipped UMETA(DisplayName = "Central Resource Flipped"),
	HeroUnlockedAlly UMETA(DisplayName = "Hero Unlocked (Ally)"),
	HeroUnlockedEnemy UMETA(DisplayName = "Hero Unlocked (Enemy)"),
	HeroArrivedAlly UMETA(DisplayName = "Hero Arrived (Ally)"),
	HeroDeathAlly UMETA(DisplayName = "Hero Death (Ally)"),
	HeroDeathEnemy UMETA(DisplayName = "Hero Death (Enemy)"),
	BaseCoreUnderAttack UMETA(DisplayName = "Base Core Under Attack"),
	BaseCoreDestroyed UMETA(DisplayName = "Base Core Destroyed"),
	FactionPowerActivatedAlly UMETA(DisplayName = "Faction Power Activated (Ally)"),
	MatchVictory UMETA(DisplayName = "Match Victory"),
	MatchDefeat UMETA(DisplayName = "Match Defeat")
};
