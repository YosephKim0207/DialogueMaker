#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "DialogueLineAuthoring.h"
#include "DialogueFlowEdGraphNode.h"
#include "DialogueBranchEdGraphNode.h"
#include "DialogueLineNodeInfo.h"
#include "DialogueGraph.h"
#include "DialogueGraphCodec.h"
#include "Editor.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDialogueLineAuthoringTest, "DialogueKit.Editor.LineAuthoring", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// 상속은 새 ID/본문을 유지하고 연결 교체와 시간 일괄 편집은 Undo 한 번으로 복원된다.
bool FDialogueLineAuthoringTest::RunTest(const FString& Parameters)
{
	auto* Block = NewObject<UDialogueGraph>(GetTransientPackage(), NAME_None, RF_Transactional);
	Block->InitializeBlockIdentity();
	auto* Graph = NewObject<UEdGraph>(Block, NAME_None, RF_Transactional);
	Graph->Schema = UDialogueEdGraphSchema::StaticClass();
	Graph->GetSchema()->CreateDefaultNodesForGraph(*Graph);
	FNewNodeAction Add(UDialogueFlowEdGraphNode::StaticClass(), FText(), FText(), FText(), 0);
	auto* A = CastChecked<UDialogueFlowEdGraphNode>(Add.PerformAction(Graph, Graph->Nodes[0]->Pins[0], FVector2D::ZeroVector));
	auto* B = Add.PerformAction(Graph, A->Pins[1], FVector2D(400, 0));
	auto* Info = CastChecked<UDialogueLineNodeInfo>(A->GetNodeInfo());
	Info->SetTextTiming(false, 0.1f);
	Info->PortraitOffset = FVector2D(12, 45);
	Info->bMirrorPortrait = true;
	Info->SetShownCondition(true);
	Info->AdvanceReadVersion();
	TestEqual(TEXT("Nearest source is current Line"), FDialogueLineAuthoring::FindNearestLines(A->Pins[1]).Num(), 1);
	auto* New = FDialogueLineAuthoring::CreateContinuation(Graph, A->Pins[1], A, FVector2D(200, 0));
	if (TestNotNull(TEXT("Continuation created"), New) == false) return false;
	auto* NewInfo = CastChecked<UDialogueLineNodeInfo>(New->GetNodeInfo());
	TestTrue(TEXT("New identity"), New->NodeGuid != A->NodeGuid);
	TestEqual(TEXT("Timing copied"), NewInfo->SecondsPerCharacter, 0.1f);
	TestEqual(TEXT("Offset copied"), NewInfo->PortraitOffset, Info->PortraitOffset);
	TestTrue(TEXT("Direction copied"), NewInfo->bMirrorPortrait);
	TestEqual(TEXT("Read version not copied"), NewInfo->GetReadVersion(), 1);
	TestFalse(TEXT("Read flag not copied"), NewInfo->IsDialogueAlreadyShown());
	TestTrue(TEXT("Existing destination preserved unlinked"), Graph->Nodes.Contains(B) && B->Pins[0]->LinkedTo.IsEmpty());
	TestTrue(TEXT("Undo creation"), GEditor->UndoTransaction());
	TestFalse(TEXT("Created node removed"), Graph->Nodes.Contains(New));
	TestTrue(TEXT("Old connection restored"), A->Pins[1]->LinkedTo.Contains(B->Pins[0]));
	TestTrue(TEXT("Apply whole Line timing"), FDialogueLineAuthoring::ApplyTiming(Graph, true, 2.0f));
	TestEqual(TEXT("Whole duration set"), Info->LineDuration, 2.0f);
	TestEqual(TEXT("Character duration cleared"), Info->SecondsPerCharacter, 0.0f);
	TestTrue(TEXT("Undo batch"), GEditor->UndoTransaction());
	TestEqual(TEXT("Original timing restored"), Info->SecondsPerCharacter, 0.1f);
	Graph->bEditable = false;
	TestFalse(TEXT("Read-only batch rejected"), FDialogueLineAuthoring::ApplyTiming(Graph, true, 3.0f));
	Graph->bEditable = true;
	TestFalse(TEXT("Negative batch rejected"), FDialogueLineAuthoring::ApplyTiming(Graph, true, -1.0f));
	const int32 BeforeCount = Graph->Nodes.Num();
	TestNull(TEXT("Wrong source rejected"), FDialogueLineAuthoring::CreateContinuation(Graph, A->Pins[1], CastChecked<UDialogueFlowEdGraphNode>(B), FVector2D::ZeroVector));
	TestNull(TEXT("Input pin rejected"), FDialogueLineAuthoring::CreateContinuation(Graph, A->Pins[0], A, FVector2D::ZeroVector));
	TestEqual(TEXT("Rejected request does not create node"), Graph->Nodes.Num(), BeforeCount);
	TestTrue(TEXT("Rejected request preserves connection"), A->Pins[1]->LinkedTo.Contains(B->Pins[0]));
	FString Error;
	TestTrue(TEXT("Authoring exports"), FDialogueGraphCodec::ExportGraph(Graph, Block, Error));
	auto* Imported = NewObject<UEdGraph>(Block);
	Imported->Schema = UDialogueEdGraphSchema::StaticClass();
	TestTrue(TEXT("Authoring imports"), FDialogueGraphCodec::ImportGraph(Block, Imported, Error));
	for (UEdGraphNode* Node : Imported->Nodes)
	{
		if (Node->NodeGuid == A->NodeGuid)
		{
			auto* Restored = CastChecked<UDialogueLineNodeInfo>(CastChecked<UDialogueFlowEdGraphNode>(Node)->GetNodeInfo());
			TestEqual(TEXT("Timing persists"), Restored->SecondsPerCharacter, 0.1f);
			TestEqual(TEXT("Portrait placement persists"), Restored->PortraitOffset, Info->PortraitOffset);
		}
	}
	return true;
}
#endif
