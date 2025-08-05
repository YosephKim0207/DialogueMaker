// Fill out your copyright notice in the Description page of Project Settings.


#include "DialogueGraph.h"

#include "UObject/ObjectSaveContext.h"

FString UDialogueGraph::GetSpeakerName() const
{
	// if (SpeakerName.IsEmpty() || FString::Find(TEXT(" ")))
	// {
	// 	UE_LOG(LogTemp, Error, TEXT("UDialogueGraph::GetSpeakerName : Please Enter Correct Speaker Name in Graph Editor Property!!!"));
	// 	return FString("SpeakerName");
	// }
	
	return SpeakerName;
}

void UDialogueGraph::SetPreSaveListener(std::function<void()> NewOnPreSaveListener)
{
	OnPreSaveListener = NewOnPreSaveListener;
}

void UDialogueGraph::PreSave(FObjectPreSaveContext SaveContext)
{
	if (OnPreSaveListener != nullptr)
	{
		OnPreSaveListener();
	}
}
