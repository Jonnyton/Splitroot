using UnrealBuildTool;

public class ArchonFactoryCanary : ModuleRules
{
	public ArchonFactoryCanary(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new[]
		{
			"AIModule",
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"Json",
			"NavigationSystem",
			"PhysicsCore",
			"Slate",
			"SlateCore",
			"UMG"
		});
	}
}
