#include "QuestBase.h"
#include "QuestSubsystem.h"

DEFINE_LOG_CATEGORY_STATIC(QuestBase, Log, All);

FName UQuestBase::GetQuestID() const
{
	return QuestID;
}

FGameplayTag UQuestBase::GetQuestRootTag() const
{
	return QuestRootTag;
}

bool UQuestBase::IsLastQuestStep(int32 CheckIndex) const
{
	return QuestSteps.Num() - 1 == CheckIndex;
}

FPrimaryAssetId UQuestBase::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("Quest"), GetFName());
}
