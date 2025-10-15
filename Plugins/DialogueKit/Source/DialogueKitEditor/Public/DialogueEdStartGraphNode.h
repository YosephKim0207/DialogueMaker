#pragma once

#include "CoreMinimal.h"
#include "DialogueEdGraphNodeBase.h"
#include "DialogueEdStartGraphNode.generated.h"

UCLASS()
class UDialogueEdStartGraphNode : public UDialogueEdGraphNodeBase
{
	GENERATED_BODY()
	
public:
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FLinearColor GetNodeTitleColor() const override;
	virtual bool CanUserDeleteNode() const override;

	virtual UEdGraphPin* CreateCustomPin(EEdGraphPinDirection Direction, FName Name) override;
	virtual EDialogueType GetDialogueNodeType() const override;
};
