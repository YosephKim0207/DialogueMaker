#include "DialogueBranchEdGraphNode.h"

#include "EdGraphSchema_K2.h"

#include "DialogueBranchNodeInfoBase.h"
#define LOCTEXT_NAMESPACE "DialogueGraphEditor"

FText UDialogueBranchEdGraphNode::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return FText::FromString("Branch");
}

FText UDialogueBranchEdGraphNode::GetTooltipText() const
{
	return FText::FromString("Branch Statement\nIf Condition is true, execution goes to True, otherwise it goes to False");
}

FLinearColor UDialogueBranchEdGraphNode::GetNodeTitleColor() const
{
	return FLinearColor::Yellow;
}

// Branch 실행 입력 하나와 True/False 출력만 구성한다.
void UDialogueBranchEdGraphNode::AllocateDefaultPins()
{
	CreateDefaultInputPin();
	CreateDefaultOutputPin();
}

FSlateIcon UDialogueBranchEdGraphNode::GetIconAndTint(FLinearColor& OutColor) const
{
	static FSlateIcon Icon(FAppStyle::GetAppStyleSetName(), "GraphEditor.Branch_16x");
	return Icon;
}

UEdGraphPin* UDialogueBranchEdGraphNode::CreateCustomPin(EEdGraphPinDirection Direction, FName Name)
{
	FName Category = TEXT("DialogueFlow");
	FName SubCategory = TEXT("DialoguePin");

	UEdGraphPin* Pin = CreatePin(Direction, Category, Name);
	Pin->PinType.PinSubCategory = SubCategory;

	return Pin;
}

UEdGraphPin* UDialogueBranchEdGraphNode::CreateDefaultInputPin()
{
	return CreateCustomPin(EGPD_Input, FName(TEXT("Condition")));
}

void UDialogueBranchEdGraphNode::CreateDefaultOutputPin()
{
	CreateCustomPin(EGPD_Output, FName(TEXT("True")));
	CreateCustomPin(EGPD_Output, FName(TEXT("False")));
}

void UDialogueBranchEdGraphNode::GetNodeContextMenuActions(UToolMenu* Menu, UGraphNodeContextMenuContext* Context) const
{
	FToolMenuSection& Section = Menu->AddSection(TEXT("Section Name"), FText::FromString(TEXT("Dialogue Node Actions")));
	UDialogueBranchEdGraphNode* Node = (UDialogueBranchEdGraphNode*)this;

	Section.AddMenuEntry(
		TEXT("Delete Entry"),
		FText::FromString(TEXT("Delete Node")),
		FText::FromString(TEXT("Delete the node")),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), "GraphEditor.Branch_16x"),
		FUIAction(FExecuteAction::CreateLambda(
			[Node] (){
				Node->GetGraph()->RemoveNode(Node);

	}
	))
	);
}

void UDialogueBranchEdGraphNode::InitNodeInfo(UObject* Outer)
{
	DialogueBranchNodeInfo = NewObject<UDialogueBranchNodeInfoBase>(Outer, NAME_None, RF_Transactional);
}

void UDialogueBranchEdGraphNode::SetDialogueNodeInfo(UDialogueNodeInfoBase* NewDialogueNodeInfo)
{
	DialogueBranchNodeInfo = Cast<UDialogueBranchNodeInfoBase>(NewDialogueNodeInfo);
}

UDialogueBranchNodeInfoBase* UDialogueBranchEdGraphNode::GetDialogueBranchNodeInfo() const
{
	return DialogueBranchNodeInfo;
}

UDialogueNodeInfoBase* UDialogueBranchEdGraphNode::GetNodeInfo() const
{
	return DialogueBranchNodeInfo;
}

EDialogueType UDialogueBranchEdGraphNode::GetDialogueNodeType() const
{
	return EDialogueType::BranchNode;
}
