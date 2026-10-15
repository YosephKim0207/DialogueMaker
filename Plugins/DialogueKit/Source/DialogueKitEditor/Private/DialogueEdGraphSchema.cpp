// Fill out your copyright notice in the Description page of Project Settings.

#include "DialogueEdGraphSchema.h"
#include "DialogueFlowEdGraphNode.h"
#include "DialogueLineAuthoring.h"
#include "DialogueChoiceEdGraphNode.h"
#include "DialogueStateChangeEdGraphNode.h"
#include "ScopedTransaction.h"
#include "DialogueBranchEdGraphNode.h"
#include "DialogueEdEndGraphNode.h"
#include "DialogueEdGraphNode.h"
#include "DialogueEdStartGraphNode.h"

const FPinConnectionResponse UDialogueEdGraphSchema::CanCreateConnection(const UEdGraphPin* A,
                                                                         const UEdGraphPin* B) const
{
	if (A == nullptr || B == nullptr)
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("Pin is nullptr"));
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
