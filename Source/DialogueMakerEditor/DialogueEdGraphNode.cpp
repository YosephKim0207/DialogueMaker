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

FText UDialogueEdGraphNode::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	// 노드 제목을 대사의 첫 부분으로 간략하게 보여줌
	// if (Dialogue.DialogueText.IsEmpty())
	// {
	// 	return FText::FromString(TEXT("Dialogue"));
	// }
	// return FText::FromString(Dialogue.DialogueText.ToString().Left(20) + TEXT("..."));

	if (DialogueNodeInfo->DialogueText.IsEmpty())
	{
		return FText::FromString(TEXT("Dialogue"));
	}
	return FText::FromString(DialogueNodeInfo->DialogueText.ToString().Left(20) + TEXT("..."));
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
		FText::FromString(TEXT("Add Pin")),
		FText::FromString(TEXT("Create a new pin")),
		FSlateIcon(TEXT("DialogueMakerEditorStyle"), TEXT("DialogueMakerEditor.NodeDeletePinIcon")),
		FUIAction(FExecuteAction::CreateLambda(
			[Node] (){
				Node->CreateCustomPin(
					EGPD_Output,
					TEXT("Another Output")
					);

				Node->GetGraph()->NotifyGraphChanged();
				Node->GetGraph()->Modify();
	}
	))
	);

	Section.AddMenuEntry(
		TEXT("Delete Pin Entry"),
		FText::FromString(TEXT("Delete Pin")),
		FText::FromString(TEXT("Delete a last pin")),
		FSlateIcon(TEXT("DialogueMakerEditorStyle"), TEXT("DialogueMakerEditor.NodeDeletePinIcon")),
		FUIAction(FExecuteAction::CreateLambda(
			[Node] (){
						UEdGraphPin* Pin = Node->GetPinAt(Node->Pins.Num() - 1);
				if (Pin->Direction != EGPD_Input)
				{
					Node->RemovePin(Pin);

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
