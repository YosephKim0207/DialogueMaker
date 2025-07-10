#include "DialogueEdStartGraphNode.h"

FText UDialogueEdStartGraphNode::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return FText::FromString("Start");
}

FLinearColor UDialogueEdStartGraphNode::GetNodeTitleColor() const
{
	return FLinearColor::Red;
}

bool UDialogueEdStartGraphNode::CanUserDeleteNode() const
{
	return false;
}

UEdGraphPin* UDialogueEdStartGraphNode::CreateCustomPin(EEdGraphPinDirection Direction, FName Name)
{
	FName Category = TEXT("Outputs");
	FName SubCategory = TEXT("StartPin");

	UEdGraphPin* Pin = CreatePin(Direction, Category, Name);
	Pin->PinType.PinSubCategory = SubCategory;

	return Pin;
}

EDialogueType UDialogueEdStartGraphNode::GetDialogueNodeType() const
{
	return EDialogueType::StartNode;
}
