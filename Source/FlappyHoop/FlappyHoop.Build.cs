// Copyright Epic Games, Inc. All Rights Reserved.

using System.IO;
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
			"OnlineSubsystem",  // Google Play Games leaderboards (OnlineSubsystemGooglePlay, Android)
			"Json"              // cloud save format
		});

		if (Target.Platform == UnrealTargetPlatform.Android)
		{
			PrivateDependencyModuleNames.Add("Launch"); // JNI (FJavaWrapper) for cloud save
			// Google Play Games Saved Games glue added to GameActivity (cloud save).
			AdditionalPropertiesForReceipt.Add("AndroidPlugin", Path.Combine(ModuleDirectory, "FlappyHoop_UPL.xml"));
		}

		if (Target.Platform == UnrealTargetPlatform.IOS)
		{
			PublicFrameworks.Add("GameKit"); // Game Center leaderboards
		}
	}
}
