// Fill out your copyright notice in the Description page of Project Settings.

#include "DialogueEdGraphSchema.h"
#include "DialogueFlowEdGraphNode.h"
#include "DialogueLineAuthoring.h"
#include "DialogueRerouteNode.h"
#include "EdGraphNode_Comment.h"
#include "DialogueCommentNode.h"
#include "DialogueChoiceEdGraphNode.h"
#include "DialogueStateChangeEdGraphNode.h"
#include "ScopedTransaction.h"
#include "DialogueBranchEdGraphNode.h"
#include "DialogueEdEndGraphNode.h"
#include "DialogueEdGraphNode.h"
#include "DialogueEdStartGraphNode.h"

// 실행 흐름이 없는 편집용 영역 주석을 생성한다.
struct FDialogueCommentAction : FEdGraphSchemaAction
{
	FDialogueCommentAction() : FEdGraphSchemaAction(FText::FromString(TEXT("Nodes")), FText::FromString(TEXT("주석")), FText::FromString(TEXT("영역 주석 생성")), 0) {}
	virtual UEdGraphNode* PerformAction(UEdGraph* Graph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelect = true) override
	{
		if (Graph == nullptr || Graph->bEditable == false || FromPin) return nullptr;
		const FScopedTransaction Transaction(NSLOCTEXT("DialogueKit", "AddComment", "주석 생성"));
		Graph->Modify();
		auto* Node = NewObject<UEdGraphNode_Comment>(Graph, NAME_None, RF_Transactional);
		Node->CreateNewGuid();
		Node->NodePosX = Location.X;
		Node->NodePosY = Location.Y;
		Node->PostPlacedNewNode();
		Node->NodeComment = TEXT("주석");
		Graph->AddNode(Node, true, bSelect);
		Graph->NotifyGraphChanged();
		return Node;
	}
};

const FPinConnectionResponse UDialogueEdGraphSchema::CanCreateConnection(const UEdGraphPin* A,
                                                                         const UEdGraphPin* B) const
{
	if (A == nullptr || B == nullptr)
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("Pin is nullptr"));
	}
	
	if (!A->GetOwningNode()->GetGraph()->bEditable || !B->GetOwningNode()->GetGraph()->bEditable)
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("테스트 중에는 연결을 수정할 수 없습니다."));
	}

	// 서로 같은 노드끼리 연결하려는 경우
	if (A->GetOwningNode() == B->GetOwningNode())
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("Can't self connect."));
	}

	// Input pin끼리 연결하려는 경우
	if (A->Direction == EGPD_Input && B->Direction == EGPD_Input)
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("Can't connect with inputs"));
	}

	// Output pin끼리 연결하려는 경우
	if (A->Direction == EGPD_Output && B->Direction == EGPD_Output)
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("Can't connect with outputs."));
	}
	
	const UEdGraphPin* Output = A->Direction == EGPD_Output ? A : B;
	if (Output->LinkedTo.Num() > 0)
	{
		return FPinConnectionResponse(Output == A ? CONNECT_RESPONSE_BREAK_OTHERS_A : CONNECT_RESPONSE_BREAK_OTHERS_B, TEXT("Replace output connection."));
	}
	return FPinConnectionResponse(CONNECT_RESPONSE_MAKE, TEXT("Connect possible."));
}

// Dialogue Graph에서 마우스 우클릭시 노출시킬 생성 가능한 노드들 제공
void UDialogueEdGraphSchema::GetGraphContextActions(FGraphContextMenuBuilder& ContextMenuBuilder) const
{
	TSharedPtr<FNewNodeAction> NewNodeAction = MakeShareable(new FNewNodeAction(
		UDialogueFlowEdGraphNode::StaticClass(),
		FText::FromString("Nodes"),
		NSLOCTEXT("DialogueKit", "CreateLine", "Line 생성"),
		FText::FromString("Add Line."),
		0));

	TSharedPtr<FNewNodeAction> NewEndNodeAction = MakeShareable(new FNewNodeAction(
		UDialogueEdEndGraphNode::StaticClass(),
		FText::FromString("Nodes"),
		FText::FromString("New End Node"),
		FText::FromString("Add New End Node."),
		0));
	
	TSharedPtr<FNewNodeAction> BranchNodeAction = MakeShareable(new FNewNodeAction(
		UDialogueBranchEdGraphNode::StaticClass(),
		FText::FromString("Flow Control"),
		FText::FromString("Branch Node"),
		FText::FromString("Branch Statement\nIf Condition is true, execution goes to True, otherwise it goes to False"),
		0));
	
	ContextMenuBuilder.AddAction(MakeShareable(new FNewNodeAction(UDialogueChoiceEdGraphNode::StaticClass(), FText::FromString(TEXT("Nodes")), FText::FromString(TEXT("Choice")), FText::FromString(TEXT("Add Choice")), 0)));
	ContextMenuBuilder.AddAction(MakeShareable(new FNewNodeAction(UDialogueStateChangeEdGraphNode::StaticClass(), FText::FromString(TEXT("Nodes")), NSLOCTEXT("DialogueKit", "StateChangeNode", "상태 변경"), FText::FromString(TEXT("StateChange")), 0)));
	ContextMenuBuilder.AddAction(NewNodeAction);
	if (ContextMenuBuilder.FromPin == nullptr)
	{
		ContextMenuBuilder.AddAction(MakeShared<FDialogueCommentAction>());
	}
	ContextMenuBuilder.AddAction(MakeShared<FNewNodeAction>(UDialogueRerouteNode::StaticClass(), FText::FromString(TEXT("Nodes")),
		FText::FromString(TEXT("경유점")), FText::FromString(TEXT("연결선 정리용 경유점")), 0));
	if (ContextMenuBuilder.FromPin && ContextMenuBuilder.FromPin->Direction == EGPD_Output)
	{
		if (FDialogueLineAuthoring::FindNearestLines(ContextMenuBuilder.FromPin).IsEmpty())
		{
			ContextMenuBuilder.AddAction(MakeShared<FEdGraphSchemaAction_Dummy>(FText::FromString(TEXT("Nodes")),
				NSLOCTEXT("DialogueKit", "NoSourceLine", "연속된 화자의 Line 생성 — 사용 불가: 앞선 Line 없음"),
				NSLOCTEXT("DialogueKit", "NoSourceLineTip", "역방향 연결에서 원본 Line을 찾을 수 없습니다."), 0));
		}
		else
		{
			ContextMenuBuilder.AddAction(MakeShared<FContinueLineAction>());
		}
	}
	ContextMenuBuilder.AddAction(NewEndNodeAction);
	ContextMenuBuilder.AddAction(BranchNodeAction);
}

void UDialogueEdGraphSchema::CreateDefaultNodesForGraph(UEdGraph& Graph) const
{
	UDialogueEdStartGraphNode* StartNode = NewObject<UDialogueEdStartGraphNode>(&Graph, NAME_None, RF_Transactional);
	StartNode->CreateNewGuid();
	StartNode->NodePosX = 0;
	StartNode->NodePosY = 0;

	StartNode->CreateCustomPin(EGPD_Output, FName(TEXT("Start")));

	Graph.AddNode(StartNode);
	Graph.Modify();
}

// Dialogue Graph Node 정보 초기화
UEdGraphNode* FNewNodeAction::PerformAction(UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode)
{
	if (ParentGraph == nullptr || ParentGraph->bEditable == false || ParentGraph->GetSchema() == nullptr || ClassTemplate == nullptr ||
		(FromPin && (FromPin->Direction != EGPD_Output || FromPin->GetOwningNode() == nullptr ||
			ParentGraph->Nodes.Contains(FromPin->GetOwningNode()) == false ||
			FromPin->GetOwningNode()->Pins.Contains(FromPin) == false || FromPin->GetOwningNode()->GetGraph() != ParentGraph)))
	{
		return nullptr;
	}
	FScopedTransaction Transaction(FText::FromString(TEXT("Create Dialogue Node")));
	ParentGraph->Modify();
	UDialogueEdGraphNodeBase* Result = NewObject<UDialogueEdGraphNodeBase>(ParentGraph, ClassTemplate, NAME_None, RF_Transactional);
	Result->CreateNewGuid();
	Result->NodePosX = Location.X;
	Result->NodePosY = Location.Y;
	Result->InitNodeInfo(Result);
	
	UEdGraphPin* InputPin = Result->CreateDefaultInputPin();
	Result->CreateDefaultOutputPin();
	
	ParentGraph->AddNode(Result, true, bSelectNewNode);
	if (FromPin != nullptr)
	{
		if (FromPin->Direction != EGPD_Output || ParentGraph->GetSchema()->TryCreateConnection(FromPin, InputPin) == false)
		{
			Result->DestroyNode();
			Transaction.Cancel();
			return nullptr;
		}
	}

	ParentGraph->Modify();
	ParentGraph->NotifyGraphChanged();
	ParentGraph->MarkPackageDirty();

	return Result;
}

// 기존 선의 양끝을 한 트랜잭션으로 경유점에 연결한다.
void UDialogueEdGraphSchema::OnPinConnectionDoubleCicked(UEdGraphPin* PinA, UEdGraphPin* PinB, const FVector2D& Position) const
{
	if (PinA == nullptr || PinB == nullptr || PinA->LinkedTo.Contains(PinB) == false || PinA->Direction == PinB->Direction)
	{
		return;
	}
	UEdGraph* Graph = PinA->GetOwningNode()->GetGraph();
	if (Graph == nullptr || Graph->bEditable == false)
	{
		return;
	}
	UEdGraphPin* Output = PinA->Direction == EGPD_Output ? PinA : PinB;
	UEdGraphPin* Input = PinA->Direction == EGPD_Input ? PinA : PinB;
	const FScopedTransaction Transaction(NSLOCTEXT("DialogueKit", "InsertReroute", "경유점 삽입"));
	FNewNodeAction Action(UDialogueRerouteNode::StaticClass(), FText(), FText(), FText(), 0);
	UEdGraphNode* Node = Action.PerformAction(Graph, Output, Position);
	if (Node)
	{
		TryCreateConnection(Node->Pins[1], Input);
	}
}

// 실행 중 이미 열린 핀 메뉴에서도 연결 해제를 거부한다.
void UDialogueEdGraphSchema::BreakNodeLinks(UEdGraphNode& TargetNode) const
{
    if (TargetNode.GetGraph() && TargetNode.GetGraph()->bEditable) Super::BreakNodeLinks(TargetNode);
}

// Graph 위젯의 잠금과 별개로 Schema의 변경 경계도 보호한다.
void UDialogueEdGraphSchema::BreakPinLinks(UEdGraphPin& TargetPin, bool bSendsNodeNotifcation) const
{
    if (TargetPin.GetOwningNode()->GetGraph()->bEditable) Super::BreakPinLinks(TargetPin, bSendsNodeNotifcation);
}

// 단일 링크 해제 역시 테스트 중에는 실행하지 않는다.
void UDialogueEdGraphSchema::BreakSinglePinLink(UEdGraphPin* SourcePin, UEdGraphPin* TargetPin) const
{
    if (SourcePin && TargetPin && SourcePin->GetOwningNode()->GetGraph()->bEditable && TargetPin->GetOwningNode()->GetGraph()->bEditable)
        Super::BreakSinglePinLink(SourcePin, TargetPin);
}

// 전용 경유점 위젯도 일반 실행 핀과 같은 파란색 테두리/채움을 사용한다.
FLinearColor UDialogueEdGraphSchema::GetPinTypeColor(const FEdGraphPinType& PinType) const
{
	if (PinType.PinCategory == TEXT("DialogueFlow"))
	{
		return FLinearColor(0.1f, 0.4f, 0.8f);
	}
	return Super::GetPinTypeColor(PinType);
}
