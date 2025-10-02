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
	virtual void CreateDefaultNodesForGraph(UEdGraph& Graph) const override;
};


USTRUCT()
struct FNewNodeAction : public FEdGraphSchemaAction
{
	GENERATED_BODY()

public:
	FNewNodeAction() {};
	FNewNodeAction(UClass* NewClassTemplate, FText InNodeCategory, FText InMenuDesc, FText InMenuTooltip, const int32 InGrouping)
	: FEdGraphSchemaAction(InNodeCategory, InMenuDesc, InMenuTooltip, InGrouping), ClassTemplate(NewClassTemplate) {};

	virtual UEdGraphNode* PerformAction(UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode = true) override;

protected:
	UClass* ClassTemplate;
};
