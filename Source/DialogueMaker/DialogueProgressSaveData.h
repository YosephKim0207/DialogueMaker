// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DialogueGraph.h"
#include "GameFramework/SaveGame.h"
#include "DialogueProgressSaveData.generated.h"

/**
 * 
 */
UCLASS()
class DIALOGUEMAKER_API UDialogueProgressSaveData : public USaveGame
{
	GENERATED_BODY()

public:
	void AddDialogueProgress(UDialogueGraph* DialogueGraph, FGuid CurrentGuid);
	FGuid GetDialogueProgress(UDialogueGraph* DialogueGraph);
	
private:
	UPROPERTY(VisibleAnywhere)
	TMap<TSoftObjectPtr<UDialogueGraph>, FGuid> DialogueProgressMap;
};
