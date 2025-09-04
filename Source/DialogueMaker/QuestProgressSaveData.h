// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "QuestProgressSaveData.generated.h"

/**
 * 
 */
UCLASS()
class DIALOGUEMAKER_API UQuestProgressSaveData : public USaveGame
{
	GENERATED_BODY()

public:
	void GetOngoingQuestIDs(TMap<FName, int32>& OutOngoingQuestIDStepIndexMap);
	void GetCompleteQuestIDs(TArray<FName>& OutClearedQuestIDs);
	int32 GetOngoingQuestStepIndex(const class UQuestBase* Quest) const;
	bool IsClearedQuest(const UQuestBase* Quest) const;

	void AdvanceQuestStepIndex(UQuestBase* Quest);

	bool TryAddNewQuest(const UQuestBase* Quest);
	void SetQuestClear(const UQuestBase* Quest);
	
private:
	UPROPERTY()
	TMap<FName, int32> OngoingQuestIDStepIndexMap;

	UPROPERTY()
	TArray<FName> ClearedQuestIDs;
}; 