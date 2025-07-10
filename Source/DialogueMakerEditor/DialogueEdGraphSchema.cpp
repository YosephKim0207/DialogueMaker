// Fill out your copyright notice in the Description page of Project Settings.


#include "DialogueEdGraphSchema.h"

#include "DialogueEdEndGraphNode.h"
#include "DialogueEdGraphNode.h"
#include "DialogueEdStartGraphNode.h"
#include "DialogueMaker/DialogueNodeInfo.h"

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
	
	return FPinConnectionResponse(CONNECT_RESPONSE_MAKE, TEXT("Connect possible."));
}

void UDialogueEdGraphSchema::GetGraphContextActions(FGraphContextMenuBuilder& ContextMenuBuilder) const
{
	// TODO 영상 따라서 구현한 후 FEdGraphSchemaAction이 아닌 fedGraphSchemaAction_NewNode로 직접 대체해보기(아마도 dialogueEdGraphNOd의 AllocateDefaultPin과 연계 가능성 높음?
	// TSharedPtr<FEdGraphSchemaAction_NewNode> NewNodeAction = MakeShareable(new FEdGraphSchemaAction_NewNode(
	// 	FText::FromString("Dialogue"),
	// 	FText::FromString("새 대화 노드"),
	// 	FText::FromString("새로운 대화 노드를 추가합니다."),
	// 	0
	// ));
	//
	// NewNodeAction->NodeTemplate = NewObject<UDialogueEdGraphNode>(ContextMenuBuilder.OwnerOfTemporaries);
	// ContextMenuBuilder.AddAction(NewNodeAction);

	TSharedPtr<FNewNodeAction> NewNodeAction = MakeShareable(new FNewNodeAction(
		UDialogueEdGraphNode::StaticClass(),
		FText::FromString("Nodes"),
		FText::FromString("New Dialogue Node"),
		FText::FromString("Add New Dialogue Node."),
		0));

	TSharedPtr<FNewNodeAction> NewEndNodeAction = MakeShareable(new FNewNodeAction(
		UDialogueEdEndGraphNode::StaticClass(),
		FText::FromString("Nodes"),
		FText::FromString("New End Node"),
		FText::FromString("Add New End Node."),
		0));
	
	ContextMenuBuilder.AddAction(NewNodeAction);
	ContextMenuBuilder.AddAction(NewEndNodeAction);
}

void UDialogueEdGraphSchema::CreateDefaultNodesForGraph(UEdGraph& Graph) const
{
	UDialogueEdStartGraphNode* StartNode = NewObject<UDialogueEdStartGraphNode>(&Graph);
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
	UDialogueEdGraphNodeBase* Result = NewObject<UDialogueEdGraphNodeBase>(ParentGraph, ClassTemplate);
	Result->CreateNewGuid();
	Result->NodePosX = Location.X;
	Result->NodePosY = Location.Y;
	Result->InitNodeInfo(Result);
	
	UEdGraphPin* InputPin = Result->CreateDefaultInputPin();
	Result->CreateDefaultOutputPin();
	
	if (FromPin != nullptr)
	{
		Result->GetSchema()->TryCreateConnection(FromPin, InputPin);
	}

	ParentGraph->Modify();
	ParentGraph->AddNode(Result, true, true);

	return Result;
}
