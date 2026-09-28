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
			"MoviePlayer"
		});
	}
}
