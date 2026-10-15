#include "DialogueFlowEdGraphNode.h"
#include "DialogueLineNodeInfo.h"
#include "EdGraph/EdGraph.h"
#include "ToolMenus.h"
#include "ScopedTransaction.h"
#include "Misc/MessageDialog.h"

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

// 메뉴를 연 뒤 그래프 상태가 바뀌어도 삭제 가능 여부를 다시 확인한다.
bool UDialogueFlowEdGraphNode::CanDeleteFromGraph() const
{
	const UEdGraph* Graph = GetGraph();
	return Graph && Graph->bEditable && Graph->Nodes.Contains(this) && CanUserDeleteNode();
}

// 삭제와 연결 해제를 같은 트랜잭션에 포함하여 노드 ID와 연결을 Undo로 복구한다.
bool UDialogueFlowEdGraphNode::DeleteFromGraph()
{
	if (CanDeleteFromGraph() == false)
	{
		return false;
	}
	const FScopedTransaction Transaction(NSLOCTEXT("DialogueKit", "DeleteFlowNodeTransaction", "대화 노드 삭제"));
	GetGraph()->Modify();
	Modify();
	DestroyNode();
	return true;
}

// 핀 메뉴와 구분하여 노드 본문 우클릭에만 삭제 확인 액션을 제공한다.
void UDialogueFlowEdGraphNode::GetNodeContextMenuActions(UToolMenu* Menu, UGraphNodeContextMenuContext* Context) const
{
	Super::GetNodeContextMenuActions(Menu, Context);
	if (Menu == nullptr || Context == nullptr || Context->Node != this || Context->Pin || Context->bIsDebugging || CanDeleteFromGraph() == false)
	{
		return;
	}
	const TWeakObjectPtr<UDialogueFlowEdGraphNode> WeakNode(const_cast<UDialogueFlowEdGraphNode*>(this));
	FToolMenuSection& Section = Menu->AddSection(TEXT("DialogueFlowActions"), NSLOCTEXT("DialogueKit", "FlowActions", "대화 노드 작업"));
	Section.AddMenuEntry(
		TEXT("DeleteDialogueFlowNode"),
		NSLOCTEXT("DialogueKit", "DeleteFlowNode", "노드 삭제…"),
		NSLOCTEXT("DialogueKit", "DeleteFlowNodeTooltip", "이 노드와 연결선을 삭제합니다."),
		FSlateIcon(TEXT("DialogueKitEditorStyle"), TEXT("DialogueKitEditor.NodeDeleteNodeIcon")),
		FUIAction(FExecuteAction::CreateLambda([WeakNode]()
		{
			if (WeakNode.IsValid() == false || WeakNode->CanDeleteFromGraph() == false)
			{
				return;
			}
			const FText Message = FText::Format(NSLOCTEXT("DialogueKit", "ConfirmDeleteFlowNode", "{0} 노드와 연결선을 삭제하시겠습니까?"), WeakNode->GetNodeTitle(ENodeTitleType::ListView));
			if (FMessageDialog::Open(EAppMsgType::YesNo, Message) == EAppReturnType::Yes && WeakNode.IsValid())
			{
				WeakNode->DeleteFromGraph();
			}
		}), FCanExecuteAction::CreateLambda([WeakNode]()
		{
			return WeakNode.IsValid() && WeakNode->CanDeleteFromGraph();
		})));
}
