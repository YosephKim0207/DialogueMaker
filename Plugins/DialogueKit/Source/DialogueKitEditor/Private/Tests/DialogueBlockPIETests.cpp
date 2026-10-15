#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "DialogueBlockPreview.h"
#include "DialoguePreviewPolicy.h"
#include "DialogueGraph.h"
#include "DialogueBlockRunner.h"
#include "DialogueEdGraphSchema.h"
#include "DialogueEdEndGraphNode.h"
#include "DialogueFlowEdGraphNode.h"
#include "Editor.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "GameMapsSettings.h"
#include "UObject/StrongObjectPtr.h"

// 실제 PIE 예약/시작/종료를 별도 테스트로 분리한다. 렌더링과 음향 관찰을 대신하지 않는다.
struct FDialoguePIEFixture
{
    TStrongObjectPtr<UDialogueGraph> Asset;
    TStrongObjectPtr<UEdGraph> Graph;
    FSoftClassPath OriginalClass;
    double Started = 0;
    bool bStopping = false;
};

class FDialogueWaitPIE : public IAutomationLatentCommand
{
public:
    FDialogueWaitPIE(FAutomationTestBase* InTest, TSharedRef<FDialoguePIEFixture> InData) : Test(InTest), Data(InData) {}
    bool Update() override
    {
        if (FPlatformTime::Seconds() - Data->Started > 30)
        {
            Test->AddError(TEXT("Block PIE lifecycle timed out"));
            FDialogueBlockPreview::Shutdown();
            return true;
        }
        if (!Data->bStopping)
        {
            auto* Runner = FDialogueBlockPreview::GetRunner();
            if (!Runner) return false;
            Test->TestTrue(TEXT("Real PIE world"), GEditor->PlayWorld && GEditor->PlayWorld->WorldType == EWorldType::PIE);
            Test->TestTrue(TEXT("Base game instance"), GEditor->PlayWorld->GetGameInstance()->GetClass() == UGameInstance::StaticClass());
            Test->TestEqual(TEXT("Settings restored after GI creation"), GetDefault<UGameMapsSettings>()->GameInstanceClass, Data->OriginalClass);
            Test->TestEqual(TEXT("Line displayed"), Runner->GetPresentation().State, EDialogueRunState::WaitingLine);
            Test->TestFalse(TEXT("Graph locked"), Data->Graph->bEditable);
            Runner->AdvanceLine();
            Test->TestEqual(TEXT("End reached"), Runner->GetPresentation().State, EDialogueRunState::Completed);
            FDialogueBlockPreview::Stop();
            Data->bStopping = true;
            return false;
        }
        if (GEditor->IsPlaySessionInProgress() || UDialoguePreviewPolicy::IsBlockPreviewActive()) return false;
        Test->TestTrue(TEXT("Graph unlocked"), Data->Graph->bEditable);
        Test->TestEqual(TEXT("Settings retained"), GetDefault<UGameMapsSettings>()->GameInstanceClass, Data->OriginalClass);
        return true;
    }
private:
    FAutomationTestBase* Test;
    TSharedRef<FDialoguePIEFixture> Data;
};

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDialoguePIELifecycleTest, "ProjectPreview.BlockPIELifecycle", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// 저장되지 않은 임시 Block 하나로 도구의 실제 엔진 실행 경로를 사용한다.
bool FDialoguePIELifecycleTest::RunTest(const FString& Parameters)
{
    auto Data = MakeShared<FDialoguePIEFixture>();
    Data->Asset.Reset(NewObject<UDialogueGraph>());
    Data->Asset->InitializeBlockIdentity();
    Data->Graph.Reset(NewObject<UEdGraph>(Data->Asset.Get()));
    Data->Graph->Schema = UDialogueEdGraphSchema::StaticClass();
    Data->Graph->GetSchema()->CreateDefaultNodesForGraph(*Data->Graph.Get());
    FNewNodeAction LineAction(UDialogueFlowEdGraphNode::StaticClass(), FText(), FText(), FText(), 0);
    auto* Line = CastChecked<UDialogueEdGraphNodeBase>(LineAction.PerformAction(Data->Graph.Get(), Data->Graph->Nodes[0]->Pins[0], FVector2D::ZeroVector));
    FNewNodeAction EndAction(UDialogueEdEndGraphNode::StaticClass(), FText(), FText(), FText(), 0);
    EndAction.PerformAction(Data->Graph.Get(), Line->Pins[1], FVector2D::ZeroVector);
    Data->OriginalClass = GetDefault<UGameMapsSettings>()->GameInstanceClass;
    Data->Started = FPlatformTime::Seconds();
    FDialoguePreviewPreset Preset;
    if (!TestTrue(TEXT("PIE requested"), FDialogueBlockPreview::Start(Data->Asset.Get(), Data->Graph.Get(), false, &Preset))) return false;
    TestTrue(TEXT("Request queued"), GEditor->IsPlaySessionRequestQueued());
    FDialogueBlockPreview::Stop();
    TestFalse(TEXT("Pending cancellation clears isolation"), UDialoguePreviewPolicy::IsBlockPreviewActive());
    TestFalse(TEXT("Pending cancellation clears request"), GEditor->IsPlaySessionRequestQueued());
    TestTrue(TEXT("Pending cancellation unlocks graph"), Data->Graph->bEditable);
    TestEqual(TEXT("Pending cancellation restores settings"), GetDefault<UGameMapsSettings>()->GameInstanceClass, Data->OriginalClass);
    if (!TestTrue(TEXT("PIE can restart after cancel"), FDialogueBlockPreview::Start(Data->Asset.Get(), Data->Graph.Get(), false, &Preset))) return false;
    ADD_LATENT_AUTOMATION_COMMAND(FDialogueWaitPIE(this, Data));
    return true;
}
#endif
