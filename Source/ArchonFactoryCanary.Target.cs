using UnrealBuildTool;
using System.Collections.Generic;

public class ArchonFactoryCanaryTarget : TargetRules
{
	public ArchonFactoryCanaryTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("ArchonFactoryCanary");
	}
}
