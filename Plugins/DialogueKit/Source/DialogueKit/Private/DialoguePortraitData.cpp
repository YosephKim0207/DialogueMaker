// Fill out your copyright notice in the Description page of Project Settings.


#include "DialoguePortraitData.h"

UTexture2D* UDialoguePortraitData::GetPortraitTexture() const
{
	return Portrait.Get();
}

FPrimaryAssetId UDialoguePortraitData::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("DialoguePortraitData"), GetFName()); 
}
