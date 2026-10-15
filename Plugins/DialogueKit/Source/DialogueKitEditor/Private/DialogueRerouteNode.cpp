#include "DialogueRerouteNode.h"
#include "SGraphNodeKnot.h"

// 블루프린트와 같은 작은 경유점 위젯을 사용한다.
TSharedPtr<SGraphNode> UDialogueRerouteNode::CreateVisualWidget()
{
	return SNew(SGraphNodeKnot, this);
}

// 경유점의 입출력 위치를 Slate에 전달한다.
bool UDialogueRerouteNode::ShouldDrawNodeAsControlPointOnly(int32& InputIndex, int32& OutputIndex) const
{
	InputIndex = 0;
	OutputIndex = 1;
	return Pins.Num() == 2;
}

// 메뉴와 검색용 편집 이름이다.
FText UDialogueRerouteNode::GetNodeTitle(ENodeTitleType::Type Type) const
{
	return NSLOCTEXT("DialogueKit", "Reroute", "경유점");
}
