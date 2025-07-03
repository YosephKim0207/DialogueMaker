#include "DialoguePropertyTabFactory.h"

#include "DialoguePrimaryTabFactory.h"
#include "DialogueGraphEditor.h"
#include "IDetailsView.h"
#include "PropertyEditorModule.h"
#include "Modules/ModuleManager.h"


DialoguePropertyTabFactory::DialoguePropertyTabFactory(TSharedPtr<FDialogueGraphEditor> InApp) : FWorkflowTabFactory(FName("DialoguePropertyTab"), InApp)
{
	App = InApp;
	TabLabel = FText::FromString("DialoguePropertyTab");
	ViewMenuDescription = FText::FromString("Display property view for Dialogue");
	ViewMenuTooltip = FText::FromString("Show the property view");
}

// 생성된 Dialogue Editor의 기본 tab에서 보여줄 본문
TSharedRef<SWidget> DialoguePropertyTabFactory::CreateTabBody(const FWorkflowTabSpawnInfo& Info) const
{
	TSharedPtr<FDialogueGraphEditor> InApp = App.Pin();
	FPropertyEditorModule& PropertyEditorModuleModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor"));

	return SNew(SVerticalBox) + SVerticalBox::Slot().FillHeight(1.0f).HAlign(HAlign_Fill)
		[
			SNew(STextBlock).Text(FText::FromString(InApp->GetWorkingAsset()->Title))
		];
}

FText DialoguePropertyTabFactory::GetTabToolTipText(const FWorkflowTabSpawnInfo& Info) const
{
	return FText::FromString(TEXT("A Dialogue property tab for Current dialogue graph"));
}
