#include "QuestBase.h"
#include "QuestSubsystem.h"

DEFINE_LOG_CATEGORY_STATIC(QuestBase, Log, All);

FName UQuestBase::GetQuestID() const
{
	return FName(*GetPrimaryAssetId().ToString());
}

FGameplayTag UQuestBase::GetQuestRootTag() const
{
	return QuestRootTag;
}

bool UQuestBase::IsLastQuestStep(int32 CheckIndex) const
{
	return QuestSteps.Num() - 1 == CheckIndex;
}

const TArray<FQuestStep> UQuestBase::GetQuestSteps() const
{
	return QuestSteps;
}

FPrimaryAssetId UQuestBase::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("Quest"), GetFName());
}

const FQuestStep UQuestBase::GetQuestStepByClearTag(const FName& SearchTargetTagName)
{
	FGameplayTag SearchTargetTag = FGameplayTag::RequestGameplayTag(SearchTargetTagName);
	for (FQuestStep QuestStep : QuestSteps)
	{
		if (QuestStep.ClearTag.MatchesTagExact(SearchTargetTag))
		{
			return  QuestStep;
		}
	}

	return FQuestStep();
}
