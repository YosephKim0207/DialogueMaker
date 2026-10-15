#pragma once

#include "CoreMinimal.h"
#include "DialogueEdGraphNodeBase.h"
#include "DialogueNodeInfoBase.h"
#include "DialogueFlowEdGraphNode.generated.h"

// 신규 실행 노드의 기본 편집 표현이다. 기존 프로토타입 노드 클래스는 유지한다.
UCLASS()
class DIALOGUEKITEDITOR_API UDialogueFlowEdGraphNode : public UDialogueEdGraphNodeBase
{
	GENERATED_BODY()
public:
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FLinearColor GetNodeTitleColor() const override;
	virtual void GetNodeContextMenuActions(UToolMenu* Menu, UGraphNodeContextMenuContext* Context) const override;
	// 에디터 삭제 명령용이다. 확인 UI는 호출자가 처리하며 링크/Undo 기록을 함께 변경한다.
	bool DeleteFromGraph();
	bool CanDeleteFromGraph() const;
	virtual UEdGraphPin* CreateCustomPin(EEdGraphPinDirection Direction, FName Name) override;
	virtual UEdGraphPin* CreateDefaultInputPin() override;
	virtual void CreateDefaultOutputPin() override;
	virtual void AllocateDefaultPins() override;
	virtual void InitNodeInfo(UObject* Outer) override;
	virtual void SetDialogueNodeInfo(UDialogueNodeInfoBase* NewInfo) override;
	virtual UDialogueNodeInfoBase* GetNodeInfo() const override { return NodeInfo; }
	virtual EDialogueType GetDialogueNodeType() const override { return EDialogueType::LineNode; }

protected:
	UPROPERTY()
	TObjectPtr<UDialogueNodeInfoBase> NodeInfo;
};
