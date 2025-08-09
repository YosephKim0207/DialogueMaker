// Fill out your copyright notice in the Description page of Project Settings.


#include "DialogueProgressSaveData.h"

void UDialogueProgressSaveData::AddDialogueProgress(UDialogueGraph* DialogueGraph, FGuid CurrentGuid)
{
	TSoftObjectPtr<UDialogueGraph> DialogueGraphSoftObjPtr(DialogueGraph);
	if (DialogueProgressMap.Contains(DialogueGraphSoftObjPtr))
	{
		DialogueProgressMap[DialogueGraphSoftObjPtr] = CurrentGuid;
	}
	else
	{
		DialogueProgressMap.Add(DialogueGraphSoftObjPtr, CurrentGuid);	
	}
}

FGuid UDialogueProgressSaveData::GetDialogueProgress(UDialogueGraph* DialogueGraph)
{
	TSoftObjectPtr<UDialogueGraph> DialogueGraphSoftObjPtr(DialogueGraph);
	if (DialogueProgressMap.Contains(DialogueGraphSoftObjPtr))
	{
		return DialogueProgressMap[DialogueGraphSoftObjPtr];
	}

	return FGuid();
}
