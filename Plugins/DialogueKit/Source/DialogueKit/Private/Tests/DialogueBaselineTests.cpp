// 기존 대화 동작의 회귀 기준이며 신규 체크포인트 정책의 완료를 뜻하지 않는다.
#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "NativeGameplayTags.h"
#include "Kismet/GameplayStatics.h"
#include "ShownDialogueSaveData.h"
#include "Struct/DialogueStructure.h"

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_DialogueAutomation_Eligible, "DialogueKit.Automation.Eligible");

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDialogueChoiceBaselineTest,
    "DialogueKit.Baseline.ChoiceConditions",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// 빈 조건, 레벨 경계, 태그 조건이 실제 선택지 표시 판정에 반영되는지 검사한다.
bool FDialogueChoiceBaselineTest::RunTest(const FString& Parameters)
{
    FDialogueChoice Choice;
    FPlayerCondition Player;
    TestTrue(TEXT("Empty conditions allow the choice"), Choice.IsPossibleToShow(Player));
    Choice.SelectableChoiceEvalCriteria.RequiredLevel = 3;
    Player.PlayerLevel = 2;
    TestFalse(TEXT("Below required level is hidden"), Choice.IsPossibleToShow(Player));
    Player.PlayerLevel = 3;
    TestTrue(TEXT("Exact required level is allowed"), Choice.IsPossibleToShow(Player));
    Choice.SelectableChoiceEvalCriteria.RequiredTagQuery =
        FGameplayTagQuery::MakeQuery_MatchTag(TAG_DialogueAutomation_Eligible);
    TestFalse(TEXT("Missing required tag is hidden"), Choice.IsPossibleToShow(Player));
    Player.PlayerOwnedTags.AddTag(TAG_DialogueAutomation_Eligible);
    TestTrue(TEXT("Level and tag satisfied"), Choice.IsPossibleToShow(Player));
    Player.PlayerLevel = 2;
    TestFalse(TEXT("Tag does not bypass level"), Choice.IsPossibleToShow(Player));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDialogueHistoryMemoryTest,
    "DialogueKit.Baseline.HistoryMemoryRoundTrip",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// Graph별 이력 격리와 덮어쓰기 의미가 SaveGame 직렬화 뒤에도 유지되는지 검사한다.
bool FDialogueHistoryMemoryTest::RunTest(const FString& Parameters)
{
    UShownDialogueSaveData* Source = NewObject<UShownDialogueSaveData>();
    const FPrimaryAssetId GraphA(FName(TEXT("Dialogue")), FName(TEXT("AutomationA")));
    const FPrimaryAssetId GraphB(FName(TEXT("Dialogue")), FName(TEXT("AutomationB")));
    const FGuid NodeA(1, 2, 3, 4);
    const FGuid NodeB(5, 6, 7, 8);
    const FGuid NodeC(9, 10, 11, 12);
    FGuidList A;
    A.Add(NodeA);
    A.Add(NodeB);
    Source->SetShownDialogues(GraphA, A);
    A.Reset();
    A.Add(NodeC);
    Source->SetShownDialogues(GraphB, A);

    TArray<uint8> Bytes;
    if (!TestTrue(TEXT("Serialize history"), UGameplayStatics::SaveGameToMemory(Source, Bytes)))
    {
        return false;
    }
    UShownDialogueSaveData* Restored = Cast<UShownDialogueSaveData>(UGameplayStatics::LoadGameFromMemory(Bytes));
    if (!TestNotNull(TEXT("Deserialize history type"), Restored))
    {
        return false;
    }
    FGuidList ActualA;
    Restored->GetShownDialogues(GraphA, ActualA);
    TestEqual(TEXT("Graph A keeps both nodes"), ActualA.Guids.Num(), 2);
    TestTrue(TEXT("Graph A contains A"), ActualA.Contains(NodeA));
    TestTrue(TEXT("Graph A contains B"), ActualA.Contains(NodeB));
    TestFalse(TEXT("Graph A excludes C"), ActualA.Contains(NodeC));
    FGuidList ActualB;
    Restored->GetShownDialogues(GraphB, ActualB);
    TestEqual(TEXT("Graph B has only its own node"), ActualB.Guids.Num(), 1);
    TestTrue(TEXT("Graph B contains C"), ActualB.Contains(NodeC));
    ActualA.Reset();
    ActualA.Add(NodeC);
    Restored->SetShownDialogues(GraphA, ActualA);
    FGuidList Replaced;
    Restored->GetShownDialogues(GraphA, Replaced);
    TestEqual(TEXT("Set replaces rather than appends"), Replaced.Guids.Num(), 1);
    TestTrue(TEXT("Replacement is visible"), Replaced.Contains(NodeC));
    FGuidList Original;
    Source->GetShownDialogues(GraphA, Original);
    TestTrue(TEXT("Restored object does not mutate source"), Original.Contains(NodeA));
    TestFalse(TEXT("Source excludes restored-only node"), Original.Contains(NodeC));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDialogueHistoryFileTest,
    "DialogueKit.Baseline.HistoryIsolatedFileRoundTrip",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// 실제 사용자 슬롯을 사용하지 않고 실행별 고유 슬롯의 저장·복원·정리를 확인한다.
bool FDialogueHistoryFileTest::RunTest(const FString& Parameters)
{
    const FString Slot = TEXT("DialogueKit_Automation_") + FGuid::NewGuid().ToString(EGuidFormats::Digits);
    constexpr int32 UserIndex = 0;
    if (!TestFalse(TEXT("Unique test slot does not exist"), UGameplayStatics::DoesSaveGameExist(Slot, UserIndex)))
    {
        return false; // 우연히 존재하는 슬롯은 테스트 소유가 아니므로 삭제하지 않는다.
    }
    bool bNeedsCleanup = true;
    bool bFileWritten = false;
    ON_SCOPE_EXIT
    {
        if (bNeedsCleanup)
        {
            // 부분 저장 실패도 정리하되 테스트 소유 슬롯 이외에는 접근하지 않는다.
            const bool bDeleted = UGameplayStatics::DeleteGameInSlot(Slot, UserIndex);
            if (bFileWritten) { TestTrue(TEXT("Clean up the test-owned slot"), bDeleted); }
        }
    };
    UShownDialogueSaveData* Source = NewObject<UShownDialogueSaveData>();
    const FPrimaryAssetId Graph(FName(TEXT("Dialogue")), FName(TEXT("AutomationFile")));
    const FGuid Node(13, 14, 15, 16);
    FGuidList Nodes;
    Nodes.Add(Node);
    Source->SetShownDialogues(Graph, Nodes);
    bFileWritten = UGameplayStatics::SaveGameToSlot(Source, Slot, UserIndex);
    if (!TestTrue(TEXT("Write unique test slot"), bFileWritten))
    {
        return false;
    }
    UShownDialogueSaveData* Loaded = Cast<UShownDialogueSaveData>(UGameplayStatics::LoadGameFromSlot(Slot, UserIndex));
    if (!TestNotNull(TEXT("Load saved history"), Loaded))
    {
        return false;
    }
    FGuidList Actual;
    Loaded->GetShownDialogues(Graph, Actual);
    TestEqual(TEXT("File keeps one node"), Actual.Guids.Num(), 1);
    TestTrue(TEXT("File keeps the exact node ID"), Actual.Contains(Node));
    const bool bDeleted = UGameplayStatics::DeleteGameInSlot(Slot, UserIndex);
    TestTrue(TEXT("Delete the exact test slot"), bDeleted);
    bNeedsCleanup = !bDeleted;
    TestFalse(TEXT("Test leaves no save file"), UGameplayStatics::DoesSaveGameExist(Slot, UserIndex));
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
