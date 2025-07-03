#include "DialoguePrimaryTabFactory.h"
#include "DialogueGraphEditor.h"
#include "IDetailsView.h"
#include "PropertyEditorModule.h"
#include "Modules/ModuleManager.h"


DialoguePrimaryTabFactory::DialoguePrimaryTabFactory(TSharedPtr<FDialogueGraphEditor> InApp) : FWorkflowTabFactory(FName("DialoguePrimaryTab"), InApp)
{
	App = InApp;
	TabLabel = FText::FromString("DialoguePrimaryTab");
	ViewMenuDescription = FText::FromString("Display primary view for Dialogue");
	ViewMenuTooltip = FText::FromString("Show the primary view");
}

// 생성된 Dialogue Editor의 기본 tab에서 보여줄 본문
TSharedRef<SWidget> DialoguePrimaryTabFactory::CreateTabBody(const FWorkflowTabSpawnInfo& Info) const
{
	TSharedPtr<FDialogueGraphEditor> InApp = App.Pin();
	FPropertyEditorModule& PropertyEditorModuleModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor"));

	FDetailsViewArgs DetailsViewArgs;
	DetailsViewArgs.bAllowSearch = false;
	DetailsViewArgs.bHideSelectionTip = true;
	DetailsViewArgs.bLockable = false;
	DetailsViewArgs.bSearchInitialKeyFocus = true;
	DetailsViewArgs.bUpdatesFromSelection = false;
	DetailsViewArgs.NotifyHook = nullptr;
	DetailsViewArgs.bShowOptions = true;
	DetailsViewArgs.bShowModifiedPropertiesOption = false;
	DetailsViewArgs.bShowScrollBar = false;

	TSharedPtr<IDetailsView> DetailsView = PropertyEditorModuleModule.CreateDetailView(DetailsViewArgs);
	DetailsView->SetObject(InApp->GetWorkingAsset());
	
	return SNew(SVerticalBox) + SVerticalBox::Slot().FillHeight(1.0f).HAlign(HAlign_Fill)[DetailsView.ToSharedRef()];
}

FText DialoguePrimaryTabFactory::GetTabToolTipText(const FWorkflowTabSpawnInfo& Info) const
{
	return FText::FromString(TEXT("A Dialogue primary tab for Make dialogue graph"));
}
