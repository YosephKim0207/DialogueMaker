#pragma once

#include "CoreMinimal.h"
#include "DialogueEditorModule.generated.h"


class DIALOGUEMAKEREDITOR_API FDialogueGraphEditorCommands : public TCommands<FDialogueGraphEditorCommands>
{
public:
	FDialogueGraphEditorCommands();
	
	virtual void RegisterCommands() override;

public:
	TSharedPtr<FUICommandInfo> ConvertToDataTable;
};
