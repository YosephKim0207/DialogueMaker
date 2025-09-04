#include "QuestSubsystem.h"

#include "GameplayTagsManager.h"
#include "QuestProgressSaveData.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY_STATIC(QuestSubsystem, Log, All);

UQuestSubsystem* UQuestSubsystem::Get(const UObject* WorldContextObject)
{
	if (WorldContextObject == nullptr)
	{
		return nullptr;
	}

	UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	if (World == nullptr)
	{
		UE_LOG(QuestSubsystem, Warning, TEXT("UQuestSubsystem::Get : World is nullptr"));

		return nullptr;
	}

	if (UGameInstance* GameInstance = World->GetGameInstance())
	{
		return GameInstance->GetSubsystem<UQuestSubsystem>();
	}
				
	UE_LOG(QuestSubsystem, Warning, TEXT("UQuestSubsystem::Get : GameInstance is nullptr"));

	return nullptr;
}

void UQuestSubsystem::StartQuest(const UQuestBase* Quest)
{
	UE_LOG(QuestSubsystem, Display, TEXT("UQuestSubsystem::StartQuest : Enter"));

	AddNewOngoingQuest(Quest);
}

void UQuestSubsystem::AddNewOngoingQuest(const UQuestBase* Quest)
{
	if (IsPossibleToLoadQuestProgressData() == false)
	{
		CachedQuestProgressSaveData = Cast<UQuestProgressSaveData>(UGameplayStatics::CreateSaveGameObject(UQuestProgressSaveData::StaticClass()));
	}

	if (CachedQuestProgressSaveData->TryAddNewQuest(Quest))
	{
		UGameplayStatics::SaveGameToSlot(CachedQuestProgressSaveData, QuestProgressSaveSlot, SaveIndex);
	}
}

void UQuestSubsystem::SetQuestClear(const UQuestBase* Quest)
{
	if (IsPossibleToLoadQuestProgressData())
	{
		// 퀘스트 진행을 위해 저장된 GameplayTags 정리 및 Clear 태그 추가
		FGameplayTagContainer ChildrenTagContainer = UGameplayTagsManager::Get().RequestGameplayTagChildren(Quest->GetQuestBaseTag());
		for (FGameplayTag ChildTag : ChildrenTagContainer)
		{
			if (ChildTag.ToString().EndsWith(".Clear"))
			{
				// TODO QuestBaseTag.Clear 추가
			}
			else
			{
				// TODO 퀘스트 진행을 위해 저장된 clear를 제외한 ChildrenTags 모두 제거
			}
		}
		
		CachedQuestProgressSaveData->SetQuestClear(Quest);
		UGameplayStatics::SaveGameToSlot(CachedQuestProgressSaveData, QuestProgressSaveSlot, SaveIndex);
	}

	UE_LOG(QuestSubsystem, Error, TEXT("UQuestSubsystem::SetClearedQuest : CachedQuestProgressSaveData Error"));
}

bool UQuestSubsystem::IsClearedQuest(const UQuestBase* Quest)
{
	if (IsPossibleToLoadQuestProgressData())
	{
		return CachedQuestProgressSaveData->IsClearedQuest(Quest);
	}

	UE_LOG(QuestSubsystem, Warning, TEXT("UQuestSubsystem::GetIsClearedQuest : CachedQuestProgressSaveData Error"));

	return false;
}

void UQuestSubsystem::AdvanceQuestStep(UQuestBase* Quest)
{
	// TODO 해당 step 보상 등의 결과 처리
	

	// Quest 내부의 QuestStepIndex 증가,
	int32 CurrentQuestStepIndex = CachedQuestProgressSaveData->GetOngoingQuestStepIndex(Quest);
	if (Quest->IsLastQuestStep(CurrentQuestStepIndex))
	{
		SetQuestClear(Quest);
	}
	else
	{
		CachedQuestProgressSaveData->AdvanceQuestStepIndex(Quest);
	}
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
