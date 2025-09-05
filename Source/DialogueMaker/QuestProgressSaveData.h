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
	bool TryAddNewQuest(const class UQuestBase* Quest);
	bool IsOngoingQuest(const UQuestBase* Quest) const;
	int32 GetOngoingQuestStepIndex(const UQuestBase* Quest) const;
	void AdvanceQuestStepIndex(UQuestBase* Quest);
	
	bool IsClearedQuest(const UQuestBase* Quest) const;
	void SetQuestClear(const UQuestBase* Quest);

private:
	UPROPERTY()
	TMap<FName, int32> OngoingQuestIDStepIndexMap;

	UPROPERTY()
	TArray<FName> ClearedQuestIDs;
}; 