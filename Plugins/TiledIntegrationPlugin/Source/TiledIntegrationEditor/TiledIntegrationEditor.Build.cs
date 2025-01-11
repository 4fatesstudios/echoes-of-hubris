// Copyright Flekz Games. All Rights Reserved.

using System.IO;
using UnrealBuildTool;

public class TiledIntegrationEditor : ModuleRules
{
	public TiledIntegrationEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PrivateIncludePaths.AddRange(new string[] {
			System.IO.Path.Combine(GetModuleDirectory("Paper2D"), "Private"),
		});

        PublicDependencyModuleNames.AddRange(
			new string[]{ 
				"Core",
				"DeveloperSettings",
				"UnrealEd"
			}
			);
			
		PrivateDependencyModuleNames.AddRange(
			new string[]{ 
				"Blutility",
				"CoreUObject",
				"Engine",
				"EditorFramework",
				"EditorScriptingUtilities",
				"EditorSubsystem",
				"Json",
				"Slate",
				"SlateCore",
				"Paper2D",
				"Paper2DEditor",
				"Projects",
				"TiledIntegration",
				"ToolMenus",
				"UMGEditor"
			}
			);
    }
}
