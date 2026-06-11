#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ArchonFactionTypes.h"
#include "ArchonStrategicAudioEvents.h"
#include "ArchonFactionAudioLibrary.generated.h"

class USoundBase;

/**
 * Faction audio cue lookup. Mirrors games/<game-id>/FactoryContracts/faction_audio_palette.json
 * and games/<game-id>/FactoryContracts/strategic_audio_events.json. Get*Cue functions return
 * nullptr until the audio assets are imported into Content/Audio/<Faction>/
 * — honest absence rather than crashes.
 *
 * ValidateLenswrightCueName protects the no-gunpowder hill at the audio layer:
 * every imported asset assigned to a Lenswright cue MUST pass this predicate
 * before being accepted by the audio library. Substrate audit, not per-game.
 */
UCLASS()
class ARCHONFACTORYCANARY_API UArchonFactionAudioLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Archon|Audio")
	static USoundBase* GetFactionWeaponFireCue(EArchonFaction Faction);

	UFUNCTION(BlueprintPure, Category = "Archon|Audio")
	static USoundBase* GetFactionWeaponImpactCue(EArchonFaction Faction);

	UFUNCTION(BlueprintPure, Category = "Archon|Audio")
	static USoundBase* GetFactionFootstepCue(EArchonFaction Faction);

	UFUNCTION(BlueprintPure, Category = "Archon|Audio")
	static USoundBase* GetFactionHeroAmbientLoop(EArchonFaction Faction);

	UFUNCTION(BlueprintPure, Category = "Archon|Audio")
	static USoundBase* GetFactionDeathCue(EArchonFaction Faction);

	UFUNCTION(BlueprintPure, Category = "Archon|Audio")
	static USoundBase* GetStrategicEventCue(EArchonStrategicAudioEvent Event);

	/** Returns the forbidden firearm-related substrings that Lenswright cues must not contain. */
	UFUNCTION(BlueprintPure, Category = "Archon|Audio")
	static TArray<FString> GetLenswrightForbiddenTerms();

	/**
	 * No-gunpowder hill audit. Returns true if the cue name is acceptable for a
	 * Lenswright audio asset. Case-insensitive substring match against the
	 * forbidden-terms list mirrored from games/<game-id>/FactoryContracts/faction_audio_palette.json.
	 */
	UFUNCTION(BlueprintPure, Category = "Archon|Audio")
	static bool ValidateLenswrightCueName(const FString& CueName);

	/**
	 * Validates a candidate asset name+tag set for a faction cue. Currently the
	 * Lenswright audit is the only enforced rule; Verdant/Kinwild return true.
	 */
	UFUNCTION(BlueprintPure, Category = "Archon|Audio")
	static bool ValidateCueNameForFaction(EArchonFaction Faction, const FString& CueName);
};
