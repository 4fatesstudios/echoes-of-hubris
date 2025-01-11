// Copyright Flekz Games. All Rights Reserved.

using UnrealBuildTool;

public class TiledIntegration : ModuleRules
{
	public TiledIntegration(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateIncludePaths.AddRange(
			new string[] {
				System.IO.Path.Combine(GetModuleDirectory("Paper2D"), "Private"),
			}
			);

		PublicDependencyModuleNames.AddRange(
			new string[]{ 
				"Core"
			}
			);
			
		PrivateDependencyModuleNames.AddRange(
			new string[]{ 
				"CoreUObject",
				"Engine",
				"Json",
				"Paper2D",
				"RenderCore",
                "RHI",
                "Slate",
				"SlateCore"
			}
			);
    }
}
