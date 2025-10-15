#include "DialogueGraphEditorCommands.h"

#define LOCTEXT_NAMESPACE "DialogueGraphEditorCommands"

FDialogueGraphEditorCommands::FDialogueGraphEditorCommands()
	:TCommands<FDialogueGraphEditorCommands>(
		"DialogueGraphEditor"
		, NSLOCTEXT("Context", "DialogueGraphEditor", "Dialogue Graph Editor")
		, NAME_None
		, FAppStyle::GetAppStyleSetName()
		)
{
}

void FDialogueGraphEditorCommands::RegisterCommands()
{
	UI_COMMAND(ConvertToDataTable, "Convert To DataTable", "Convert and Create DataTable from Dialogue Graph Editor", EUserInterfaceActionType::Button, FInputChord());
}

#undef LOCTEXT_NAMESPACE