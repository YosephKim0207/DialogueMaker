#pragma once

#include "CoreMinimal.h"
#include "DialogueMaker/DialogueNodeType.h"
#include "DialogueEdGraphNodeBase.generated.h"

UCLASS()
class UDialogueEdGraphNodeBase : public UEdGraphNode
{
	GENERATED_BODY()
	
public:
	virtual UEdGraphPin* CreateCustomPin(EEdGraphPinDirection Direction, FName Name) { /* 반드시 override 필요 */ return nullptr; };
	virtual UEdGraphPin* CreateDefaultInputPin() { return nullptr; };
	virtual void CreateDefaultOutputPin() { /* Default에서 특별한 작업 필요 없음 */ };

	virtual void InitNodeInfo(UObject* Outer) { /* Default에서 초기화 없음 */ };
	virtual void SetDialogueNodeInfo(class UDialogueNodeInfoBase* NewDialogueNodeInfo) { /* Default에서 Set 없음 */ };
	virtual UDialogueNodeInfoBase* GetNodeInfo() const { /* Default에서 info 없음 */ return nullptr; };

	virtual EDialogueType GetDialogueNodeType() const { return EDialogueType::Unknown; };
	// Custom Graph에서 Node 선택시 Properties에 정보 업데이트 용도
	virtual void OnPropertiesChanged() { /* Default에서 작업 없음 */ };
};
