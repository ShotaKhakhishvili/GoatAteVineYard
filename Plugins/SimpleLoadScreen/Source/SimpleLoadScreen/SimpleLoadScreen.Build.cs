// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class SimpleLoadScreen : ModuleRules
{
	public SimpleLoadScreen(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PrivateDependencyModuleNames.AddRange(new string[] { 
			"Core", 
			"CoreUObject", 
			"MoviePlayer", 
			"Engine", 
			"Slate", 
			"SlateCore",
			"RenderCore",
            "PreLoadScreen",
            "DeveloperSettings"
        });
	}
}