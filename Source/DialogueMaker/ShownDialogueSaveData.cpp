// Fill out your copyright notice in the Description page of Project Settings.

#include "ShownDialogueSaveData.h"

void UShownDialogueSaveData::SetShownDialogues(const FPrimaryAssetId DialogueGraphAssetID,
	const FGuidList& NodeGuids)
{
	ShownDialoguesMap.Add(DialogueGraphAssetID, NodeGuids);
}

void UShownDialogueSaveData::GetShownDialogues(const FPrimaryAssetId DialogueGraphAssetID,
	FGuidList& OutNodeGuids)
{
	if (ShownDialoguesMap.Contains(DialogueGraphAssetID))
	{
		OutNodeGuids = ShownDialoguesMap[DialogueGraphAssetID];
	}
}
