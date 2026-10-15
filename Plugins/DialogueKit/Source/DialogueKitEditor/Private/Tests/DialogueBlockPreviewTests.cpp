#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "DialogueBlockPreview.h"
#include "DialoguePreviewPolicy.h"
#include "DialogueGraph.h"
#include "DialogueGraphCodec.h"
#include "DialogueGraphValidation.h"
#include "DialogueEdGraphSchema.h"
#include "DialogueEdEndGraphNode.h"
#include "DialogueFlowEdGraphNode.h"
#include "DialogueLineNodeInfo.h"
#include "UObject/UnrealType.h"
#include "UObject/Package.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDialogueBlockPreviewCopyTest, "DialogueKit.Preview.UnsavedCopyAndConditions", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// 미저장 Line/안정 ID를 복사하고 원본과 프리셋 및 패키지 상태를 보존한다.
bool FDialogueBlockPreviewCopyTest::RunTest(const FString& Parameters)
{
    auto* Package = CreatePackage(*(TEXT("/Temp/DialoguePreview_") + FGuid::NewGuid().ToString(EGuidFormats::Digits)));
    auto* Asset = NewObject<UDialogueGraph>(Package);
    Asset->InitializeBlockIdentity();
    auto* Graph = NewObject<UEdGraph>(Asset);
    Graph->Schema = UDialogueEdGraphSchema::StaticClass();
    Graph->GetSchema()->CreateDefaultNodesForGraph(*Graph);
    FNewNodeAction LineAction(UDialogueFlowEdGraphNode::StaticClass(), FText(), FText(), FText(), 0);
    auto* Line = CastChecked<UDialogueEdGraphNodeBase>(LineAction.PerformAction(Graph, Graph->Nodes[0]->Pins[0], FVector2D::ZeroVector));
    FNewNodeAction EndAction(UDialogueEdEndGraphNode::StaticClass(), FText(), FText(), FText(), 0);
    EndAction.PerformAction(Graph, Line->Pins[1], FVector2D::ZeroVector);
    FString Error;
    TestTrue(TEXT("Original export"), FDialogueGraphCodec::ExportGraph(Graph, Asset, Error));
    auto* OriginalRuntime = Asset->Graph.Get();
    FindFProperty<FTextProperty>(UDialogueNodeInfo::StaticClass(), TEXT("DialogueText"))->SetPropertyValue_InContainer(Line->GetNodeInfo(), FText::FromString(TEXT("미저장 대사")));
    Package->SetDirtyFlag(false);
    auto* Copy = FDialogueBlockPreview::CopyWorkingBlock(Asset, Graph, Error);
    if (!TestNotNull(TEXT("Copy"), Copy)) return false;
    TestEqual(TEXT("Stable block ID"), Copy->GetBlockId(), Asset->GetBlockId());
    TestTrue(TEXT("Original graph untouched"), Asset->Graph == OriginalRuntime);
    TestFalse(TEXT("Package dirty unchanged"), Package->IsDirty());
    bool bFound = false;
    for (const auto* Node : Copy->Graph->Nodes)
    {
        if (Node->NodeGuid == Line->NodeGuid)
        {
            bFound = true;
            TestEqual(TEXT("Unsaved text copied"), CastChecked<UDialogueLineNodeInfo>(Node->NodeInfo)->GetDialogueText().ToString(), FString(TEXT("미저장 대사")));
            TestTrue(TEXT("NodeInfo separately owned"), Node->NodeInfo != Line->GetNodeInfo());
        }
    }
    TestTrue(TEXT("Line ID retained"), bFound);
    FDialoguePreviewPreset Preset;
    FDialogueStateDefinition Definition;
    Definition.Key.Kind = EDialogueStateKind::Value;
    Definition.Key.ItemId = TEXT("Level");
    Preset.Definitions.Add(Definition);
    FDialogueSampleState State;
    State.Key = Definition.Key;
    State.Value = 3;
    Preset.States.Add(State);
    auto* Provider = FDialogueBlockPreview::MakeProvider(Preset, Error);
    if (!TestNotNull(TEXT("Provider"), Provider)) return false;
    FDialogueCondition Condition;
    Condition.Key = Definition.Key;
    Condition.Comparison = EDialogueComparison::GreaterOrEqual;
    Condition.ValueOperand = 5;
    Copy->EntryConditions.Conditions.Add(Condition);
    TestFalse(TEXT("Entry blocked"), FDialogueBlockPreview::CheckEntry(Copy, Provider, Error));
    TestTrue(TEXT("Explanation includes item"), Error.Contains(TEXT("Level")));
    TestTrue(TEXT("Explanation includes false"), Error.Contains(TEXT("False")));
    State.Value = 8;
    Provider->SetSampleState(State);
    TestTrue(TEXT("Entry passes"), FDialogueBlockPreview::CheckEntry(Copy, Provider, Error));
    TestEqual(TEXT("Preset unchanged"), Preset.States[0].Value, 3.0);
    auto* Restarted = FDialogueBlockPreview::MakeProvider(Preset, Error);
    double Value = 0;
    Restarted->QueryDialogueValue_Implementation(NAME_None, TEXT("Level"), Value);
    TestEqual(TEXT("Restart restores preset"), Value, 3.0);
    Preset.Definitions.Add(Definition);
    TestNull(TEXT("Duplicate definition rejected"), FDialogueBlockPreview::MakeProvider(Preset, Error));
    Graph->bEditable = false;
    const auto* Schema = Graph->GetSchema();
    auto* Output = Line->Pins[1];
    auto* Input = Output->LinkedTo[0];
    TestEqual(TEXT("Locked connect rejected"), Schema->CanCreateConnection(Output, Input).Response, CONNECT_RESPONSE_DISALLOW);
    Schema->BreakPinLinks(*Output, true);
    Schema->BreakSinglePinLink(Output, Input);
    Schema->BreakNodeLinks(*Line);
    TestEqual(TEXT("Locked disconnection rejected"), Output->LinkedTo.Num(), 1);
    Graph->bEditable = true;
    FDialogueGraphValidation::RegisterWorkingGraph(Asset, Graph);
    UDialoguePreviewPolicy::BeginPreview();
    TestFalse(TEXT("Save guard blocks preview asset"), FDialogueGraphValidation::CanSavePackage(Package));
    UDialoguePreviewPolicy::EndPreview();
    TestTrue(TEXT("Save restored after preview"), FDialogueGraphValidation::CanSavePackage(Package));
    FDialogueGraphValidation::UnregisterWorkingGraph(Asset);
    return true;
}
#endif
