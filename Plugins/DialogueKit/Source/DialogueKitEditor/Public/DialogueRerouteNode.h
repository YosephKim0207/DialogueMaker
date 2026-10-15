#pragma once
#include "CoreMinimal.h"
#include "DialogueFlowEdGraphNode.h"
#include "DialogueRerouteNode.generated.h"

// 실행 노드가 아닌 연결선 정리용 경유점이다.
UCLASS()
class UDialogueRerouteNode : public UDialogueFlowEdGraphNode
{
	GENERATED_BODY()
public:
	virtual EDialogueType GetDialogueNodeType() const override { return EDialogueType::Unknown; }
	virtual void InitNodeInfo(UObject* Outer) override {}
	virtual FText GetNodeTitle(ENodeTitleType::Type Type) const override;
	virtual bool ShouldDrawNodeAsControlPointOnly(int32& InputIndex, int32& OutputIndex) const override;
	virtual TSharedPtr<SGraphNode> CreateVisualWidget() override;
};
