// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <functional>

#include "CoreMinimal.h"
#include "DialogueRuntimeGraph.h"
#include "DialogueGraph.generated.h"

/**
 * 
 */
UCLASS(BlueprintType)
class DIALOGUEMAKER_API UDialogueGraph : public UObject
{
	GENERATED_BODY()

public:
	FString GetSpeakerName() const;
	void SetPreSaveListener(std::function<void()> NewOnPreSaveListener);

	virtual void PreSave(FObjectPreSaveContext SaveContext) override;


public:
	std::function<void()> OnPreSaveListener;

public:	// Properties
	UPROPERTY(EditAnywhere)
	FString SpeakerName = FString("Enter Dialogue Name Here");
	UPROPERTY(EditAnywhere)
	TObjectPtr<UDialogueRuntimeGraph> Graph;
};

