// Copyright Epic Games, Inc. All Rights Reserved.

#include "DialogueMakerEditor.h"

#include "EdGraphUtilities.h"
#include "SGraphPin.h"
#include "Interfaces/IPluginManager.h"
#include "Modules/ModuleManager.h"
#include "Styling/SlateStyleRegistry.h"

class SDialogueGraphPin : public SGraphPin
{
public:
	SLATE_BEGIN_ARGS(SDialogueGraphPin) {};
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UEdGraphPin* InGraphPinObj)
	{
		SGraphPin::Construct(SGraphPin::FArguments(), InGraphPinObj);
	};

	protected:
	virtual FSlateColor GetPinColor() const override
	{
		return FSlateColor(FLinearColor(0.2f, 1.0f, 0.2f));
	};
};

class SDialogueStartGraphPin : public SGraphPin
{
public:
	SLATE_BEGIN_ARGS(SDialogueStartGraphPin) {};
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UEdGraphPin* InGraphPinObj)
	{
		SGraphPin::Construct(SGraphPin::FArguments(), InGraphPinObj);
	};

protected:
	virtual FSlateColor GetPinColor() const override
	{
		return FSlateColor(FLinearColor(1.0f, 0.2f, 0.2f));
	};
};


class SDialogueEndGraphPin : public SGraphPin
{
public:
	SLATE_BEGIN_ARGS(SDialogueEndGraphPin) {};
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UEdGraphPin* InGraphPinObj)
	{
		SGraphPin::Construct(SGraphPin::FArguments(), InGraphPinObj);
	};

protected:
	virtual FSlateColor GetPinColor() const override
	{
		return FSlateColor(FLinearColor(0.2f, 0.2f, 1.0f));
	};
};

struct FDialoguePinFactory : FGraphPanelPinFactory
{
	virtual ~FDialoguePinFactory() {};
	virtual TSharedPtr<SGraphPin> CreatePin(UEdGraphPin* Pin) const override
	{
		if (FName(TEXT("DialoguePin")) == Pin->PinType.PinSubCategory)
		{
			return SNew(SDialogueGraphPin, Pin);
		}
		else if (FName(TEXT("StartPin")) == Pin->PinType.PinSubCategory)
		{
			return SNew(SDialogueStartGraphPin, Pin);
		}
		else if (FName(TEXT("EndPin")) == Pin->PinType.PinSubCategory)
		{
			return SNew(SDialogueEndGraphPin, Pin);
		}

		return nullptr;
	};
};

IMPLEMENT_MODULE(FDialogueMakerEditorModule, DialogueMakerEditor);

void FDialogueMakerEditorModule::StartupModule()
{
	StyleSet = MakeShareable(new FSlateStyleSet(TEXT("DialogueMakerEditorStyle")));
	// 아이콘이 들어 있는 'AssetIcon' 폴더 Root 지정
	const FString IconRoot = FPaths::Combine(FPaths::ProjectContentDir(), TEXT("Resources/Texture/AssetIcon"));
	StyleSet->SetContentRoot(IconRoot);
	
	FSlateImageBrush* ThumbnailBrush = new FSlateImageBrush(
		StyleSet->RootToContentDir(TEXT("DialogueGraph_128"), TEXT(".png")), FVector2D(128.0f, 128.0f));
	FSlateImageBrush* IconBrush = new FSlateImageBrush(
	StyleSet->RootToContentDir(TEXT("DialogueGraph_128"), TEXT(".png")), FVector2D(128.0f, 128.0f));
	FSlateImageBrush* NodeAddIcon = new FSlateImageBrush(
	StyleSet->RootToContentDir(TEXT("DialogueGraph_128"), TEXT(".png")), FVector2D(128.0f, 128.0f));
	FSlateImageBrush* NodeDeletePinIcon = new FSlateImageBrush(
	StyleSet->RootToContentDir(TEXT("DialogueGraph_128"), TEXT(".png")), FVector2D(128.0f, 128.0f));
	FSlateImageBrush* NodeDeleteNodeIcon = new FSlateImageBrush(
		StyleSet->RootToContentDir(TEXT("DialogueGraph_128"), TEXT(".png")), FVector2D(128.0f, 128.0f));
		
	StyleSet->Set(TEXT("ClassThumbnail.DialogueGraph"),	ThumbnailBrush);
	StyleSet->Set(TEXT("ClassIcon.DialogueGraph"), IconBrush);	
	StyleSet->Set(TEXT("DialogueMakerEditor.NodeAddIcon"), NodeAddIcon);
	StyleSet->Set(TEXT("DialogueMakerEditor.NodeDeletePinIcon"), NodeDeletePinIcon);
	StyleSet->Set(TEXT("DialogueMakerEditor.NodeDeleteNodeIcon"), NodeDeleteNodeIcon);
	
	FSlateStyleRegistry::RegisterSlateStyle(*StyleSet);   // 전역 등록

	PinFactory = MakeShareable(new FDialoguePinFactory);
	FEdGraphUtilities::RegisterVisualPinFactory(PinFactory);
}

void FDialogueMakerEditorModule::ShutdownModule()
{
	FSlateStyleRegistry::UnRegisterSlateStyle(*StyleSet);
	FEdGraphUtilities::UnregisterVisualPinFactory(PinFactory);
}
