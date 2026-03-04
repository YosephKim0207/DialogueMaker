// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class DialogueKitEditor : ModuleRules
{
	public DialogueKitEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		// IWYU 도구가 실제로 코드를 수정/적용하도록 허용하는 수준 지정
		IWYUSupport = IWYUSupport.Full; // 엔진 버전에 따라 사용
		bUseUnity = false;
		
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine" });

                PrivateDependencyModuleNames.AddRange(new string[]
                {
                        "DialogueKit",
                        "UnrealEd",
                        "AssetTools",
                        "Slate",
                        "SlateCore",
                        "GraphEditor",
                        "BlueprintGraph",
                        "PropertyEditor",
                        "EditorStyle",
                        "AssetRegistry",
                        "GameplayTags",
                        "AssetDefinition",
                        "Projects",
                        "ToolMenus",
                        "DesktopPlatform"
                });
		
		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
