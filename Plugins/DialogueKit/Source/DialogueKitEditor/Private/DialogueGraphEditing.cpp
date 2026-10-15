#include "DialogueGraphEditing.h"
#include "DialogueEdGraphNodeBase.h"
#include "DialogueChoiceNodeInfo.h"
#include "DialogueNodeInfo.h"
#include "DialogueCommentNode.h"
#include "DialogueGraph.h"
#include "DialogueGraphCodec.h"
#include "DialogueEdGraphNode.h"
#include "DialogueFlowEdGraphNode.h"
#include "DialogueChoiceEdGraphNode.h"
#include "DialogueBranchEdGraphNode.h"
#include "DialogueStateChangeEdGraphNode.h"
#include "DialogueEdEndGraphNode.h"
#include "DialogueRerouteNode.h"
#include "EdGraphNode_Comment.h"
#include "EdGraph/EdGraph.h"
#include "ScopedTransaction.h"

// 원본 핀에서 새 핀으로 매핑하므로 외부 연결이나 원본 링크를 수정하지 않는다.
TArray<UEdGraphNode*> FDialogueGraphEditing::Duplicate(UEdGraph* Graph, const TSet<UObject*>& Selection)
{
	TArray<UEdGraphNode*> Result;
	if (Graph == nullptr || Graph->bEditable == false) return Result;
	TArray<UEdGraphNode*> Sources;
	for (UObject* Object : Selection)
	{
		UEdGraphNode* Node = Cast<UEdGraphNode>(Object);
		UDialogueEdGraphNodeBase* Typed = Cast<UDialogueEdGraphNodeBase>(Node);
		if (Node && Graph->Nodes.Contains(Node) && Node->CanDuplicateNode() &&
			((Typed && Typed->GetDialogueNodeType() != EDialogueType::StartNode) || Cast<UEdGraphNode_Comment>(Node)))
		{
			Sources.Add(Node);
		}
	}
	if (Sources.IsEmpty()) return Result;
	const TSet<UClass*> Supported = {UDialogueEdGraphNode::StaticClass(), UDialogueFlowEdGraphNode::StaticClass(),
		UDialogueChoiceEdGraphNode::StaticClass(), UDialogueBranchEdGraphNode::StaticClass(), UDialogueStateChangeEdGraphNode::StaticClass(),
		UDialogueEdEndGraphNode::StaticClass(), UDialogueRerouteNode::StaticClass(), UEdGraphNode_Comment::StaticClass()};
	for (UEdGraphNode* Source : Sources)
	{
		if (Supported.Contains(Source->GetClass()) == false) return Result;
	}
	// 핀/선택 ID가 손상된 그래프에서는 부분 복제를 시작하지 않는다.
	UDialogueGraph* ValidationAsset = NewObject<UDialogueGraph>();
	FString Error;
	if (FDialogueGraphCodec::ExportGraph(Graph, ValidationAsset, Error) == false) return Result;
	Sources.Sort([](const UEdGraphNode& A, const UEdGraphNode& B) { return A.NodeGuid < B.NodeGuid; });
	const FScopedTransaction Transaction(NSLOCTEXT("DialogueKit", "DuplicateNodes", "대화 노드 복제"));
	Graph->Modify();
	TMap<const UEdGraphPin*, UEdGraphPin*> Pins;
	for (UEdGraphNode* Source : Sources)
	{
		UEdGraphNode* New = nullptr;
		if (auto* Typed = Cast<UDialogueEdGraphNodeBase>(Source))
		{
			auto* Copy = NewObject<UDialogueEdGraphNodeBase>(Graph, Source->GetClass(), NAME_None, RF_Transactional);
			UDialogueNodeInfoBase* Info = Typed->GetNodeInfo() ? DuplicateObject(Typed->GetNodeInfo(), Copy) : nullptr;
			Copy->SetDialogueNodeInfo(Info);
			TMap<FGuid, FGuid> Choices;
			if (auto* Choice = Cast<UDialogueChoiceNodeInfo>(Info))
			{
				for (FDialogueOption& Option : Choice->Options)
				{
					const FGuid OldId = Option.ChoiceId;
					Option.ChoiceId = FGuid::NewGuid();
					Choices.Add(OldId, Option.ChoiceId);
				}
			}
			if (auto* Line = Cast<UDialogueNodeInfo>(Info))
			{
				Line->ResetReadIdentityForDuplicate();
				TArray<FDialogueChoice> OldChoices = Line->GetDialogueChoices();
				while (Line->GetDialogueChoices().IsEmpty() == false)
				{
					Line->RemoveDialogueChoiceAt(Line->GetDialogueChoices().Num() - 1);
				}
				for (FDialogueChoice& Choice : OldChoices)
				{
					const FGuid OldId = Choice.ChoiceId;
					Choice.ChoiceId = FGuid::NewGuid();
					Choices.Add(OldId, Choice.ChoiceId);
					Line->AddDialogueChoice(Choice);
				}
			}
			for (UEdGraphPin* Pin : Source->Pins)
			{
				UEdGraphPin* NewPin = Copy->CreateCustomPin(Pin->Direction, Pin->PinName);
				NewPin->PinType = Pin->PinType;
				NewPin->PersistentGuid = Choices.Contains(Pin->PersistentGuid) ? Choices[Pin->PersistentGuid] : FGuid();
				Pins.Add(Pin, NewPin);
			}
			New = Copy;
		}
		else
		{
			New = DuplicateObject(Source, Graph);
			New->SetFlags(RF_Transactional);
			FDialogueCommentDetails::SetDescription(New, FDialogueCommentDetails::GetDescription(Source));
		}
		New->CreateNewGuid();
		New->NodePosX = Source->NodePosX + 48;
		New->NodePosY = Source->NodePosY + 48;
		New->NodeComment = Source->NodeComment;
		New->NodeWidth = Source->NodeWidth;
		New->NodeHeight = Source->NodeHeight;
		New->bCommentBubbleVisible = Source->bCommentBubbleVisible;
		Graph->AddNode(New, true, false);
		Result.Add(New);
	}
	for (const auto& Pair : Pins)
	{
		if (Pair.Key->Direction != EGPD_Output) continue;
		for (const UEdGraphPin* Target : Pair.Key->LinkedTo)
		{
			if (UEdGraphPin** NewTarget = Pins.Find(Target)) Pair.Value->MakeLinkTo(*NewTarget);
		}
	}
	Graph->NotifyGraphChanged();
	Graph->MarkPackageDirty();
	return Result;
}
