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
	void StartDialogue(AActor* NPC, class UDialogueGraph* DialogueGraph);
	void EndDialogue();
	class UDialogueNodeInfo* GetDialogueNodeInfo(FGuid DialogueNodeGuid);
	class UDialogueRuntimeNode* GetDialogueNode(FGuid DialogueNodeGuid);
	UDialogueNodeInfo* GetFirstDialogueInfo();
	bool HasChoicesInCurrentDialogue(UDialogueNodeInfo* DialogueNodeInfo) const;
	TArray<FText> GetSelectableChoicesText(UDialogueNodeInfo* DialogueNodeInfo) const;
	TArray<FGuid> GetSelectableChoicesLinkedGuid(class UDialogueRuntimeNode* DialogueRuntimeNode) const;
	void SelectChoice(FGuid ChoiceDialogueGuid);
	bool IsPossibleToShowTrueCondition(UDialogueRuntimeNode* BranchNode) ;
	void SaveDialogueProgress(FGuid CurrentDialogueGuid);
	FGuid GetCurrentDialogueGuid();
	class UDialogueRuntimeNode* GetNextNode();

	void ShowDialogue(UDialogueNodeInfo* Dialogue);

private:
	bool IsPossibleToLoadDialogueProgressData();
	void InitializeDialogueData();
	
	const FString DialogueProgressSaveSlot = TEXT("DialogueProgressSaveSlot");
	const int32 SaveIndex = 0;

	UPROPERTY()
	UDialogueGraph* CurrentDialogueGraph;
	UPROPERTY()
	TMap<FGuid, UDialogueRuntimeNode*> IdToNodeMap;
	UPROPERTY()
	FGuid CurrentOngoingNodeGuid;
	
	UPROPERTY()
	class UDialogueProgressSaveData* CachedDialogueProgressSaveData;
};
