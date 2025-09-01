#include "QuestSubsystem.h"

#include "QuestProgressSaveData.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY_STATIC(QuestSubsystem, Log, All);

void UQuestSubsystem::AddNewOngoingQuest(FName QuestID)
{
	if (IsPossibleToLoadQuestProgressData() == false)
	{
		CachedQuestProgressSaveData = Cast<UQuestProgressSaveData>(UGameplayStatics::CreateSaveGameObject(UQuestProgressSaveData::StaticClass()));
	}

	CachedQuestProgressSaveData->GetOngoingQuestIDs().Add(QuestID);
		
	UGameplayStatics::SaveGameToSlot(CachedQuestProgressSaveData, QuestProgressSaveSlot, SaveIndex);
}

void UQuestSubsystem::SetQuestCleared(FName QuestID)
{
	if (IsPossibleToLoadQuestProgressData())
	{
		CachedQuestProgressSaveData->GetOngoingQuestIDs().Remove(QuestID);
		CachedQuestProgressSaveData->GetCompleteQuestIDs().Add(QuestID);

		UGameplayStatics::SaveGameToSlot(CachedQuestProgressSaveData, QuestProgressSaveSlot, SaveIndex);
	}

	UE_LOG(QuestSubsystem, Error, TEXT("UQuestSubsystem::SetClearedQuest : CachedQuestProgressSaveData Error"));
}

bool UQuestSubsystem::IsClearedQuest(FName QuestID)
{
	if (IsPossibleToLoadQuestProgressData())
	{
		const TArray<FName> CompleteQuestIDs = CachedQuestProgressSaveData->GetCompleteQuestIDs();
		return CompleteQuestIDs.Contains(QuestID);
	}

	UE_LOG(QuestSubsystem, Warning, TEXT("UQuestSubsystem::GetIsClearedQuest : CachedQuestProgressSaveData Error"));
	return false;
}

bool UQuestSubsystem::IsPossibleToLoadQuestProgressData()
{
	if (CachedQuestProgressSaveData == nullptr)
	{
		CachedQuestProgressSaveData = Cast<UQuestProgressSaveData>(UGameplayStatics::LoadGameFromSlot(QuestProgressSaveSlot, SaveIndex));
		if (CachedQuestProgressSaveData == nullptr)
		{
			return false;
		}
	}

	return true;
}
