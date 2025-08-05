#include "QuestBase.h"

void UQuestBase::SyncQuestProgress()
{
	// TODO Save된 정보 있다면 Load, Save파일 없다면 초기화
	
}

bool UQuestBase::IsCleared() const
{
	return bIsCleared;
}

void UQuestBase::SetClear(bool bIsNewClearValue)
{
	bIsCleared = bIsNewClearValue;
}

void UQuestBase::InitQuestProgress()
{
	bIsCleared = false;
}
