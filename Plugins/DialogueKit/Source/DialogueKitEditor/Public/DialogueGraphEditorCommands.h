#pragma once

#include "CoreMinimal.h"


class DIALOGUEKITEDITOR_API FDialogueGraphEditorCommands : public TCommands<FDialogueGraphEditorCommands>
{
public:
	FDialogueGraphEditorCommands();
	
	virtual void RegisterCommands() override;

public:
	TSharedPtr<FUICommandInfo> Search;
	TSharedPtr<FUICommandInfo> DuplicateNodes;
	TSharedPtr<FUICommandInfo> ConvertToCSV;
	TSharedPtr<FUICommandInfo> ConvertCSVToDialogueLocalization;
};
