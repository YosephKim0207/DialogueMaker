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

// 표시 대상이 확정된 뒤 텍스처를 가져온다.
UTexture2D* UDialoguePortraitData::LoadPortraitTexture() const
{
	return Portrait.LoadSynchronous();
}
