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
	TArray<FName> GetOngoingQuestIDs();
	TArray<FName> GetCompleteQuestIDs();
	
private:
	UPROPERTY()
	TArray<FName> OngoingQuestIDs;

	UPROPERTY()
	TArray<FName> CompleteQuestIDs;
};
