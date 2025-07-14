// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class FlappyHoop : ModuleRules
{
	public FlappyHoop(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "AdmobGoodies", "MoviePlayer", "MonetizationGoodies" });

		PrivateDependencyModuleNames.AddRange(new string[] {  });

        PublicDefinitions.Add("DEBUG_IN_SHIPPING=1");

        // Uncomment if you are using Slate UI
        // PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

        // Uncomment if you are using online features
        // PrivateDependencyModuleNames.Add("OnlineSubsystem");

        // To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
    }
}
