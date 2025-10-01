#pragma once

#include "CoreMinimal.h"
#include "DialogueEdGraphNodeBase.h"
#include "DialogueMaker/DialogueNodeType.h"
#include "DialogueEdEndGraphNode.generated.h"

UCLASS()
class UDialogueEdEndGraphNode : public UDialogueEdGraphNodeBase
{
	GENERATED_BODY()
	
public:
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FLinearColor GetNodeTitleColor() const override;
	virtual bool CanUserDeleteNode() const override;
	virtual void GetNodeContextMenuActions(class UToolMenu* Menu, class UGraphNodeContextMenuContext* Context) const override;
	
	virtual UEdGraphPin* CreateCustomPin(EEdGraphPinDirection Direction, FName Name) override;
	virtual UEdGraphPin* CreateDefaultInputPin() override;

	virtual void InitNodeInfo(UObject* Outer) override;
	virtual void SetDialogueNodeInfo(class UDialogueNodeInfoBase* NewDialogueNodeInfo) override;
	virtual UDialogueNodeInfoBase* GetNodeInfo() const override;

	virtual EDialogueType GetDialogueNodeType() const override;
	virtual void OnPropertiesChanged() override;

protected:
	UPROPERTY()
	class UDialogueEndNodeInfo* NodeInfo;
};
