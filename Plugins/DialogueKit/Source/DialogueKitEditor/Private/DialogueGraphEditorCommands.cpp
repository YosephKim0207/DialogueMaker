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
	UI_COMMAND(ConvertToCSV, "Convert To CSV", "Export Dialogue Graph data to CSV", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(ChangeDialogueText, "Change Dialogue Text", "Apply DialogueText and ResponseText from CSV", EUserInterfaceActionType::Button, FInputChord());
}

#undef LOCTEXT_NAMESPACE
