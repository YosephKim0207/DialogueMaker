// Fill out your copyright notice in the Description page of Project Settings.


#include "DialogueEdGraphSchema.h"

#include "DialogueEdGraphNode.h"

const FPinConnectionResponse UDialogueEdGraphSchema::CanCreateConnection(const UEdGraphPin* A,
                                                                         const UEdGraphPin* B) const
{
	// 서로 같은 노드끼리 연결하려는 경우
	if (A->GetOwningNode() == B->GetOwningNode())
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("같은 노드에 연결할 수 없습니다."));
	}

	// Input pin끼리 연결하려는 경우
	if (A->Direction == EGPD_Input && B->Direction == EGPD_Input)
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("입력 핀끼리 연결할 수 없습니다."));
	}

	// Output pin끼리 연결하려는 경우
	if (A->Direction == EGPD_Output && B->Direction == EGPD_Output)
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, TEXT("출력 핀끼리 연결할 수 없습니다."));
	}
	
	return FPinConnectionResponse(CONNECT_RESPONSE_MAKE, TEXT("연결 가능"));
}

void UDialogueEdGraphSchema::GetGraphContextActions(FGraphContextMenuBuilder& ContextMenuBuilder) const
{
	TSharedPtr<FEdGraphSchemaAction_NewNode> NewNodeAction = MakeShareable(new FEdGraphSchemaAction_NewNode(
		FText::FromString("Dialogue"),
		FText::FromString("새 대화 노드"),
		FText::FromString("새로운 대화 노드를 추가합니다."),
		0
	));

	NewNodeAction->NodeTemplate = NewObject<UDialogueEdGraphNode>(ContextMenuBuilder.OwnerOfTemporaries);
	ContextMenuBuilder.AddAction(NewNodeAction);
	
}
