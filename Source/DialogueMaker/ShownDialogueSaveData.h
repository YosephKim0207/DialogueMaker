// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <Programs/UnrealBuildAccelerator/Core/Public/UbaBase.h>

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "ShownDialogueSaveData.generated.h"

/**
 * 
 */

USTRUCT()
struct FGuidList
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TArray<FGuid> Guids;
	
	void Add(const FGuid& NewGuid)
	{
		Guids.Add(NewGuid);
	};
	
	void Reset()
	{
		Guids.Reset();
	};

	bool Contains(const FGuid& Guid) const
	{
		return Guids.Contains(Guid);
	}
};

UCLASS()
class DIALOGUEMAKER_API UShownDialogueSaveData : public USaveGame
{
	GENERATED_BODY()

public:
	void SetShownDialogues(const FPrimaryAssetId DialogueGraphAssetID, const FGuidList& NodeGuids);
	void GetShownDialogues(const FPrimaryAssetId DialogueGraphAssetID, FGuidList& OutNodeGuids);
	
private:
	UPROPERTY()
	TMap<FPrimaryAssetId, FGuidList> ShownDialoguesMap;
};
