#include "DialoguePrimaryTabFactory.h"
#include "DialogueGraphEditor.h"
#include "IDetailsView.h"
#include "PropertyEditorModule.h"
#include "Modules/ModuleManager.h"
#include "Editor/UnrealEd/Public/Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"


DialoguePrimaryTabFactory::DialoguePrimaryTabFactory(TSharedPtr<FDialogueGraphEditor> InApp) : FWorkflowTabFactory(FName("DialoguePrimaryTab"), InApp)
{
	App = InApp;
	TabLabel = FText::FromString("DialoguePrimaryTab");
	ViewMenuDescription = FText::FromString("Display primary view for Dialogue");
	ViewMenuTooltip = FText::FromString("Show the primary view");
}

// 생성된 Dialogue Editor tab에서 보여줄 본문
TSharedRef<SWidget> DialoguePrimaryTabFactory::CreateTabBody(const FWorkflowTabSpawnInfo& Info) const
{
	TSharedPtr<FDialogueGraphEditor> InApp = App.Pin();
	
	return SNew(SVerticalBox) + SVerticalBox::Slot().FillHeight(1.0f).HAlign(HAlign_Fill)
		[
			SNew(SGraphEditor).IsEditable(true).GraphToEdit(InApp->GetWorkingGraph())
		];
}

FText DialoguePrimaryTabFactory::GetTabToolTipText(const FWorkflowTabSpawnInfo& Info) const
{
	return FText::FromString(TEXT("A Dialogue primary tab for Make dialogue graph"));
}
