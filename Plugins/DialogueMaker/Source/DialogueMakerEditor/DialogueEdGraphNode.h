// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DialogueEdGraphNodeBase.h"
#include "DialogueMaker/DialogueNodeType.h"
#include "EdGraph/EdGraphNode.h"
#include "DialogueEdGraphNode.generated.h"

/**
 * 
 */
UCLASS()
class DIALOGUEMAKEREDITOR_API UDialogueEdGraphNode : public UDialogueEdGraphNodeBase
{
	GENERATED_BODY()

public:
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FLinearColor GetNodeTitleColor() const override;
	virtual bool CanUserDeleteNode() const override;
	virtual void GetNodeContextMenuActions(class UToolMenu* Menu, class UGraphNodeContextMenuContext* Context) const override;
	virtual void AllocateDefaultPins() override;

	virtual UEdGraphPin* CreateDefaultInputPin() override;
	virtual void CreateDefaultOutputPin() override;
	
	virtual EDialogueType GetDialogueNodeType() const override;
	virtual void OnPropertiesChanged() override;
	
	virtual UEdGraphPin* CreateCustomPin(EEdGraphPinDirection Direction, FName Name) override;
	
	void SyncPinWithResponses();

	virtual void InitNodeInfo(UObject* Outer) override;
	virtual void SetDialogueNodeInfo(class UDialogueNodeInfoBase* NewDialogueNodeInfo) override;
	virtual UDialogueNodeInfoBase* GetNodeInfo() const override;
	class UDialogueNodeInfo* GetDialogueNodeInfo() const;

protected:
	UPROPERTY()
	class UDialogueNodeInfo* DialogueNodeInfo;
};
