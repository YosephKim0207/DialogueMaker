#include "DialogueNodeInfo.h"

bool UDialogueNodeInfo::IsDialogueAlreadyShown() const
{
	return bIsShown;
}

void UDialogueNodeInfo::SetShownCondition(const bool NewCondition)
{
	bIsShown = NewCondition;
}
