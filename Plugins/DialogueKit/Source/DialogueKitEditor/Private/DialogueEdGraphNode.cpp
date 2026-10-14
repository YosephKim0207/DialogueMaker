// Fill out your copyright notice in the Description page of Project Settings.


#include "DialogueEdGraphNode.h"
#include "DialogueNodeInfo.h"
#include "DialogueNodeType.h"

void UDialogueEdGraphNode::AllocateDefaultPins()
{
	// 입력핀, 서로 다른 대화로부터 해당 대화로 이어질  수 있음
	CreatePin(EGPD_Input, TEXT("MultipleNodes"), FName(), TEXT("In"));
	// 출력핀, 선택지가 없는 경우 사용
	CreatePin(EGPD_Output, TEXT("SingleNode"), FName(), TEXT("Out"));
}

UEdGraphPin* UDialogueEdGraphNode::CreateDefaultInputPin()
{
	return CreateCustomPin(EGPD_Input, FName(TEXT("Display")));
}

void UDialogueEdGraphNode::CreateDefaultOutputPin()
{
	FString DefaultResponse = TEXT("Continue");
	CreateCustomPin(EGPD_Output, FName(DefaultResponse));
	GetDialogueNodeInfo()->AddDialogueChoice(FDialogueChoice(FText::FromString(DefaultResponse)));
	SyncPinWithResponses();
}

EDialogueType UDialogueEdGraphNode::GetDialogueNodeType() const
{
	return EDialogueType::DialogueNode;
}

void UDialogueEdGraphNode::OnPropertiesChanged()
{
	SyncPinWithResponses();
}

UEdGraphPin* UDialogueEdGraphNode::CreateCustomPin(EEdGraphPinDirection Direction, FName Name)
{
	FName Category = (Direction == EGPD_Input) ? TEXT("Inputs") : TEXT("Outputs");
	FName SubCategory = TEXT("DialoguePin");

	UEdGraphPin* Pin = CreatePin(Direction, Category, Name);
	Pin->PinType.PinSubCategory = SubCategory;

	return Pin;
}

/* Dialogue Graph Editor에서 DialogueNode의 DialogueNodeInfo 내 Responses를 수정하면
 * DialogueNode의 Output Pin과 동기화 */
void UDialogueEdGraphNode::SyncPinWithResponses()
{
	UDialogueNodeInfo* NodeInfo = GetDialogueNodeInfo();
	NodeInfo->EnsureChoiceIds();
	TArray<UEdGraphPin*> OldOutputs;
	TArray<UEdGraphPin*> OrderedPins;
	for (UEdGraphPin* Pin : Pins)
	{
		if (Pin->Direction == EGPD_Output)
		{
			OldOutputs.Add(Pin);
		}
		else
		{
			OrderedPins.Add(Pin);
		}
	}
	for (const FDialogueChoice& Choice : NodeInfo->GetDialogueChoices())
	{
		UEdGraphPin* MatchedPin = nullptr;
		for (UEdGraphPin* Pin : OldOutputs)
		{
			if (Pin->PersistentGuid == Choice.ChoiceId)
			{
				MatchedPin = Pin;
				break;
			}
		}
		// 기존 ID 없는 프로토타입을 명시 편집할 때만 기존 순서의 미지정 핀을 승계한다.
		if (MatchedPin == nullptr)
		{
			for (UEdGraphPin* Pin : OldOutputs)
			{
				if (Pin->PersistentGuid.IsValid() == false)
				{
					MatchedPin = Pin;
					break;
				}
			}
		}
		if (MatchedPin == nullptr)
		{
			MatchedPin = CreateCustomPin(EGPD_Output, NAME_None);
		}
		MatchedPin->PersistentGuid = Choice.ChoiceId;
		MatchedPin->PinName = FName(Choice.ResponseText.ToString());
		OrderedPins.Add(MatchedPin);
		OldOutputs.Remove(MatchedPin);
	}
	for (UEdGraphPin* Pin : OldOutputs)
	{
		Pin->BreakAllPinLinks();
		RemovePin(Pin);
	}
	Pins = MoveTemp(OrderedPins);
}

void UDialogueEdGraphNode::InitNodeInfo(UObject* Outer)
{
	DialogueNodeInfo = NewObject<UDialogueNodeInfo>(Outer);
}

void UDialogueEdGraphNode::SetDialogueNodeInfo(class UDialogueNodeInfoBase* NewDialogueNodeInfo)
{
	DialogueNodeInfo = Cast<UDialogueNodeInfo>(NewDialogueNodeInfo);
}

UDialogueNodeInfoBase* UDialogueEdGraphNode::GetNodeInfo() const
{
	return DialogueNodeInfo;
}

UDialogueNodeInfo* UDialogueEdGraphNode::GetDialogueNodeInfo() const
{
	return DialogueNodeInfo;
}

FText UDialogueEdGraphNode::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	UDialogueNodeInfo* NodeInfo = Cast<UDialogueNodeInfo>(DialogueNodeInfo);
	// 노드 제목이 없는 경우 대사의 첫 부분으로 간략하게 보여준다
	if (NodeInfo->GetTitle().IsEmpty())
	{
		FString DialogueTextString = NodeInfo->GetDialogueText().ToString();
		if (DialogueTextString.Len() > 15)
		{
			DialogueTextString = DialogueTextString.Left(15) + TEXT("...");
		}
		return FText::FromString(DialogueTextString);
	}
	
	return NodeInfo->GetTitle();
}

FLinearColor UDialogueEdGraphNode::GetNodeTitleColor() const
{
	return Super::GetNodeTitleColor();
}

bool UDialogueEdGraphNode::CanUserDeleteNode() const
{
	return Super::CanUserDeleteNode();
}

void UDialogueEdGraphNode::GetNodeContextMenuActions(UToolMenu* Menu, UGraphNodeContextMenuContext* Context) const
{
	FToolMenuSection& Section = Menu->AddSection(TEXT("Section Name"), FText::FromString(TEXT("Dialogue Node Actions")));
	UDialogueEdGraphNode* Node = (UDialogueEdGraphNode*)this;
	Section.AddMenuEntry(
		TEXT("Pin Entry"),
		FText::FromString(TEXT("Add Response")),
		FText::FromString(TEXT("Create a new Response")),
                FSlateIcon(TEXT("DialogueKitEditorStyle"), TEXT("DialogueKitEditor.NodeAddIcon")),
		FUIAction(FExecuteAction::CreateLambda(
			[Node] (){
				Node->GetDialogueNodeInfo()->AddDialogueChoice(FDialogueChoice(FText::FromString(TEXT("Response"))));
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
                FSlateIcon(TEXT("DialogueKitEditorStyle"), TEXT("DialogueKitEditor.NodeDeletePinIcon")),
		FUIAction(FExecuteAction::CreateLambda(
			[Node] (){
						UEdGraphPin* Pin = Node->GetPinAt(Node->Pins.Num() - 1);
				if (Pin->Direction != EGPD_Input)
				{
					UDialogueNodeInfo* NodeInfo = Node->GetDialogueNodeInfo();
					NodeInfo->RemoveDialogueChoiceAt(NodeInfo->GetDialogueChoices().Num() - 1);
					
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
                FSlateIcon(TEXT("DialogueKitEditorStyle"), TEXT("DialogueKitEditor.NodeDeleteNodeIcon")),
		FUIAction(FExecuteAction::CreateLambda(
			[Node] (){
				Node->GetGraph()->RemoveNode(Node);

	}
	))
	);
}
