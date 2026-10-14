#include "DialogueFlowEdGraphNode.h"
#include "DialogueLineNodeInfo.h"

// 노드 이름은 본문과 분리하여 표시한다.
FText UDialogueFlowEdGraphNode::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return FText::FromString(TEXT("Line"));
}

// 발화 노드를 구분하는 색상을 반환한다.
FLinearColor UDialogueFlowEdGraphNode::GetNodeTitleColor() const
{
	return FLinearColor(0.1f, 0.4f, 0.8f);
}

// 입력/출력은 동일한 대화 실행 연결 종류를 사용한다.
UEdGraphPin* UDialogueFlowEdGraphNode::CreateCustomPin(EEdGraphPinDirection Direction, FName Name)
{
	return CreatePin(Direction, TEXT("DialogueFlow"), Name);
}

// 여러 경로가 같은 발화로 합류할 수 있다.
UEdGraphPin* UDialogueFlowEdGraphNode::CreateDefaultInputPin()
{
	return CreateCustomPin(EGPD_Input, TEXT("In"));
}

// Line은 다음 실행 노드로 향하는 출력 하나만 가진다.
void UDialogueFlowEdGraphNode::CreateDefaultOutputPin()
{
	CreateCustomPin(EGPD_Output, TEXT("Next"));
}

// 초기 생성에만 사용하며 저장 재열기는 기록된 핀을 복원한다.
void UDialogueFlowEdGraphNode::AllocateDefaultPins()
{
	CreateDefaultInputPin();
	CreateDefaultOutputPin();
}

// 노드 소유의 편집 자료를 생성한다.
void UDialogueFlowEdGraphNode::InitNodeInfo(UObject* Outer)
{
	NodeInfo = NewObject<UDialogueLineNodeInfo>(Outer, NAME_None, RF_Transactional);
}

// 왕복 복제된 정보를 설치한다.
void UDialogueFlowEdGraphNode::SetDialogueNodeInfo(UDialogueNodeInfoBase* NewInfo)
{
	NodeInfo = NewInfo;
}
