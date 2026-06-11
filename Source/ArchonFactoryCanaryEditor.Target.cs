using UnrealBuildTool;
using System.Collections.Generic;

public class ArchonFactoryCanaryEditorTarget : TargetRules
{
	public ArchonFactoryCanaryEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("ArchonFactoryCanary");
	}
}
