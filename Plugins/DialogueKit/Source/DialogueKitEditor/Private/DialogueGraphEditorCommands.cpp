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
	UI_COMMAND(Search, "검색", "현재 Block에서 화자·대사 또는 선택지를 검색합니다", EUserInterfaceActionType::Button, FInputChord(EModifierKey::Control, EKeys::F));
	UI_COMMAND(DuplicateNodes, "복제", "선택한 노드와 내부 연결만 복제합니다", EUserInterfaceActionType::Button, FInputChord(EModifierKey::Control, EKeys::D));
	UI_COMMAND(ConvertToCSV, "Convert To CSV", "Export Dialogue Graph data to CSV", EUserInterfaceActionType::Button, FInputChord());
	UI_COMMAND(ConvertCSVToDialogueLocalization, "CSV To DialogueLocalization", "Convert localization CSV to DialogueLocalization DataAsset", EUserInterfaceActionType::Button, FInputChord());
}

#undef LOCTEXT_NAMESPACE
