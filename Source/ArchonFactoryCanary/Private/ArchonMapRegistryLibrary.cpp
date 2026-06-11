#include "ArchonMapRegistryLibrary.h"

#include "Dom/JsonObject.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
	TArray<FArchonMapEntry> GetFallbackSplitrootMapRotation()
	{
		TArray<FArchonMapEntry> Rotation;

		FArchonMapEntry Valley;
		Valley.MapId = TEXT("splitroot_valley");
		Valley.DisplayName = TEXT("Splitroot Valley");
		Valley.MapUrl = TEXT("/Engine/Maps/Entry");
		Valley.UrlOptions = {
			TEXT("ArchonSplitrootValley"),
			TEXT("ArchonMatchLoop"),
			TEXT("game=/Script/ArchonFactoryCanary.ArchonMatchGameMode") };
		Rotation.Add(Valley);

		FArchonMapEntry Range;
		Range.MapId = TEXT("canary_range");
		Range.DisplayName = TEXT("Canary Range (template)");
		Range.MapUrl = TEXT("/Game/FirstPerson/Lvl_FirstPerson");
		Rotation.Add(Range);

		return Rotation;
	}

	bool TryReadStringArray(const TSharedPtr<FJsonObject>& Object, const TCHAR* FieldName, TArray<FString>& OutValues)
	{
		if (!Object.IsValid())
		{
			return false;
		}

		const TArray<TSharedPtr<FJsonValue>>* JsonValues = nullptr;
		if (!Object->TryGetArrayField(FieldName, JsonValues))
		{
			return false;
		}

		for (const TSharedPtr<FJsonValue>& Value : *JsonValues)
		{
			FString StringValue;
			if (Value.IsValid() && Value->TryGetString(StringValue))
			{
				OutValues.Add(StringValue);
			}
		}
		return true;
	}
}

TArray<FArchonMapEntry> UArchonMapRegistryLibrary::GetSplitrootMapRotation()
{
	TArray<FArchonMapEntry> Rotation;
	FString Error;
	const FString RegistryPath = FPaths::ConvertRelativePathToFull(FPaths::Combine(
		FPaths::ProjectDir(),
		TEXT("games"),
		TEXT("splitroot"),
		TEXT("FactoryContracts"),
		TEXT("map_registry.json")));
	if (LoadMapRotationFromFile(RegistryPath, Rotation, Error) && Rotation.Num() > 0)
	{
		return Rotation;
	}

	UE_LOG(LogTemp, Warning, TEXT("ArchonFactoryCanary: MapRegistryFallback reason=%s path=%s"), *Error, *RegistryPath);
	return GetFallbackSplitrootMapRotation();
}

bool UArchonMapRegistryLibrary::LoadMapRotationFromFile(
	const FString& RegistryPath,
	TArray<FArchonMapEntry>& OutRotation,
	FString& OutError)
{
	OutRotation.Reset();
	OutError.Reset();

	FString JsonText;
	if (!FFileHelper::LoadFileToString(JsonText, *RegistryPath))
	{
		OutError = TEXT("file_not_readable");
		return false;
	}

	TSharedPtr<FJsonObject> Root;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonText);
	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
	{
		OutError = TEXT("json_parse_failed");
		return false;
	}

	const TArray<TSharedPtr<FJsonValue>>* MapValues = nullptr;
	if (!Root->TryGetArrayField(TEXT("maps"), MapValues))
	{
		OutError = TEXT("missing_maps");
		return false;
	}

	TMap<FName, FArchonMapEntry> EntriesById;
	for (const TSharedPtr<FJsonValue>& MapValue : *MapValues)
	{
		const TSharedPtr<FJsonObject> MapObject = MapValue.IsValid() ? MapValue->AsObject() : nullptr;
		if (!MapObject.IsValid())
		{
			continue;
		}

		bool bShipped = true;
		MapObject->TryGetBoolField(TEXT("shipped"), bShipped);
		if (!bShipped)
		{
			continue;
		}

		FString MapIdString;
		FString MapUrl;
		if (!MapObject->TryGetStringField(TEXT("map_id"), MapIdString) ||
			!MapObject->TryGetStringField(TEXT("map_url"), MapUrl) ||
			MapIdString.IsEmpty() ||
			MapUrl.IsEmpty())
		{
			continue;
		}

		FArchonMapEntry Entry;
		Entry.MapId = FName(*MapIdString);
		Entry.MapUrl = MapUrl;
		MapObject->TryGetStringField(TEXT("display_name"), Entry.DisplayName);
		TryReadStringArray(MapObject, TEXT("url_options"), Entry.UrlOptions);
		EntriesById.Add(Entry.MapId, Entry);
	}

	if (EntriesById.Num() == 0)
	{
		OutError = TEXT("no_shipped_maps");
		return false;
	}

	TArray<FString> RotationOrder;
	TryReadStringArray(Root, TEXT("rotation_order"), RotationOrder);
	for (const FString& MapIdString : RotationOrder)
	{
		FArchonMapEntry* Entry = EntriesById.Find(FName(*MapIdString));
		if (Entry)
		{
			OutRotation.Add(*Entry);
			EntriesById.Remove(Entry->MapId);
		}
	}

	for (const TPair<FName, FArchonMapEntry>& Remaining : EntriesById)
	{
		OutRotation.Add(Remaining.Value);
	}

	if (OutRotation.Num() == 0)
	{
		OutError = TEXT("rotation_empty");
		return false;
	}

	OutError = TEXT("ok");
	return true;
}

FString UArchonMapRegistryLibrary::BuildTravelUrl(const FArchonMapEntry& Entry, const TArray<FString>& ExtraOptions)
{
	FString Url = Entry.MapUrl;
	for (const FString& Option : Entry.UrlOptions)
	{
		Url += TEXT("?") + Option;
	}
	Url += FString::Printf(TEXT("?ArchonMapId=%s"), *Entry.MapId.ToString());
	for (const FString& Option : ExtraOptions)
	{
		Url += TEXT("?") + Option;
	}
	return Url;
}

bool UArchonMapRegistryLibrary::FindMapById(const TArray<FArchonMapEntry>& Rotation, FName MapId, FArchonMapEntry& OutEntry)
{
	for (const FArchonMapEntry& Entry : Rotation)
	{
		if (Entry.MapId == MapId)
		{
			OutEntry = Entry;
			return true;
		}
	}
	return false;
}

FArchonMapEntry UArchonMapRegistryLibrary::GetNextMapInRotation(const TArray<FArchonMapEntry>& Rotation, FName CurrentMapId)
{
	if (Rotation.Num() == 0)
	{
		return FArchonMapEntry();
	}
	for (int32 Index = 0; Index < Rotation.Num(); ++Index)
	{
		if (Rotation[Index].MapId == CurrentMapId)
		{
			return Rotation[(Index + 1) % Rotation.Num()];
		}
	}
	return Rotation[0];
}

FArchonMapEntry UArchonMapRegistryLibrary::GetNextMatchMapInRotation(const TArray<FArchonMapEntry>& Rotation, FName CurrentMapId)
{
	if (Rotation.Num() == 0)
	{
		return FArchonMapEntry();
	}

	int32 StartIndex = 0;
	for (int32 Index = 0; Index < Rotation.Num(); ++Index)
	{
		if (Rotation[Index].MapId == CurrentMapId)
		{
			StartIndex = (Index + 1) % Rotation.Num();
			break;
		}
	}

	for (int32 Offset = 0; Offset < Rotation.Num(); ++Offset)
	{
		const FArchonMapEntry& Candidate = Rotation[(StartIndex + Offset) % Rotation.Num()];
		if (Candidate.UrlOptions.Contains(TEXT("ArchonMatchLoop")))
		{
			return Candidate;
		}
	}

	return Rotation[0];
}
