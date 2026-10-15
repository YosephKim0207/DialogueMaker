#include "DialogueBlockPreview.h"
#include "DialoguePreviewPolicy.h"
#include "DialogueGraph.h"
#include "DialogueGraphCodec.h"
#include "DialogueGraphValidation.h"
#include "DialogueConditionLibrary.h"
#include "DialoguePresentationWidget.h"
#include "DialogueBlockRunner.h"
#include "Editor.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "GameMapsSettings.h"
#include "Settings/LevelEditorPlaySettings.h"
#include "PlayInEditorDataTypes.h"
#include "UObject/StrongObjectPtr.h"
#include "PropertyEditorModule.h"
#include "IDetailsView.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWindow.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "Misc/MessageDialog.h"
#include "Containers/Ticker.h"
#include "EdGraph/EdGraph.h"

namespace DialoguePreview
{
    TStrongObjectPtr<UDialoguePreviewSettings> Editing;
    TWeakPtr<SWindow> SettingsWindow;
    TStrongObjectPtr<UDialogueGraph> Block;
    TStrongObjectPtr<UDialoguePreviewStateProvider> Provider;
    TStrongObjectPtr<UDialogueBlockRunner> Runner;
    TStrongObjectPtr<UDialoguePresentationWidget> View;
    TWeakObjectPtr<UEdGraph> LockedGraph;
    bool bPreviousEditable = true;
    bool bClassOverridden = false;
    FSoftClassPath PreviousInstanceClass;
    FDelegateHandle Started, Ended, Cancelled, InstanceStarted;
    FTSTicker::FDelegateHandle Watchdog;
    double RequestedAt = 0;
    bool bAwaitingStart = false;

    // 원본 설정은 디스크 기록 없이 이전 값으로 정확히 돌린다.
    void RestoreInstanceClass()
    {
        if (!bClassOverridden) return;
        GetMutableDefault<UGameMapsSettings>()->GameInstanceClass = PreviousInstanceClass;
        bClassOverridden = false;
    }
    // 이름 있는 프리셋의 메모리 편집본을 한 번 준비한다.
    UDialoguePreviewSettings* GetEditing()
    {
        if (!Editing.IsValid())
        {
            Editing.Reset(DuplicateObject<UDialoguePreviewSettings>(GetMutableDefault<UDialoguePreviewSettings>(), GetTransientPackage()));
            if (Editing->Presets.IsEmpty()) Editing->Presets.AddDefaulted();
        }
        return Editing.Get();
    }
}

// 프리셋은 게임 저장과 다른 로컬 편집 설정이며 버튼을 누를 때만 저장한다.
void FDialogueBlockPreview::ShowSettings()
{
    using namespace DialoguePreview;
    if (auto Existing = SettingsWindow.Pin()) { Existing->BringToFront(); return; }
    auto* Settings = GetEditing();
    FDetailsViewArgs Args;
    Args.bHideSelectionTip = true;
    auto Details = FModuleManager::LoadModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor")).CreateDetailView(Args);
    Details->SetObject(Settings);
    Details->SetIsPropertyEditingEnabledDelegate(FIsPropertyEditingEnabled::CreateLambda([] { return !UDialoguePreviewPolicy::IsBlockPreviewActive(); }));
    auto Window = SNew(SWindow).Title(FText::FromString(TEXT("Block 테스트 프리셋"))).ClientSize(FVector2D(720, 650));
    SettingsWindow = Window;
    Window->SetContent(SNew(SVerticalBox)
        + SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).AutoWrapText(true).Text(FText::FromString(TEXT("Presets에서 이름·정의·현재 값을 편집하고 Selected Preset에 사용할 번호(0부터)를 지정하세요. 레벨·능력치는 Value, 아이템 수량은 Count, 진행도는 Tag입니다. 테스트는 실제 효과/저장을 실행하지 않습니다.")))]
        + SVerticalBox::Slot().FillHeight(1)[Details]
        + SVerticalBox::Slot().AutoHeight()[SNew(SButton).Text(FText::FromString(TEXT("프리셋 명시적 저장")))
            .IsEnabled_Lambda([] { return !UDialoguePreviewPolicy::IsBlockPreviewActive(); })
            .OnClicked_Lambda([]
            {
                auto* Saved = GetMutableDefault<UDialoguePreviewSettings>();
                Saved->Presets = GetEditing()->Presets;
                Saved->SelectedPreset = GetEditing()->SelectedPreset;
                Saved->SaveConfig();
                return FReply::Handled();
            })]);
    FSlateApplication::Get().AddWindow(Window);
}

// PIE 복제 모드로 안정 ID를 유지하고 현재 Graph의 미저장 내용을 복사본에만 반영한다.
UDialogueGraph* FDialogueBlockPreview::CopyWorkingBlock(UDialogueGraph* Asset, UEdGraph* Graph, FString& Error)
{
    if (!Asset || !Graph) { Error = TEXT("Block 또는 Graph가 없습니다."); return nullptr; }
    FObjectDuplicationParameters Params(Asset, GetTransientPackage());
    Params.DuplicateMode = EDuplicateMode::PIE;
    auto* Copy = Cast<UDialogueGraph>(StaticDuplicateObjectEx(Params));
    if (!Copy) { Error = TEXT("Block 복제 실패"); return nullptr; }
    Copy->ClearFlags(RF_Public | RF_Standalone | RF_Transactional);
    Copy->SetFlags(RF_Transient);
    if (!FDialogueGraphCodec::ExportGraph(Graph, Copy, Error)) return nullptr;
    return Copy;
}

// DataTable 정의와 수동 정의를 검증한 후 별도 메모리 상태를 구성한다.
UDialoguePreviewStateProvider* FDialogueBlockPreview::MakeProvider(const FDialoguePreviewPreset& Preset, FString& Error)
{
    auto* Result = NewObject<UDialoguePreviewStateProvider>();
    TArray<FDialogueStateDefinition> Definitions = Preset.Definitions;
    if (!Preset.DefinitionTable.IsNull())
    {
        TArray<FDialogueStateDefinition> TableDefinitions;
        if (!UDialogueConditionLibrary::ReadDefinitionTable(Preset.DefinitionTable.LoadSynchronous(), TableDefinitions))
        { Error = TEXT("정의 DataTable을 읽을 수 없습니다."); return nullptr; }
        Definitions.Append(TableDefinitions);
    }
    for (const auto& Definition : Definitions)
    {
        if (!Result->AddDefinition(Definition)) { Error = TEXT("정의 키가 잘못되었거나 중복입니다."); return nullptr; }
    }
    for (const auto& State : Preset.States)
    {
        if (!Result->SetSampleState(State)) { Error = TEXT("현재 값의 정의가 없거나 값이 잘못되었습니다."); return nullptr; }
    }
    return Result;
}

// 최종 AND/OR 결과와 각 항목의 현재 값/기준/판정을 함께 설명한다.
bool FDialogueBlockPreview::CheckEntry(UDialogueGraph* InBlock, UObject* InProvider, FString& Explanation)
{
    if (!InBlock || !InProvider || !InProvider->GetClass()->ImplementsInterface(UDialogueStateProvider::StaticClass()))
    { Explanation = TEXT("Block 또는 상태 제공자가 없습니다."); return false; }
    FDialogueDiagnosticContext Context;
    Context.Block = InBlock->GetBlockReference();
    const auto Result = UDialogueConditionLibrary::EvaluateConditions(InBlock->EntryConditions, InProvider, false, Context, nullptr);
    Explanation = FString::Printf(TEXT("Block 진입 판정: %s\n"), Result.bPassed ? TEXT("True") : TEXT("False"));
    for (const auto& Condition : InBlock->EntryConditions.Conditions)
    {
        FString Current;
        FString Operand;
        bool bKnown = false;
        if (Condition.Key.Kind == EDialogueStateKind::Tag)
        {
            bool Value = false;
            bKnown = IDialogueStateProvider::Execute_QueryDialogueTag(InProvider, Condition.Key.TargetId, Condition.Key.Tag, Value);
            Current = Value ? TEXT("True") : TEXT("False");
            Operand = Condition.bTagOperand ? TEXT("True") : TEXT("False");
        }
        else if (Condition.Key.Kind == EDialogueStateKind::Value)
        {
            double Value = 0;
            bKnown = IDialogueStateProvider::Execute_QueryDialogueValue(InProvider, Condition.Key.TargetId, Condition.Key.ItemId, Value);
            Current = FString::SanitizeFloat(Value);
            Operand = FString::SanitizeFloat(Condition.ValueOperand);
        }
        else
        {
            int64 Value = 0;
            bKnown = IDialogueStateProvider::Execute_QueryDialogueCount(InProvider, Condition.Key.TargetId, Condition.Key.ItemId, Value);
            Current = LexToString(Value);
            Operand = LexToString(Condition.CountOperand);
        }
        FDialogueConditionSet Single;
        Single.Conditions.Add(Condition);
        const auto Verdict = UDialogueConditionLibrary::EvaluateConditions(Single, InProvider, false, Context, nullptr);
        Explanation += FString::Printf(TEXT("%s / %s: 현재 %s, %s %s → %s%s\n"), *Condition.Key.TargetId.ToString(),
            *(Condition.Key.Kind == EDialogueStateKind::Tag ? Condition.Key.Tag.ToString() : Condition.Key.ItemId.ToString()),
            *(bKnown ? Current : TEXT("조회 실패")), *StaticEnum<EDialogueComparison>()->GetNameStringByValue(int64(Condition.Comparison)),
            *Operand, Verdict.bPassed ? TEXT("True") : TEXT("False"), Verdict.bHadErrors ? TEXT(" (오류 대체)") : TEXT(""));
    }
    return Result.bPassed;
}

// 검사와 조건 판정 후 단일 빈 맵 PIE를 예약하며 에셋/프리셋/게임 저장은 하지 않는다.
bool FDialogueBlockPreview::Start(UDialogueGraph* Asset, UEdGraph* Graph, bool bIgnoreEntry, const FDialoguePreviewPreset* PresetOverride)
{
    using namespace DialoguePreview;
    if (!GEditor || GEditor->IsPlaySessionInProgress() || UDialoguePreviewPolicy::IsBlockPreviewActive()) return false;
    const auto Issues = FDialogueGraphValidation::Inspect(Graph);
    if (!Issues.IsEmpty())
    {
        FString Message = TEXT("Graph 검사 결과:\n");
        for (const auto& Issue : Issues) Message += Issue.Message.ToString() + TEXT("\n");
        const bool bCanRun = FDialogueGraphValidation::CanSave(Issues);
        if (!bCanRun) { FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(Message)); return false; }
        if (FMessageDialog::Open(EAppMsgType::YesNo, FText::FromString(Message + TEXT("경고를 확인하고 테스트를 진행할까요?"))) != EAppReturnType::Yes) return false;
    }
    auto* Settings = GetEditing();
    if (!PresetOverride && !Settings->Presets.IsValidIndex(Settings->SelectedPreset))
    { FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(TEXT("선택한 프리셋 번호가 없습니다."))); return false; }
    FString Error;
    TStrongObjectPtr<UDialoguePreviewStateProvider> NewProvider(MakeProvider(PresetOverride ? *PresetOverride : Settings->Presets[Settings->SelectedPreset], Error));
    TStrongObjectPtr<UDialogueGraph> NewBlock(CopyWorkingBlock(Asset, Graph, Error));
    if (!NewProvider.IsValid() || !NewBlock.IsValid())
    { FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(Error)); return false; }
    if (!bIgnoreEntry && !CheckEntry(NewBlock.Get(), NewProvider.Get(), Error))
    { FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(Error)); return false; }
    if (!UDialoguePreviewPolicy::BeginPreview()) return false;
    Block.Reset(NewBlock.Get());
    Provider.Reset(NewProvider.Get());
    LockedGraph = Graph;
    bPreviousEditable = Graph->bEditable;
    Graph->bEditable = false;
    PreviousInstanceClass = GetDefault<UGameMapsSettings>()->GameInstanceClass;
    GetMutableDefault<UGameMapsSettings>()->GameInstanceClass = FSoftClassPath(UGameInstance::StaticClass());
    bClassOverridden = true;
    InstanceStarted = FWorldDelegates::OnPIEStarted.AddLambda([](UGameInstance*) { RestoreInstanceClass(); });
    Started = FEditorDelegates::PostPIEStarted.AddStatic(&FDialogueBlockPreview::OnStarted);
    Ended = FEditorDelegates::EndPIE.AddLambda([](bool) { Finish(); });
    Cancelled = FEditorDelegates::CancelPIE.AddStatic(&FDialogueBlockPreview::Finish);
    RequestedAt = FPlatformTime::Seconds();
    bAwaitingStart = true;
    Watchdog = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([](float)
    {
        if (bAwaitingStart && FPlatformTime::Seconds() - RequestedAt > 30 && GEditor)
        {
            // 온라인 로그인을 사용하지 않는 단일 PIE 생성은 같은 호출 스택에서 끝난다.
            // delegate 없이 실패한 요청도 CancelPIE를 통해 정리하며 격리를 먼저 풀지 않는다.
            if (GEditor->PlayWorld) GEditor->RequestEndPlayMap();
            else GEditor->CancelRequestPlaySession();
        }
        return true;
    }));
    auto* PlaySettings = DuplicateObject<ULevelEditorPlaySettings>(GetMutableDefault<ULevelEditorPlaySettings>(), GetTransientPackage());
    PlaySettings->SetPlayNetMode(EPlayNetMode::PIE_Standalone);
    PlaySettings->SetRunUnderOneProcess(true);
    PlaySettings->SetPlayNumberOfClients(1);
    PlaySettings->bLaunchSeparateServer = false;
    FRequestPlaySessionParams Params;
    Params.EditorPlaySettings = PlaySettings;
    Params.GlobalMapOverride = TEXT("/Engine/Maps/Entry.Entry");
    Params.GameModeOverride = AGameModeBase::StaticClass();
    Params.bAllowOnlineSubsystem = false;
    GEditor->RequestPlaySession(Params);
    return true;
}

// 게임 월드가 준비된 뒤 고정 테스트 제공자와 UI만 연결한다.
void FDialogueBlockPreview::OnStarted(bool bSimulating)
{
    using namespace DialoguePreview;
    RestoreInstanceClass();
    bAwaitingStart = false;
    UWorld* World = GEditor ? GEditor->PlayWorld : nullptr;
    APlayerController* Player = World ? World->GetFirstPlayerController() : nullptr;
    if (!Player || !Block.IsValid() || !Provider.IsValid()) { Stop(); return; }
    Runner.Reset(NewObject<UDialogueBlockRunner>(World));
    View.Reset(CreateWidget<UDialoguePresentationWidget>(Player));
    if (!View.IsValid()) { Stop(); return; }
    View->AddToViewport();
    View->BindRunner(Runner.Get());
    Runner->StartBlock(Block.Get(), Provider.Get(), Provider.Get(), nullptr);
    FInputModeUIOnly Mode;
    Mode.SetWidgetToFocus(View->TakeWidget());
    Player->SetInputMode(Mode);
    Player->bShowMouseCursor = true;
}

// 테스트 종료는 엔진의 PIE 종료 요청을 사용한다.
void FDialogueBlockPreview::Stop()
{
    if (!UDialoguePreviewPolicy::IsBlockPreviewActive()) return;
    if (GEditor && GEditor->IsPlaySessionRequestQueued()) { GEditor->CancelRequestPlaySession(); Finish(); }
    else if (GEditor && GEditor->IsPlaySessionInProgress()) GEditor->RequestEndPlayMap();
    else Finish();
}

// 성공/실패/취소에서 동일한 정리를 수행하고 원래 설정과 편집 상태를 복원한다.
void FDialogueBlockPreview::Finish()
{
    using namespace DialoguePreview;
    RestoreInstanceClass();
    FWorldDelegates::OnPIEStarted.Remove(InstanceStarted);
    FEditorDelegates::PostPIEStarted.Remove(Started);
    FEditorDelegates::EndPIE.Remove(Ended);
    FEditorDelegates::CancelPIE.Remove(Cancelled);
    FTSTicker::GetCoreTicker().RemoveTicker(Watchdog);
    Watchdog.Reset();
    if (View.IsValid()) { View->DetachPresentation(); View->RemoveFromParent(); }
    View.Reset(); Runner.Reset(); Provider.Reset(); Block.Reset();
    if (LockedGraph.IsValid()) LockedGraph->bEditable = bPreviousEditable;
    LockedGraph.Reset();
    UDialoguePreviewPolicy::EndPreview();
}

// 모듈 종료에서도 설정 override와 delegate를 남기지 않는다.
void FDialogueBlockPreview::Shutdown()
{
    Stop();
    Finish();
    if (auto Window = DialoguePreview::SettingsWindow.Pin()) Window->RequestDestroyWindow();
    DialoguePreview::Editing.Reset();
}

// 현재 테스트 실행 상태를 개발 도구와 자동 검증에서 조회한다.
UDialogueBlockRunner* FDialogueBlockPreview::GetRunner()
{
    return DialoguePreview::Runner.Get();
}
