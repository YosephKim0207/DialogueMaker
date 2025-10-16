// Fill out your copyright notice in the Description page of Project Settings.

#include "ShownDialogueSaveData.h"

DEFINE_LOG_CATEGORY_STATIC(ShownDialogueSaveDataLog, Log, All);

void UShownDialogueSaveData::SetShownDialogues(const FPrimaryAssetId DialogueGraphAssetID,
	const FGuidList& NodeGuids)
{
	UE_LOG(ShownDialogueSaveDataLog, Display, TEXT("UShownDialogueSaveData::SetShownDialogues : Enter"));

	ShownDialoguesMap.Add(DialogueGraphAssetID, NodeGuids);
}

void UShownDialogueSaveData::GetShownDialogues(const FPrimaryAssetId DialogueGraphAssetID,
	FGuidList& OutNodeGuids)
{
	UE_LOG(ShownDialogueSaveDataLog, Display, TEXT("UShownDialogueSaveData::GetShownDialogues : Enter"));
	
	if (ShownDialoguesMap.Contains(DialogueGraphAssetID))
	{
		OutNodeGuids = ShownDialoguesMap[DialogueGraphAssetID];
	}
}
