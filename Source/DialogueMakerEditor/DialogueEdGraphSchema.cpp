// Fill out your copyright notice in the Description page of Project Settings.

#include "DialogueEdGraphSchema.h"
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
	
	return FPinConnectionResponse(CONNECT_RESPONSE_MAKE, TEXT("Connect possible."));
}

// Dialogue Graph에서 마우스 우클릭시 노출시킬 생성 가능한 노드들 제공
void UDialogueEdGraphSchema::GetGraphContextActions(FGraphContextMenuBuilder& ContextMenuBuilder) const
{
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
	
	TSharedPtr<FNewNodeAction> BranchNodeAction = MakeShareable(new FNewNodeAction(
		UDialogueBranchEdGraphNode::StaticClass(),
		FText::FromString("Flow Control"),
		FText::FromString("Branch Node"),
		FText::FromString("Branch Statement\nIf Condition is true, execution goes to True, otherwise it goes to False"),
		0));
	
	ContextMenuBuilder.AddAction(NewNodeAction);
	ContextMenuBuilder.AddAction(NewEndNodeAction);
	ContextMenuBuilder.AddAction(BranchNodeAction);
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
