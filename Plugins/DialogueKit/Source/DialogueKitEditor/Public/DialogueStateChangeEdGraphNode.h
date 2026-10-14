#pragma once

#include "CoreMinimal.h"
#include "DialogueFlowEdGraphNode.h"
#include "DialogueStateChangeEdGraphNode.generated.h"

UCLASS()
class DIALOGUEKITEDITOR_API UDialogueStateChangeEdGraphNode : public UDialogueFlowEdGraphNode
{
	GENERATED_BODY()
public:
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual EDialogueType GetDialogueNodeType() const override { return EDialogueType::StateChangeNode; }
	virtual void InitNodeInfo(UObject* Outer) override;
};
