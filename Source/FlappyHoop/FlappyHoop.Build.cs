// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class FlappyHoop : ModuleRules
{
	public FlappyHoop(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"UMG",
			"AdmobGoodies",
			"MonetizationGoodies"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Slate",
			"SlateCore",
			"MoviePlayer",
			"OnlineSubsystem"   // Google Play Games leaderboards (OnlineSubsystemGooglePlay, Android)
		});

		if (Target.Platform == UnrealTargetPlatform.IOS)
		{
			PublicFrameworks.Add("GameKit"); // Game Center leaderboards
		}
	}
}
