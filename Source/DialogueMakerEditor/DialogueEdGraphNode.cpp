// Fill out your copyright notice in the Description page of Project Settings.


#include "DialogueEdGraphNode.h"

#include "EditorStyleSet.h"
#include "DialogueMaker/DialogueNodeInfo.h"

void UDialogueEdGraphNode::AllocateDefaultPins()
{
	// 입력핀, 서로 다른 대화로부터 해당 대화로 이어질  수 있음
	CreatePin(EGPD_Input, TEXT("MultipleNodes"), FName(), TEXT("In"));
	// 출력핀, 선택지가 없는 경우 사용
	CreatePin(EGPD_Output, TEXT("SingleNode"), FName(), TEXT("Out"));
	
}

UEdGraphPin* UDialogueEdGraphNode::CreateCustomPin(EEdGraphPinDirection Direction, FName Name)
{
	FName Category = (Direction == EGPD_Input) ? TEXT("Inputs") : TEXT("Outputs");
	FName SubCategory = TEXT("DialoguePin");

	UEdGraphPin* Pin = CreatePin(Direction, Category, Name);
	Pin->PinType.PinSubCategory = SubCategory;

	return Pin;
}

void UDialogueEdGraphNode::SetDialogueNodeInfo(class UDialogueNodeInfo* NewDialogueNodeInfo)
{
	DialogueNodeInfo = NewDialogueNodeInfo;	
}

/* Dialogue Graph Editor에서 DialogueNode의 DialogueNodeInfo 내 Responses를 수정하면
 * DialogueNode의 Output Pin과 동기화 */
void UDialogueEdGraphNode::SyncPinWithResponses()
{
	UDialogueNodeInfo* NodeInfo = GetDialogueNodeInfo();
	int GraphNodePinsCount = Pins.Num() - 1;	// 첫 번째 pin은 언제나 input pin이라고 가정한다
	int NodeInfoPinsCount = NodeInfo->DialogueResponses.Num();

	while (GraphNodePinsCount > NodeInfoPinsCount)
	{
		RemovePinAt(GraphNodePinsCount - 1, EGPD_Output);
		GraphNodePinsCount--;
	}

	while (NodeInfoPinsCount > GraphNodePinsCount)
	{
		CreateCustomPin(EGPD_Output, FName(NodeInfo->DialogueResponses[GraphNodePinsCount].ToString()));
		GraphNodePinsCount++;
	}

	int PinIndex = 1;	// 첫 번째 pin은 언제나 input pin이라고 가정한다
	for (FText& Option : DialogueNodeInfo->DialogueResponses)
	{
		GetPinAt(PinIndex)->PinName = FName(Option.ToString());
		PinIndex++;
	}
}

class UDialogueNodeInfo* UDialogueEdGraphNode::GetDialogueNodeInfo() const
{
	return DialogueNodeInfo;	
}

FText UDialogueEdGraphNode::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	// 노드 제목이 없는 경우 대사의 첫 부분으로 간략하게 보여준다
	if (DialogueNodeInfo->Title.IsEmpty())
	{
		FString DialogueTextString = DialogueNodeInfo->DialogueText.ToString();
		if (DialogueTextString.Len() > 15)
		{
			DialogueTextString = DialogueTextString.Left(15) + TEXT("...");
		}
		return FText::FromString(DialogueTextString);
	}
	
	return DialogueNodeInfo->Title;
}

FLinearColor UDialogueEdGraphNode::GetNodeTitleColor() const
{
	return Super::GetNodeTitleColor();
}

bool UDialogueEdGraphNode::CanUserDeleteNode() const
{
	return Super::CanUserDeleteNode();
}

void UDialogueEdGraphNode::GetNodeContextMenuActions(class UToolMenu* Menu,
	class UGraphNodeContextMenuContext* Context) const
{
	FToolMenuSection& Section = Menu->AddSection(TEXT("Section Name"), FText::FromString(TEXT("Custom Node Actions")));
	UDialogueEdGraphNode* Node = (UDialogueEdGraphNode*)this;
	Section.AddMenuEntry(
		TEXT("Pin Entry"),
		FText::FromString(TEXT("Add Response")),
		FText::FromString(TEXT("Create a new Response")),
		FSlateIcon(TEXT("DialogueMakerEditorStyle"), TEXT("DialogueMakerEditor.NodeDeletePinIcon")),
		FUIAction(FExecuteAction::CreateLambda(
			[Node] (){
				Node->GetDialogueNodeInfo()->DialogueResponses.Add(FText::FromString(TEXT("Response")));
				Node->SyncPinWithResponses();
				
				Node->GetGraph()->NotifyGraphChanged();
				Node->GetGraph()->Modify();
	}
	))
	);

	Section.AddMenuEntry(
		TEXT("Delete Pin Entry"),
		FText::FromString(TEXT("Delete Response")),
		FText::FromString(TEXT("Delete a last Response")),
		FSlateIcon(TEXT("DialogueMakerEditorStyle"), TEXT("DialogueMakerEditor.NodeDeletePinIcon")),
		FUIAction(FExecuteAction::CreateLambda(
			[Node] (){
						UEdGraphPin* Pin = Node->GetPinAt(Node->Pins.Num() - 1);
				if (Pin->Direction != EGPD_Input)
				{
					UDialogueNodeInfo* NodeInfo = Node->GetDialogueNodeInfo();
					NodeInfo->DialogueResponses.RemoveAt(NodeInfo->DialogueResponses.Num() - 1);
					Node->SyncPinWithResponses();
					
					Node->GetGraph()->NotifyGraphChanged();
					Node->GetGraph()->Modify();
				}
	}
	))
	);

	Section.AddMenuEntry(
		TEXT("Delete Entry"),
		FText::FromString(TEXT("Delete Node")),
		FText::FromString(TEXT("Delete the node")),
		FSlateIcon(TEXT("DialogueMakerEditorStyle"), TEXT("DialogueMakerEditor.NodeDeleteNodeIcon")),
		FUIAction(FExecuteAction::CreateLambda(
			[Node] (){
				Node->GetGraph()->RemoveNode(Node);

	}
	))
	);
}
