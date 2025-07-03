// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EdGraph/EdGraphSchema.h"
#include "DialogueEdGraphSchema.generated.h"

/**
 * 
 */
UCLASS()
class DIALOGUEMAKEREDITOR_API UDialogueEdGraphSchema : public UEdGraphSchema
{
	GENERATED_BODY()

	virtual const FPinConnectionResponse CanCreateConnection(const UEdGraphPin* A, const UEdGraphPin* B) const override;
	virtual void GetGraphContextActions(FGraphContextMenuBuilder& ContextMenuBuilder) const override;
};
