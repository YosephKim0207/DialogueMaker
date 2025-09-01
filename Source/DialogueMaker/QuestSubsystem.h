// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "QuestBase.h"
#include "QuestSubsystem.generated.h"

/**
 * 
 */
UCLASS()
class DIALOGUEMAKER_API UQuestSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	void AddNewOngoingQuest(FName QuestID);
	void SetQuestCleared(FName QuestID);
	bool IsClearedQuest(FName QuestID);

private:
	bool IsPossibleToLoadQuestProgressData();

private:
	const FString QuestProgressSaveSlot = TEXT("QuestProgressSaveSlot");
	const int32 SaveIndex = 0;
	
	UPROPERTY()
	class UQuestProgressSaveData* CachedQuestProgressSaveData;
};
