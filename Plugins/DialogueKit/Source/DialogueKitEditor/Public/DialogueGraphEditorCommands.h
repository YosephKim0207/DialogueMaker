#pragma once

#include "CoreMinimal.h"


class DIALOGUEKITEDITOR_API FDialogueGraphEditorCommands : public TCommands<FDialogueGraphEditorCommands>
{
public:
	FDialogueGraphEditorCommands();
	
	virtual void RegisterCommands() override;

public:
	TSharedPtr<FUICommandInfo> ConvertToCSV;
	TSharedPtr<FUICommandInfo> ConvertCSVToDialogueLocalization;
};
