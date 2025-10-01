// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class DialogueMaker : ModuleRules
{
    public DialogueMaker(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new[]
            {
                "Core",
                "CoreUObject",
                "Engine",
                "InputCore",
                "EnhancedInput",
                "GameplayTags",
                "UMG",
                "DeveloperSettings"
            });

        PrivateDependencyModuleNames.AddRange(
            new[]
            {
                "Slate",
                "SlateCore"
            });
    }
}
