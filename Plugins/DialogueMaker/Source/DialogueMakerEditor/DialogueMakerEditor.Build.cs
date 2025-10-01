// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class DialogueMakerEditor : ModuleRules
{
    public DialogueMakerEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new[]
            {
                "Core",
                "CoreUObject",
                "Engine",
                "UnrealEd"
            });

        PrivateDependencyModuleNames.AddRange(
            new[]
            {
                "DialogueMaker",
                "AssetDefinition",
                "AssetTools",
                "BlueprintGraph",
                "EditorStyle",
                "GraphEditor",
                "Kismet",
                "Projects",
                "PropertyEditor",
                "Slate",
                "SlateCore",
                "ToolMenus"
            });
    }
}
