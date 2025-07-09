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
	
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FLinearColor GetNodeTitleColor() const override;
	virtual bool CanUserDeleteNode() const override;
	virtual void GetNodeContextMenuActions(class UToolMenu* Menu, class UGraphNodeContextMenuContext* Context) const override;
	virtual void AllocateDefaultPins() override;

	UEdGraphPin* CreateCustomPin(EEdGraphPinDirection Direction, FName Name);

	void SetDialogueNodeInfo(class UDialogueNodeInfo* NewDialogueNodeInfo) { DialogueNodeInfo = NewDialogueNodeInfo; };
	class UDialogueNodeInfo* GetDialogueNodeInfo() const
	{
		return DialogueNodeInfo;
	}
	
protected:
	UPROPERTY()
	class UDialogueNodeInfo* DialogueNodeInfo = nullptr;
};
