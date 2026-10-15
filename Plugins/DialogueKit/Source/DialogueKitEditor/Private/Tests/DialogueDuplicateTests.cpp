#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "DialogueGraphEditing.h"
#include "DialogueEdGraphSchema.h"
#include "DialogueFlowEdGraphNode.h"
#include "DialogueChoiceEdGraphNode.h"
#include "DialogueChoiceNodeInfo.h"
#include "DialogueLineNodeInfo.h"
#include "DialogueGraph.h"
#include "DialogueGraphCodec.h"
#include "Editor.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDialogueDuplicateTest, "DialogueKit.Editor.Duplicate", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// 새 노드/핀/선택 ID와 내부 연결만 복제하고 Undo/Redo에서 동일하게 복원한다.
bool FDialogueDuplicateTest::RunTest(const FString& Parameters)
{
	auto* Asset = NewObject<UDialogueGraph>();
	Asset->InitializeBlockIdentity();
	auto* Graph = NewObject<UEdGraph>(Asset, NAME_None, RF_Transactional);
	Graph->Schema = UDialogueEdGraphSchema::StaticClass();
	Graph->GetSchema()->CreateDefaultNodesForGraph(*Graph);
	FNewNodeAction AddLine(UDialogueFlowEdGraphNode::StaticClass(), FText(), FText(), FText(), 0);
	FNewNodeAction AddChoice(UDialogueChoiceEdGraphNode::StaticClass(), FText(), FText(), FText(), 0);
	auto* A = CastChecked<UDialogueFlowEdGraphNode>(AddLine.PerformAction(Graph, Graph->Nodes[0]->Pins[0], FVector2D::ZeroVector));
	auto* Choice = CastChecked<UDialogueChoiceEdGraphNode>(AddChoice.PerformAction(Graph, A->Pins[1], FVector2D::ZeroVector));
	auto* B = AddLine.PerformAction(Graph, Choice->Pins[1], FVector2D::ZeroVector);
	auto* Info = CastChecked<UDialogueLineNodeInfo>(A->GetNodeInfo());
	Info->AdvanceReadVersion();
	TSet<UObject*> Selection = {Graph->Nodes[0], A, Choice};
	auto Copies = FDialogueGraphEditing::Duplicate(Graph, Selection);
	TestEqual(TEXT("Start excluded"), Copies.Num(), 2);
	UDialogueFlowEdGraphNode* NewLine = nullptr;
	UDialogueChoiceEdGraphNode* NewChoice = nullptr;
	for (UEdGraphNode* Node : Copies)
	{
		if (auto* C = Cast<UDialogueChoiceEdGraphNode>(Node)) NewChoice = C;
		else NewLine = Cast<UDialogueFlowEdGraphNode>(Node);
	}
	if (NewLine == nullptr || NewChoice == nullptr) return false;
	TestTrue(TEXT("Only internal edge copied"), NewLine->Pins[1]->LinkedTo.Contains(NewChoice->Pins[0]));
	TestTrue(TEXT("External edges omitted"), NewLine->Pins[0]->LinkedTo.IsEmpty() && NewChoice->Pins[1]->LinkedTo.IsEmpty());
	TestTrue(TEXT("Original edge retained"), Choice->Pins[1]->LinkedTo.Contains(B->Pins[0]));
	TestTrue(TEXT("Node identity new"), NewLine->NodeGuid != A->NodeGuid);
	TestTrue(TEXT("Pin identity new"), NewLine->Pins[0]->PinId != A->Pins[0]->PinId);
	const FGuid NewChoiceId = CastChecked<UDialogueChoiceNodeInfo>(NewChoice->GetNodeInfo())->Options[0].ChoiceId;
	TestTrue(TEXT("Choice identity new"), NewChoiceId != CastChecked<UDialogueChoiceNodeInfo>(Choice->GetNodeInfo())->Options[0].ChoiceId);
	TestEqual(TEXT("Choice pin mapped"), NewChoice->Pins[1]->PersistentGuid, NewChoiceId);
	TestEqual(TEXT("Read version reset"), CastChecked<UDialogueLineNodeInfo>(NewLine->GetNodeInfo())->GetReadVersion(), 1);
	TestTrue(TEXT("Undo duplication"), GEditor->UndoTransaction());
	TestEqual(TEXT("Original node count restored"), Graph->Nodes.Num(), 4);
	TestTrue(TEXT("Redo duplication"), GEditor->RedoTransaction());
	TestTrue(TEXT("Redo restores internal edge"), NewLine->Pins[1]->LinkedTo.Contains(NewChoice->Pins[0]));
	TestEqual(TEXT("Redo preserves new Choice ID"), NewChoice->Pins[1]->PersistentGuid, NewChoiceId);
	TSet<UObject*> Single = {A};
	auto SingleCopy = FDialogueGraphEditing::Duplicate(Graph, Single);
	TestEqual(TEXT("Single duplication count"), SingleCopy.Num(), 1);
	TestTrue(TEXT("Single has no edges"), SingleCopy[0]->Pins[0]->LinkedTo.IsEmpty() && SingleCopy[0]->Pins[1]->LinkedTo.IsEmpty());
	FString Error;
	TestTrue(TEXT("Duplicate graph exports"), FDialogueGraphCodec::ExportGraph(Graph, Asset, Error));
	return true;
}
#endif
