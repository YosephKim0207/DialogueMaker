#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "DialoguePresentationWidget.h"
#include "DialogueGraph.h"
#include "DialogueGraphCodec.h"
#include "DialogueEdGraphSchema.h"
#include "DialogueEdEndGraphNode.h"
#include "DialogueFlowEdGraphNode.h"
#include "DialogueLineNodeInfo.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDialogueWidgetInputTest, "DialogueKit.Presentation.WidgetInput", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// 임시 월드의 실제 기본 Widget을 통해 펼치기와 Runner 진행이 분리됨을 검사한다.
bool FDialogueWidgetInputTest::RunTest(const FString& Parameters)
{
    auto* Asset = NewObject<UDialogueGraph>();
    Asset->InitializeBlockIdentity();
    auto* Graph = NewObject<UEdGraph>(Asset);
    Graph->Schema = UDialogueEdGraphSchema::StaticClass();
    Graph->GetSchema()->CreateDefaultNodesForGraph(*Graph);
    FNewNodeAction LineAction(UDialogueFlowEdGraphNode::StaticClass(), FText(), FText(), FText(), 0);
    auto* Line = CastChecked<UDialogueEdGraphNodeBase>(LineAction.PerformAction(Graph, Graph->Nodes[0]->Pins[0], FVector2D::ZeroVector));
    FNewNodeAction EndAction(UDialogueEdEndGraphNode::StaticClass(), FText(), FText(), FText(), 0);
    EndAction.PerformAction(Graph, Line->Pins[1], FVector2D::ZeroVector);
    CastChecked<UDialogueLineNodeInfo>(Line->GetNodeInfo())->SetTextTiming(true, 10.0f);
    FindFProperty<FTextProperty>(UDialogueNodeInfo::StaticClass(), TEXT("DialogueText"))->SetPropertyValue_InContainer(Line->GetNodeInfo(), FText::FromString(TEXT("표시 입력 검증")));
    FString Error;
    if (!TestTrue(TEXT("Export"), FDialogueGraphCodec::ExportGraph(Graph, Asset, Error))) return false;
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    auto* Runner = NewObject<UDialogueBlockRunner>(World);
    auto* View = CreateWidget<UDialoguePresentationWidget>(World);
    if (TestNotNull(TEXT("Widget created"), View))
    {
        TSharedPtr<SWidget> SlateView = View->TakeWidget();
        View->BindRunner(Runner);
        TestTrue(TEXT("Start"), Runner->StartBlock(Asset, nullptr, nullptr, nullptr));
        TestFalse(TEXT("Text initially partial"), View->GetTextPresentation()->IsTextComplete());
        TestTrue(TEXT("Input reveals"), View->SubmitPresentationAdvance());
        TestTrue(TEXT("Full text"), View->GetTextPresentation()->IsTextComplete());
        TestEqual(TEXT("Reveal does not advance"), Runner->GetPresentation().State, EDialogueRunState::WaitingLine);
        TestFalse(TEXT("Same frame cannot advance"), View->SubmitPresentationAdvance());
        Runner->CancelBlock();
        TestEqual(TEXT("Cancel hides view"), View->GetVisibility(), ESlateVisibility::Collapsed);
        View->DetachPresentation();
        TestTrue(TEXT("Detached runner can restart"), Runner->StartBlock(Asset, nullptr, nullptr, nullptr));
        TestFalse(TEXT("Detached input rejected"), View->SubmitPresentationAdvance());
        View->BindRunner(Runner);
        TestFalse(TEXT("Rebind restores partial text"), View->GetTextPresentation()->IsTextComplete());
        View->DetachPresentation();
        SlateView.Reset();
        View->ReleaseSlateResources(true);
    }
    GEngine->DestroyWorldContext(World);
    World->DestroyWorld(false);
    return true;
}
#endif
