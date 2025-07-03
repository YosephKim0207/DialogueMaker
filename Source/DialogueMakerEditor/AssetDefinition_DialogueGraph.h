// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AssetDefinitionDefault.h"
#include "AssetDefinition_DialogueGraph.generated.h"

/**
 * 
 */
UCLASS()
class DIALOGUEMAKEREDITOR_API UAssetDefinition_DialogueGraph : public UAssetDefinitionDefault
{
	GENERATED_BODY()
	
	virtual FText GetAssetDisplayName() const override;
	virtual FLinearColor GetAssetColor() const override;
	virtual TSoftClassPtr<UObject> GetAssetClass() const override;
	virtual TConstArrayView<FAssetCategoryPath> GetAssetCategories() const override;
	virtual EAssetCommandResult OpenAssets(const FAssetOpenArgs& OpenArgs) const override;
	// virtual const FSlateBrush* GetThumbnailBrush(const FAssetData& InAssetData, const FName InClassName) const override;
};