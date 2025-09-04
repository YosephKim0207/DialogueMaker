#include "DialogueEndNodeInfo.h"

void UDialogueEndNodeInfo::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName PropName = PropertyChangedEvent.Property->GetFName();
	if (PropertyChangedEvent.Property &&
	    PropName == GET_MEMBER_NAME_CHECKED(UDialogueEndNodeInfo, QuestBase) ||
	    PropName == GET_MEMBER_NAME_CHECKED(UDialogueEndNodeInfo, SelectedQuestStepTag))
	{
		if (UQuestBase* Quest = QuestBase.LoadSynchronous())
		{
			QuestRootTag = Quest->GetQuestRootTag();
			SelectedQuestStep = Quest->GetQuestStepByClearTag(SelectedQuestStepTag);
		}
		else
		{
			QuestRootTag = FGameplayTag();
			SelectedQuestStep = FQuestStep();
		}
	}
}

TArray<FName> UDialogueEndNodeInfo::GetQuestStepTagOptions() const
{
	TArray<FName> Options;

	if (const UQuestBase* Quest = QuestBase.LoadSynchronous())
	{
		const TArray<FQuestStep> QuestSteps = Quest->GetQuestSteps();
		for (const FQuestStep& Step : QuestSteps)
		{
			Options.Add(Step.ClearTag.GetTagName());
		}
	}
	return Options;
}
