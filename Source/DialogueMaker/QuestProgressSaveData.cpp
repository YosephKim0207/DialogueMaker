#include "QuestProgressSaveData.h"
#include "QuestBase.h"

DEFINE_LOG_CATEGORY_STATIC(QuestProgressSaveData, Log, All);

void UQuestProgressSaveData::GetOngoingQuestIDs(TMap<FName, int32>& OutOngoingQuestIDStepIndexMap)
{
	OutOngoingQuestIDStepIndexMap = OngoingQuestIDStepIndexMap;
}

void UQuestProgressSaveData::GetCompleteQuestIDs(TArray<FName>& OutClearedQuestIDs)
{
	OutClearedQuestIDs = ClearedQuestIDs;
}

int32 UQuestProgressSaveData::GetOngoingQuestStepIndex(const UQuestBase* Quest) const
{
	return OngoingQuestIDStepIndexMap[Quest->GetQuestID()];
}

bool UQuestProgressSaveData::IsClearedQuest(const UQuestBase* Quest) const
{
	return ClearedQuestIDs.Contains(Quest->GetQuestID());
}

void UQuestProgressSaveData::AdvanceQuestStepIndex(UQuestBase* Quest)
{
	if (OngoingQuestIDStepIndexMap.Contains(Quest->GetQuestID()) == false)
	{
		UE_LOG(QuestProgressSaveData, Warning, TEXT("UQuestProgressSaveData::AdvanceQuestStepIndex : Quest is not Exist"));

		if (TryAddNewQuest(Quest) == false)
		{
			UE_LOG(QuestProgressSaveData, Warning, TEXT("UQuestProgressSaveData::AdvanceQuestStepIndex : Quest Can't TryAdd"));

			return;
		}
	}
	
	int32 CurrentQuestStepIndex = OngoingQuestIDStepIndexMap[Quest->GetQuestID()];
	if (Quest->IsLastQuestStep(CurrentQuestStepIndex))
	{
		return;
	}

	OngoingQuestIDStepIndexMap[Quest->GetQuestID()] = CurrentQuestStepIndex + 1;
}

bool UQuestProgressSaveData::TryAddNewQuest(const UQuestBase* Quest)
{
	if (OngoingQuestIDStepIndexMap.Contains(Quest->GetQuestID()))
	{
		UE_LOG(QuestProgressSaveData, Warning, TEXT("UQuestProgressSaveData::TryAddNewQuest : Quest already Exist"));

		return false;
	}

	if (ClearedQuestIDs.Contains(Quest->GetQuestID()))
	{
		UE_LOG(QuestProgressSaveData, Warning, TEXT("UQuestProgressSaveData::TryAddNewQuest : Quest already Cleared"));

		return false;
	}

	OngoingQuestIDStepIndexMap.Add(Quest->GetQuestID(), 0);
	
	return true;
}

void UQuestProgressSaveData::SetQuestClear(const UQuestBase* Quest)
{
	OngoingQuestIDStepIndexMap.Remove(Quest->GetQuestID());
	ClearedQuestIDs.Add(Quest->GetQuestID());
}

