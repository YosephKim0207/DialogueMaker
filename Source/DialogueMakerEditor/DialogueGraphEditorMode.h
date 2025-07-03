#pragma once

#include "CoreMinimal.h"
#include "WorkflowOrientedApp/ApplicationMode.h"
#include "WorkflowOrientedApp/WorkflowTabManager.h"


class DialogueGraphEditorMode : public FApplicationMode
{
public:
	DialogueGraphEditorMode(TSharedPtr<class FDialogueGraphEditor> InApp);

	virtual void RegisterTabFactories(TSharedPtr<FTabManager> InTabManager) override;
	virtual void PreDeactivateMode() override;
	virtual void PostActivateMode() override;

	private:
	TWeakPtr<class FDialogueGraphEditor> App;
	FWorkflowAllowedTabSet Tabs;
};
