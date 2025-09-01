#include "QuestProgressSaveData.h"

TArray<FName> UQuestProgressSaveData::GetOngoingQuestIDs()
{
	return OngoingQuestIDs;
}

TArray<FName> UQuestProgressSaveData::GetCompleteQuestIDs()
{
	return CompleteQuestIDs;
}
