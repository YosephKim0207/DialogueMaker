// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DialogueGraph.generated.h"

/**
 * 
 */
UCLASS(BlueprintType)
class DIALOGUEMAKER_API UDialogueGraph : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
	FString Title = FString("Title");
	UPROPERTY(EditAnywhere)
	TObjectPtr<UEdGraph> EdGraph;
	UPROPERTY(EditAnywhere)
	TObjectPtr<UDataTable> ConvertedDataTable;
};
