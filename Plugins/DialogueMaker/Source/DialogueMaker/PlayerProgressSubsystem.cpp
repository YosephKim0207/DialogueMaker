#include "PlayerProgressSubsystem.h"

#include "PlayerProgressSaveData.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY_STATIC(PlayerProgressSubsystemLog, Log, All);

UPlayerProgressSubsystem* UPlayerProgressSubsystem::Get(const UObject* WorldContextObject)
{
	if (WorldContextObject == nullptr)
	{
		return nullptr;
	}

	UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	if (World == nullptr)
	{
		UE_LOG(PlayerProgressSubsystemLog, Warning, TEXT("UPlayerProgressSubsystem::Get : World is nullptr"));

		return nullptr;
	}

	if (UGameInstance* GameInstance = World->GetGameInstance())
	{
		return GameInstance->GetSubsystem<UPlayerProgressSubsystem>();
	}
				
	UE_LOG(PlayerProgressSubsystemLog, Warning, TEXT("UPlayerProgressSubsystem::Get : GameInstance is nullptr"));

	return nullptr;
}

void UPlayerProgressSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (LoadProgress() == false)
	{
		SaveData = Cast<UPlayerProgressSaveData>(UGameplayStatics::CreateSaveGameObject(UPlayerProgressSaveData::StaticClass()));
		SaveProgress();
	}
}

const FGameplayTagContainer& UPlayerProgressSubsystem::GetAllTags() const
{
	return SaveData->GetAllTags();
}

bool UPlayerProgressSubsystem::HasTag(const FGameplayTag Tag) const
{
	return SaveData->HasTag(Tag);
}

void UPlayerProgressSubsystem::AddTag(const FGameplayTag& NewGameplayTag) const
{
	SaveData->AddTag(NewGameplayTag);
}

void UPlayerProgressSubsystem::AddTags(const FGameplayTagContainer& NewGameplayTagContainer) const
{
	SaveData->AddTags(NewGameplayTagContainer);
}

void UPlayerProgressSubsystem::RemoveTag(const FGameplayTag& RemoveTargetGameplayTag) const
{
	SaveData->RemoveTag(RemoveTargetGameplayTag);
}

void UPlayerProgressSubsystem::RemoveTags(const FGameplayTagContainer& RemoveTargetGameplayTagContainer) const
{
	SaveData->RemoveTags(RemoveTargetGameplayTagContainer);
}

int32 UPlayerProgressSubsystem::GetPlayerLevel() const
{
	return SaveData->GetPlayerLevel();
}

void UPlayerProgressSubsystem::SetPlayerLevel(int32 NewPlayerLevel) const
{
	SaveData->SetPlayerLevel(NewPlayerLevel);
}

EChapterID UPlayerProgressSubsystem::GetCurrentChapter() const
{
	return SaveData->GetCurrentChapter();
}

bool UPlayerProgressSubsystem::LoadProgress()
{
	SaveData = Cast<UPlayerProgressSaveData>(UGameplayStatics::LoadGameFromSlot(SaveSlot, SaveIndex));
	if (SaveData == nullptr)
	{
		return false;
	}

	return true;
}

void UPlayerProgressSubsystem::SaveProgress() const
{
	UGameplayStatics::SaveGameToSlot(SaveData, SaveSlot, SaveIndex);
}
