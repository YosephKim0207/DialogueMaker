#pragma once

#include "CoreMinimal.h"
#include "DialogueFlowEdGraphNode.h"
#include "DialogueChoiceEdGraphNode.generated.h"

UCLASS()
class DIALOGUEKITEDITOR_API UDialogueChoiceEdGraphNode : public UDialogueFlowEdGraphNode
{
	GENERATED_BODY()
public:
	virtual FLinearColor GetNodeTitleColor() const override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual EDialogueType GetDialogueNodeType() const override { return EDialogueType::ChoiceNode; }
	virtual void InitNodeInfo(UObject* Outer) override;
	virtual void CreateDefaultOutputPin() override;
	virtual void OnPropertiesChanged() override;
};
