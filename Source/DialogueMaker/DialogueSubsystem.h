// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/Subsystem.h"
#include "DialogueSubsystem.generated.h"

/**
 * 
 */
UCLASS()
class DIALOGUEMAKER_API UDialogueSubsystem : public USubsystem
{
	GENERATED_BODY()

public:
	void StartDialogue(AActor* NPC, UDataTable* DialogueDataTable);
	void EndDialogue();
	struct FDialogueStructure GetCurrentDialogue();
	bool HasChoicesInCurrentDialogue();
	TArray<struct FDialogueChoice> GetSelectableChoices();
	void SelectChoice(FGuid ChoiceDialogueGuid);
	bool IsDialogueShowPossibleCondition() ;
	void SaveDialogueProgress(UDataTable* DialogueDataTable, FGuid CurrentDialogueGuid);
	FGuid GetCurrentDialogueGuid(UDataTable* DialogueDataTable);
	
};
