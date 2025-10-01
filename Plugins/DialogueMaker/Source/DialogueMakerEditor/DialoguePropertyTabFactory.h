#pragma once

#include "CoreMinimal.h"
#include "WorkflowOrientedApp/WorkflowTabFactory.h"

class DialoguePropertyTabFactory : public FWorkflowTabFactory
{
public:
	DialoguePropertyTabFactory(TSharedPtr<class FDialogueGraphEditor> InApp);
	
	virtual TSharedRef<SWidget> CreateTabBody(const FWorkflowTabSpawnInfo& Info) const override;
	virtual FText GetTabToolTipText(const FWorkflowTabSpawnInfo& Info) const override;

private:
	TWeakPtr<class FDialogueGraphEditor> App;
};
