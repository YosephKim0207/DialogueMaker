#include "DialogueGraphEditorMode.h"

#include "DialogueGraphEditor.h"
#include "DialoguePrimaryTabFactory.h"

DialogueGraphEditorMode::DialogueGraphEditorMode(TSharedPtr<FDialogueGraphEditor> InApp) : FApplicationMode(TEXT("DialogueGraphMode"))
{
	App = InApp;
	Tabs.RegisterFactory(MakeShareable(new DialoguePrimaryTabFactory(InApp)));

	TabLayout = FTabManager::NewLayout(TEXT("DialogueGraphEditorMode_Layout_v1"))
	->AddArea
	(
		// FTabManager::NewPrimaryArea()->SetOrientation(Orient_Horizontal)
		// ->Split
		// (
		// 	FTabManager::NewStack()->AddTab(FName(TEXT("DialoguePrimaryTab")), ETabState::OpenedTab)
		// )
		FTabManager::NewPrimaryArea()->SetOrientation(Orient_Horizontal)
		->Split
		(
			FTabManager::NewSplitter()->SetOrientation(Orient_Horizontal)
			->Split
			(
				FTabManager::NewStack()
				->SetSizeCoefficient(0.75f)
				->AddTab(FName(TEXT("DialoguePrimaryTabFactory")), ETabState::OpenedTab)
			)
			->Split
			(
				FTabManager::NewStack()
				->SetSizeCoefficient(0.25f)
				->AddTab(FName(TEXT("DialoguePropertyTabFactory")), ETabState::OpenedTab)
				)
		)
	);
}

void DialogueGraphEditorMode::RegisterTabFactories(TSharedPtr<FTabManager> InTabManager)
{
	TSharedPtr<FDialogueGraphEditor> InApp = App.Pin();
	InApp->PushTabFactories(Tabs);
	FApplicationMode::RegisterTabFactories(InTabManager);
}

void DialogueGraphEditorMode::PreDeactivateMode()
{
	FApplicationMode::PreDeactivateMode();
}

void DialogueGraphEditorMode::PostActivateMode()
{
	FApplicationMode::PostActivateMode();
}
