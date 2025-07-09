// Fill out your copyright notice in the Description page of Project Settings.


#include "DialogueGraph.h"

#include "UObject/ObjectSaveContext.h"

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
