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

// Asset Icon Image 설정
// const FSlateBrush* UAssetDefinition_DialogueGraph::GetThumbnailBrush(const FAssetData& InAssetData,
// 	const FName InClassName) const
// {
// 	
// 	TSharedPtr<FSlateStyleSet> Style = MakeShared<FSlateStyleSet>("DialogueAssetStyle");
//
// 	// 아이콘이 들어 있는 'AssetIcon' 폴더 Root 지정
// 	const FString IconRoot = FPaths::Combine(FPaths::ProjectContentDir(), TEXT("Resources/Texture/AssetIcon"));
// 	Style->SetContentRoot(IconRoot);
//
// 	FSlateImageBrush* ThumbnailBrush = new FSlateImageBrush(
// 		Style->RootToContentDir(TEXT("DialogueGraph_128"), TEXT(".png")), FVector2D(128.0f, 128.0f));
// 	FSlateImageBrush* IconBrush = new FSlateImageBrush(
// 		Style->RootToContentDir(TEXT("DialogueGraph_128"), TEXT(".png")), FVector2D(128.0f, 128.0f));
// 	const FName IconKey(TEXT("ClassThumbnail.DialogueGraph"));
// 	const FName BrushKey(TEXT("ClassIcon.DialogueGraph"));	
// 	Style->Set(IconKey,	ThumbnailBrush);	 
// 	Style->Set(BrushKey, IconBrush);	
//
// 	FSlateStyleRegistry::RegisterSlateStyle(*Style);   // 전역 등록
//
// 	return Style->GetBrush(IconKey);                     // 브러시 반환
// }
