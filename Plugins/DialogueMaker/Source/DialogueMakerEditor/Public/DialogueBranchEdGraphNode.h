#pragma once

#include "CoreMinimal.h"
#include "DialogueEdGraphNodeBase.h"
#include "DialogueBranchEdGraphNode.generated.h"

UCLASS()
class DIALOGUEMAKEREDITOR_API UDialogueBranchEdGraphNode : public UDialogueEdGraphNodeBase
{
	GENERATED_BODY()
public:
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FText GetTooltipText() const override;
	virtual FLinearColor GetNodeTitleColor() const override;
	
	virtual void AllocateDefaultPins() override;
	virtual FSlateIcon GetIconAndTint(FLinearColor& OutColor) const override;
	
	// virtual class FNodeHandlingFunctor* CreateNodeHandler(class FKismetCompilerContext& CompilerContext) const override;
	// virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
	// virtual FText GetMenuCategory() const override;

	virtual UEdGraphPin* CreateCustomPin(EEdGraphPinDirection Direction, FName Name) override;
	virtual UEdGraphPin* CreateDefaultInputPin() override;
	virtual void CreateDefaultOutputPin() override;
	virtual void GetNodeContextMenuActions(class UToolMenu* Menu, class UGraphNodeContextMenuContext* Context) const override;
	
	virtual void InitNodeInfo(UObject* Outer) override;
	virtual void SetDialogueNodeInfo(UDialogueNodeInfoBase* NewDialogueNodeInfo) override;
	class UDialogueBranchNodeInfoBase* GetDialogueBranchNodeInfo() const;
	virtual UDialogueNodeInfoBase* GetNodeInfo() const override;
	virtual EDialogueType GetDialogueNodeType() const override;
	
private:
	class UDialogueBranchNodeInfoBase* DialogueBranchNodeInfo;
	
};
