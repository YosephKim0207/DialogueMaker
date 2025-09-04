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
	static UQuestSubsystem* Get(const UObject* WorldContextObject);
	
	void StartQuest(const UQuestBase* Quest);
	void SetQuestClear(const UQuestBase* Quest);
	bool IsClearedQuest(const UQuestBase* Quest);
	void AdvanceQuestStep(UQuestBase* Quest);
	
private:
	void AddNewOngoingQuest(const UQuestBase* Quest);
	bool IsPossibleToLoadQuestProgressData();

private:
	const FString QuestProgressSaveSlot = TEXT("QuestProgressSaveSlot");
	const int32 SaveIndex = 0;
	
	UPROPERTY()
	class UQuestProgressSaveData* CachedQuestProgressSaveData;
};
