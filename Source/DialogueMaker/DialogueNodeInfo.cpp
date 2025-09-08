#include "DialogueNodeInfo.h"

const FText& UDialogueNodeInfo::GetTitle() const
{
	return Title;
}

const FText& UDialogueNodeInfo::GetDialogueText() const
{
	return DialogueText;
}

const TArray<FDialogueChoice>& UDialogueNodeInfo::GetDialogueChoices() const
{
	return DialogueChoices;
}

void UDialogueNodeInfo::AddDialogueChoice(const FDialogueChoice& DialogueChoice)
{
	DialogueChoices.Add(DialogueChoice);
}

void UDialogueNodeInfo::RemoveDialogueChoiceAt(int32 Index)
{
	DialogueChoices.RemoveAt(Index);
}

bool UDialogueNodeInfo::IsDialogueAlreadyShown() const
{
	return bIsShown;
}

void UDialogueNodeInfo::SetShownCondition(const bool NewCondition)
{
	bIsShown = NewCondition;
}
