#pragma once

#include "CoreMinimal.h"
#include "DialogueMaker/DialogueNodeType.h"
#include "DialogueEdGraphNodeBase.generated.h"

UCLASS()
class UDialogueEdGraphNodeBase : public UEdGraphNode
{
	GENERATED_BODY()
	
public:
	virtual UEdGraphPin* CreateCustomPin(EEdGraphPinDirection Direction, FName Name) { /* ë°˜ë“œ??override ?„ìš” */ return nullptr; };
	virtual UEdGraphPin* CreateDefaultInputPin() { return nullptr; };
	virtual void CreateDefaultOutputPin() { /* Default?ì„œ ?¹ë³„???‘ì—… ?„ìš” ?†ìŒ */ };

	virtual void InitNodeInfo(UObject* Outer) { /* Default?ì„œ ì´ˆê¸°???†ìŒ */ };
	virtual void SetDialogueNodeInfo(class UDialogueNodeInfoBase* NewDialogueNodeInfo) { /* Default?ì„œ Set ?†ìŒ */ };
	virtual UDialogueNodeInfoBase* GetNodeInfo() const { /* Default?ì„œ info ?†ìŒ */ return nullptr; };

	virtual EDialogueType GetDialogueNodeType() const { return EDialogueType::Unknown; };
	// Custom Graph?ì„œ Node ? íƒ??Properties???•ë³´ ?…ë°?´íŠ¸ ?©ë„
	virtual void OnPropertiesChanged() { /* Default?ì„œ ?‘ì—… ?†ìŒ */ };
};
