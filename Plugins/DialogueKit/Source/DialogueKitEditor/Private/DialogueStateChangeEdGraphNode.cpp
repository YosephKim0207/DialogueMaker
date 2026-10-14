#include "DialogueStateChangeEdGraphNode.h"
#include "DialogueStateChangeNodeInfo.h"

// 작가에게 상태 변경 노드의 역할을 표시한다.
FText UDialogueStateChangeEdGraphNode::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return NSLOCTEXT("DialogueKit", "StateChangeNode", "상태 변경");
}

// 실제 게임 제공자에게 전달할 효과 목록을 노드 내부에 보관한다.
void UDialogueStateChangeEdGraphNode::InitNodeInfo(UObject* Outer)
{
	NodeInfo = NewObject<UDialogueStateChangeNodeInfo>(Outer, NAME_None, RF_Transactional);
}
