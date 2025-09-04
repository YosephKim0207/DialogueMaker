#include "QuestBase.h"
#include "QuestSubsystem.h"

DEFINE_LOG_CATEGORY_STATIC(QuestBase, Log, All);

void UQuestBase::SyncQuestProgress()
{
	UE_LOG(QuestBase, Display, TEXT("UQuestBase::SyncQuestProgress : Enter"));
}

FName UQuestBase::GetQuestID() const
{
	return QuestID;
}

FGameplayTag UQuestBase::GetQuestBaseTag() const
{
	return QuestBaseTag;
}

bool UQuestBase::IsLastQuestStep(int32 CheckIndex) const
{
	return QuestSteps.Num() - 1 == CheckIndex;
}