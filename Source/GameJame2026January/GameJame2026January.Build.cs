// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class GameJame2026January : ModuleRules
{
	public GameJame2026January(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"GameJame2026January",
			"GameJame2026January/Variant_Platforming",
			"GameJame2026January/Variant_Platforming/Animation",
			"GameJame2026January/Variant_Combat",
			"GameJame2026January/Variant_Combat/AI",
			"GameJame2026January/Variant_Combat/Animation",
			"GameJame2026January/Variant_Combat/Gameplay",
			"GameJame2026January/Variant_Combat/Interfaces",
			"GameJame2026January/Variant_Combat/UI",
			"GameJame2026January/Variant_SideScrolling",
			"GameJame2026January/Variant_SideScrolling/AI",
			"GameJame2026January/Variant_SideScrolling/Gameplay",
			"GameJame2026January/Variant_SideScrolling/Interfaces",
			"GameJame2026January/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
