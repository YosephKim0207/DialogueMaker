// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Enum/PlayerProgress.h"
#include "GameFramework/SaveGame.h"
#include "PlayerProgressSaveData.generated.h"

/**
 * 
 */
UCLASS()
class DIALOGUEMAKER_API UPlayerProgressSaveData : public USaveGame
{
	GENERATED_BODY()

public:
	const FGameplayTagContainer& GetAllTags() const;
	bool HasTag(const FGameplayTag& Tag) const;
	void AddTag(const FGameplayTag& NewGameplayTag);
	void AddTags(const FGameplayTagContainer& NewGameplayTagContainer);
	void RemoveTag(const FGameplayTag& RemoveTargetGameplayTag);
	void RemoveTags(const FGameplayTagContainer& RemoveTargetGameplayTagContainer);
	
	int32 GetPlayerLevel() const;
	void SetPlayerLevel(int32 NewPlayerLevel);

	EChapterID GetCurrentChapter() const;
	
private:
	UPROPERTY()
	FGameplayTagContainer PlayersTagContainer;
	
	UPROPERTY()
	int32 PlayerLevel = 0;

	UPROPERTY()
	EChapterID CurrentChapter = EChapterID::Chapter01;
};
