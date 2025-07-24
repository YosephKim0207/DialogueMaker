#include "DialogueGraphEditorMode.h"

#include "DialogueGraphEditor.h"
#include "DialoguePrimaryTabFactory.h"
#include "DialoguePropertyTabFactory.h"

DialogueGraphEditorMode::DialogueGraphEditorMode(TSharedPtr<FDialogueGraphEditor> InApp) : FApplicationMode(TEXT("DialogueGraphMode"))
{
	App = InApp;
	Tabs.RegisterFactory(MakeShareable(new DialoguePrimaryTabFactory(InApp)));
	Tabs.RegisterFactory(MakeShareable(new DialoguePropertyTabFactory(InApp)));

	TabLayout = FTabManager::NewLayout(TEXT("DialogueGraphEditorMode_Layout_v1"))
	->AddArea
	(
		FTabManager::NewPrimaryArea()->SetOrientation(Orient_Horizontal)
		->Split
		(
			FTabManager::NewSplitter()->SetOrientation(Orient_Horizontal)
			->Split
			(
				FTabManager::NewStack()
				->SetSizeCoefficient(0.75f)
				->AddTab(FName(TEXT("DialoguePrimaryTab")), ETabState::OpenedTab)
			)
			->Split
			(
				FTabManager::NewStack()
				->SetSizeCoefficient(0.25f)
				->AddTab(FName(TEXT("DialoguePropertyTab")), ETabState::OpenedTab)
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
