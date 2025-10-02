// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Enum/PlayerProgress.h"
#include "PlayerProgressSubsystem.generated.h"

/**
 * 
 */
UCLASS()
class DIALOGUEMAKER_API UPlayerProgressSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	static UPlayerProgressSubsystem* Get(const UObject* WorldContextObject);

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	
	const FGameplayTagContainer& GetAllTags() const;
	bool HasTag(const FGameplayTag Tag) const;
	void AddTag(const FGameplayTag& NewGameplayTag) const;
	void AddTags(const FGameplayTagContainer& NewGameplayTagContainer) const;
	void RemoveTag(const FGameplayTag& RemoveTargetGameplayTag) const;
	void RemoveTags(const FGameplayTagContainer& RemoveTargetGameplayTagContainer) const;
	
	int32 GetPlayerLevel() const;
	void SetPlayerLevel(int32 NewPlayerLevel) const;

	EChapterID GetCurrentChapter() const;
	
	void SaveProgress() const;
private:
	bool LoadProgress();

	const FString SaveSlot = TEXT("PlayerProgressSaveSlot");
	const int32 SaveIndex = 0;
	
	UPROPERTY()
	class UPlayerProgressSaveData* SaveData;
};