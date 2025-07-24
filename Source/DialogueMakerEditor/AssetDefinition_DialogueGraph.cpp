// Fill out your copyright notice in the Description page of Project Settings.

#include "AssetDefinition_DialogueGraph.h"

#include "DialogueGraphEditor.h"
#include "DialogueMaker/DialogueGraph.h"
#include "Interfaces/IPluginManager.h"
#include "Styling/SlateStyleRegistry.h"

FText UAssetDefinition_DialogueGraph::GetAssetDisplayName() const
{
	return NSLOCTEXT("AssetDefinition", "DialogueGraph", "Dialogue Graph");
}

FLinearColor UAssetDefinition_DialogueGraph::GetAssetColor() const
{
	return FLinearColor::Red;
}

TSoftClassPtr<UObject> UAssetDefinition_DialogueGraph::GetAssetClass() const
{
    return UDialogueGraph::StaticClass();
}

TConstArrayView<FAssetCategoryPath> UAssetDefinition_DialogueGraph::GetAssetCategories() const
{
	static const auto Categories = { FAssetCategoryPath(NSLOCTEXT("AssetDefinition", "DialogueCategory", "Dialogue")) };
	return Categories;
}

EAssetCommandResult UAssetDefinition_DialogueGraph::OpenAssets(const FAssetOpenArgs& OpenArgs) const
{
	// EToolkitMode::Type Mode = OpenArgs.ToolkitHost.IsValid() ? EToolkitMode::Standalone : EToolkitMode::WorldCentric;

	for (const FAssetData& AssetData : OpenArgs.Assets)
	{
		UObject* LoadedAsset = AssetData.GetAsset();
		if (LoadedAsset)
		{
			if (UDialogueGraph* Graph = Cast<UDialogueGraph>(LoadedAsset))
			{
				// 에디터 툴킷을 생성하고 초기화합니다.
				// MakeShared<FDialogueGraphEditor>()->InitEditor(Mode, OpenArgs.ToolkitHost, Graph);
				MakeShared<FDialogueGraphEditor>()->InitEditor(OpenArgs.GetToolkitMode(), OpenArgs.ToolkitHost, Graph);
			}
		}
	}

	return EAssetCommandResult::Handled;
}
