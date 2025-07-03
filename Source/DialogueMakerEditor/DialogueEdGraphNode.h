// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DialogueStructure.h"
#include "EdGraph/EdGraphNode.h"
#include "DialogueEdGraphNode.generated.h"

/**
 * 
 */
UCLASS()
class DIALOGUEMAKEREDITOR_API UDialogueEdGraphNode : public UEdGraphNode
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
	FDialogueStructure Dialogue;

	virtual void AllocateDefaultPins() override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
};
