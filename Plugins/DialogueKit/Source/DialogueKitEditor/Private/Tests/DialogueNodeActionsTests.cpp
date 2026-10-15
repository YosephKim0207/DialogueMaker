#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "DialogueFlowEdGraphNode.h"
#include "DialogueChoiceEdGraphNode.h"
#include "DialogueStateChangeEdGraphNode.h"
#include "DialogueEdEndGraphNode.h"
#include "DialogueEdGraphSchema.h"
#include "DialogueGraphCodec.h"
#include "DialogueGraph.h"
#include "Editor.h"
#include "ToolMenus.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDialogueNodeActionsTest, "DialogueKit.Editor.NodeDeleteUndo", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// 세 종류 노드의 삭제·연결 해제·Undo/Redo와 메뉴의 노드/핀 구분을 검증한다.
bool FDialogueNodeActionsTest::RunTest(const FString& Parameters)
{
	for (UClass* Type : {UDialogueFlowEdGraphNode::StaticClass(), UDialogueChoiceEdGraphNode::StaticClass(), UDialogueStateChangeEdGraphNode::StaticClass()})
	{
		auto* Block = NewObject<UDialogueGraph>(GetTransientPackage(), NAME_None, RF_Transactional);
		Block->InitializeBlockIdentity();
		auto* Graph = NewObject<UEdGraph>(Block, NAME_None, RF_Transactional);
		Graph->Schema = UDialogueEdGraphSchema::StaticClass();
		Graph->GetSchema()->CreateDefaultNodesForGraph(*Graph);
		FNewNodeAction Add(Type, FText(), FText(), FText(), 0);
		auto* Node = CastChecked<UDialogueFlowEdGraphNode>(Add.PerformAction(Graph, Graph->Nodes[0]->Pins[0], FVector2D::ZeroVector));
		FNewNodeAction AddEnd(UDialogueEdEndGraphNode::StaticClass(), FText(), FText(), FText(), 0);
		auto* End = AddEnd.PerformAction(Graph, Node->Pins[1], FVector2D::ZeroVector);
		UEdGraphPin* Previous = Graph->Nodes[0]->Pins[0];
		const FGuid NodeId = Node->NodeGuid;
		const FGuid InputId = Node->Pins[0]->PinId;
		auto* Context = NewObject<UGraphNodeContextMenuContext>();
		Context->Init(Graph, Node, nullptr, false);
		auto* Menu = NewObject<UToolMenu>();
		Node->GetNodeContextMenuActions(Menu, Context);
		TestNotNull(TEXT("Node menu contains delete action"), Menu->FindSection(TEXT("DialogueFlowActions")));
		auto* PinMenu = NewObject<UToolMenu>();
		Context->Init(Graph, Node, Node->Pins[0], false);
		Node->GetNodeContextMenuActions(PinMenu, Context);
		TestNull(TEXT("Pin menu does not delete whole node"), PinMenu->FindSection(TEXT("DialogueFlowActions")));
		Graph->bEditable = false;
		TestFalse(TEXT("Read-only deletion refused"), Node->DeleteFromGraph());
		Graph->bEditable = true;
		TestTrue(TEXT("Delete node"), Node->DeleteFromGraph());
		TestFalse(TEXT("Node removed"), Graph->Nodes.Contains(Node));
		TestTrue(TEXT("Both neighboring links removed"), Previous->LinkedTo.IsEmpty() && End->Pins[0]->LinkedTo.IsEmpty());
		TestFalse(TEXT("Stale menu cannot delete twice"), Node->DeleteFromGraph());
		if (TestTrue(TEXT("Undo transaction"), GEditor->UndoTransaction()) == false) return false;
		TestTrue(TEXT("Node restored"), Graph->Nodes.Contains(Node));
		TestEqual(TEXT("Stable node ID restored"), Node->NodeGuid, NodeId);
		TestEqual(TEXT("Stable pin ID restored"), Node->Pins[0]->PinId, InputId);
		TestTrue(TEXT("Incoming link restored"), Previous->LinkedTo.Contains(Node->Pins[0]));
		TestTrue(TEXT("Outgoing link restored"), End->Pins[0]->LinkedTo.Contains(Node->Pins[1]));
		FString Error;
		TestTrue(TEXT("Restored graph exports"), FDialogueGraphCodec::ExportGraph(Graph, Block, Error));
		TestTrue(TEXT("Redo transaction"), GEditor->RedoTransaction());
		TestFalse(TEXT("Redo removes node"), Graph->Nodes.Contains(Node));
		TestTrue(TEXT("Redo removes both links"), Previous->LinkedTo.IsEmpty() && End->Pins[0]->LinkedTo.IsEmpty());
	}
	return true;
}
#endif
