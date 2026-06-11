#include "ArchonLaunchFlagLibrary.h"
#include "Engine/World.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

bool UArchonLaunchFlagLibrary::IsLaunchFlagActive(const UWorld* World, const TCHAR* FlagName)
{
	if (FParse::Param(FCommandLine::Get(), FlagName))
	{
		return true;
	}
	return World && HasUrlOption(World->URL, FlagName);
}

bool UArchonLaunchFlagLibrary::HasUrlOption(const FURL& Url, const TCHAR* FlagName)
{
	return Url.HasOption(FlagName);
}
