#include "DialogueEdEndGraphNode.h"
#include "DialogueMaker/DialogueEndNodeInfo.h"

FText UDialogueEdEndGraphNode::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	if (NodeInfo != nullptr && NodeInfo->Action != EDialogueNodeAction::None)
	{
		FString Result = UEnum::GetDisplayValueAsText(NodeInfo->Action).ToString();
		if (NodeInfo->ActionDetails.IsEmpty() == false)
		{
			FString ActionData = NodeInfo->ActionDetails;
			if (ActionData.Len() > 15)
			{
				ActionData = ActionData.Left(15) + TEXT("...");
			}

			Result += TEXT(" - ") + ActionData;
		}

		return FText::FromString(Result);
	}

	return FText::FromString("End");
}

FLinearColor UDialogueEdEndGraphNode::GetNodeTitleColor() const
{
	return FLinearColor::Blue;
}

bool UDialogueEdEndGraphNode::CanUserDeleteNode() const
{
	return true;
}

void UDialogueEdEndGraphNode::GetNodeContextMenuActions(class UToolMenu* Menu,
	class UGraphNodeContextMenuContext* Context) const
{
	FToolMenuSection& Section = Menu->AddSection(TEXT("Section Name"), FText::FromString(TEXT("End Node Actions")));
	UDialogueEdEndGraphNode* Node = (UDialogueEdEndGraphNode*)this;
	Section.AddMenuEntry(
		TEXT("Delete Entry"),
		FText::FromString(TEXT("Delete Entry")),
		FText::FromString(TEXT("Delete the node")),
		FSlateIcon(TEXT("DialogueMakerEditorStyle"), TEXT("DialogueMakerEditor.NodeDeleteNodeIcon")),
		FUIAction(FExecuteAction::CreateLambda(
			[Node] (){
				Node->GetGraph()->RemoveNode(Node);
	}
	))
	);


}

UEdGraphPin* UDialogueEdEndGraphNode::CreateCustomPin(EEdGraphPinDirection Direction, FName Name)
{
	FName Category = TEXT("Inputs");
	FName SubCategory = TEXT("EndPin");
	
	UEdGraphPin* Pin = CreatePin(Direction, Category, SubCategory, Name);
	Pin->PinType.PinSubCategory = SubCategory;

	return Pin;
}

UEdGraphPin* UDialogueEdEndGraphNode::CreateDefaultInputPin()
{
	return CreateCustomPin(EGPD_Input, FName(TEXT("Finish")));
}

void UDialogueEdEndGraphNode::InitNodeInfo(UObject* Outer)
{
	NodeInfo = NewObject<UDialogueEndNodeInfo>(Outer);
}

void UDialogueEdEndGraphNode::SetDialogueNodeInfo(class UDialogueNodeInfoBase* NewDialogueNodeInfo)
{
	NodeInfo = Cast<UDialogueEndNodeInfo>(NewDialogueNodeInfo);
}

UDialogueNodeInfoBase* UDialogueEdEndGraphNode::GetNodeInfo() const
{
	return NodeInfo;
}

EDialogueType UDialogueEdEndGraphNode::GetDialogueNodeType() const
{
	return EDialogueType::EndNode;
}

void UDialogueEdEndGraphNode::OnPropertiesChanged()
{
	Modify();
}
