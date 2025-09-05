#include "QuestSubsystem.h"

#include "GameplayTagsManager.h"
#include "PlayerProgressSubsystem.h"
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

void UQuestSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (LoadProgress() == false)
	{
		SaveData = Cast<UQuestProgressSaveData>(UGameplayStatics::CreateSaveGameObject(UQuestProgressSaveData::StaticClass()));
		SaveProgress();
	}
}

void UQuestSubsystem::StartQuest(const UQuestBase* Quest) const
{
	UE_LOG(QuestSubsystem, Display, TEXT("UQuestSubsystem::StartQuest : Enter"));

	AddNewOngoingQuest(Quest);
}

void UQuestSubsystem::AddNewOngoingQuest(const UQuestBase* Quest) const
{
	SaveData->TryAddNewQuest(Quest);
}

void UQuestSubsystem::SetQuestClear(const UQuestBase* Quest) const
{
	// 퀘스트 진행을 위해 저장된 GameplayTags 정리 및 Clear 태그 추가
	FGameplayTagContainer ChildrenTagContainer = UGameplayTagsManager::Get().RequestGameplayTagChildren(Quest->GetQuestRootTag());
	if (UPlayerProgressSubsystem* PlayerProgressSubsystem = UPlayerProgressSubsystem::Get(this))
	{
		for (FGameplayTag ChildTag : ChildrenTagContainer)
		{
			if (ChildTag.ToString().EndsWith(".Clear"))
			{
				PlayerProgressSubsystem->AddTag(ChildTag);
			}
			else
			{
				// 퀘스트 진행을 위해 저장된 clear를 제외한 ChildrenTags 모두 제거
				PlayerProgressSubsystem->RemoveTag(ChildTag);
			}
		}

		PlayerProgressSubsystem->SaveProgress();
	}
	
	SaveData->SetQuestClear(Quest);
	SaveProgress();
	
	UE_LOG(QuestSubsystem, Error, TEXT("UQuestSubsystem::SetClearedQuest : CachedQuestProgressSaveData Error"));
}

bool UQuestSubsystem::IsClearedQuest(const UQuestBase* Quest) const
{
	return SaveData->IsClearedQuest(Quest);
}

void UQuestSubsystem::AdvanceQuestStep(UQuestBase* Quest) const
{
	// TODO 해당 step 보상 등의 결과 처리
	

	// Quest 내부의 QuestStepIndex 증가,
	int32 CurrentQuestStepIndex = SaveData->GetOngoingQuestStepIndex(Quest);
	if (Quest->IsLastQuestStep(CurrentQuestStepIndex))
	{
		SetQuestClear(Quest);
	}
	else
	{
		SaveData->AdvanceQuestStepIndex(Quest);
	}
}

bool UQuestSubsystem::LoadProgress()
{
	SaveData = Cast<UQuestProgressSaveData>(UGameplayStatics::LoadGameFromSlot(QuestProgressSaveSlot, SaveIndex));
	if (SaveData == nullptr)
	{
		return false;
	}

	return true;
}

void UQuestSubsystem::SaveProgress() const
{
	UGameplayStatics::SaveGameToSlot(SaveData, QuestProgressSaveSlot, SaveIndex);
}
