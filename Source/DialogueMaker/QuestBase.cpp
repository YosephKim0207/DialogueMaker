#include "QuestBase.h"

#include "QuestSubsystem.h"

DEFINE_LOG_CATEGORY_STATIC(QuestBase, Log, All);

void UQuestBase::SyncQuestProgress()
{
	UE_LOG(QuestBase, Display, TEXT("UQuestBase::SyncQuestProgress : Enter"));
	
	if (UWorld* World = GetWorld())
	{
		if (UQuestSubsystem* QuestSubsystem = World->GetGameInstance()->GetSubsystem<UQuestSubsystem>())
		{
			bIsCleared = QuestSubsystem->IsClearedQuest(QuestID);
			return;
		}
	}

	InitQuestProgress();
}

void UQuestBase::StartQuest()
{
	UE_LOG(QuestBase, Display, TEXT("UQuestBase::StartQuest : Enter"));

	if (UWorld* World = GetWorld())
	{
		if (UQuestSubsystem* QuestSubsystem = World->GetGameInstance()->GetSubsystem<UQuestSubsystem>())
		{
			QuestSubsystem->AddNewOngoingQuest(GetQuestID());
			return;
		}
	}

	UE_LOG(QuestBase, Error, TEXT("UQuestBase::StartQuest : Add Ongoing Quest Fail"));
}

bool UQuestBase::IsCleared() const
{
	if (UWorld* World = GetWorld())
	{
		if (UQuestSubsystem* QuestSubsystem = World->GetGameInstance()->GetSubsystem<UQuestSubsystem>())
		{
			return QuestSubsystem->IsClearedQuest(GetQuestID());
		}
	}

	UE_LOG(QuestBase, Error, TEXT("UQuestBase::IsCleared : Load Quest Info Fail"));
	return false;
}

void UQuestBase::SetClear()
{
	UE_LOG(QuestBase, Display, TEXT("UQuestBase::SetClear : Enter"));

	bIsCleared = true;
	if (UWorld* World = GetWorld())
	{
		if (UQuestSubsystem* QuestSubsystem = World->GetGameInstance()->GetSubsystem<UQuestSubsystem>())
		{
			QuestSubsystem->SetQuestCleared(GetQuestID());
			return;
		}
	}

	UE_LOG(QuestBase, Error, TEXT("UQuestBase::SetClear : Quest Set Clear Fail"));
}

FName UQuestBase::GetQuestID() const
{
	return QuestID;
}

void UQuestBase::InitQuestProgress()
{
	bIsCleared = false;
}
