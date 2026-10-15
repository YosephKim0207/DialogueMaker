#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "DialoguePreviewPolicy.h"
#include "DialoguePreviewTestInstance.h"
#include "DialogueSubsystem.h"
#include "PlayerProgressSubsystem.h"
#include "QuestSubsystem.h"
#include "Subsystems/SubsystemCollection.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "HAL/FileManager.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDialoguePreviewIsolationTest, "DialogueKit.Preview.PersistenceIsolation", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// 실제 슬롯은 생성/변경하지 않고 초기화부터 종료 이후까지 파일 집합과 바이트를 대조한다.
bool FDialoguePreviewIsolationTest::RunTest(const FString& Parameters)
{
    const FString Folder = FPaths::ProjectSavedDir() / TEXT("SaveGames");
    auto ReadFiles = [&]()
    {
        TMap<FString, TArray<uint8>> Result;
        TArray<FString> Files;
        IFileManager::Get().FindFilesRecursive(Files, *Folder, TEXT("*"), true, false);
        for (const FString& File : Files) FFileHelper::LoadFileToArray(Result.Add(File), *File);
        return Result;
    };
    const auto Before = ReadFiles();
    TestTrue(TEXT("Begin isolation"), UDialoguePreviewPolicy::BeginPreview());
    TestFalse(TEXT("Nested rejected"), UDialoguePreviewPolicy::BeginPreview());
    auto* World = UWorld::CreateWorld(EWorldType::PIE, false);
    auto& Context = GEngine->CreateNewWorldContext(EWorldType::PIE);
    Context.SetCurrentWorld(World);
    auto* Instance = NewObject<UDialoguePreviewTestInstance>(GEngine);
    Instance->SetTestContext(&Context);
    Context.OwningGameInstance = Instance;
    World->SetGameInstance(Instance);
    TestTrue(TEXT("PIE context isolated"), UDialoguePreviewPolicy::IsPreviewWorld(Instance));
    TestFalse(TEXT("Null context not isolated"), UDialoguePreviewPolicy::IsPreviewWorld(nullptr));
    FSubsystemCollection<UGameInstanceSubsystem> Collection;
    auto* Progress = NewObject<UPlayerProgressSubsystem>(Instance);
    auto* Quest = NewObject<UQuestSubsystem>(Instance);
    auto* Dialogue = NewObject<UDialogueSubsystem>(Instance);
    Progress->Initialize(Collection);
    Quest->Initialize(Collection);
    Dialogue->Initialize(Collection);
    Progress->SetPlayerLevel(777);
    UDialoguePreviewPolicy::EndPreview();
    Progress->SaveProgress();
    Quest->SaveProgress();
    TestEqual(TEXT("Memory state retained"), Progress->GetPlayerLevel(), 777);
    auto* Provider = NewObject<UDialoguePreviewStateProvider>();
    FDialogueStateDefinition Definition;
    Definition.Key.Kind = EDialogueStateKind::Count;
    Definition.Key.ItemId = TEXT("TestItem");
    Provider->AddDefinition(Definition);
    FDialogueEffectBatch Batch;
    Batch.Context.ExecutionId = FGuid::NewGuid();
    FDialogueEffect Effect;
    Effect.Key = Definition.Key;
    Effect.Count = 10;
    Batch.Effects.Add(Effect);
    TestTrue(TEXT("Preview effect allows flow"), Provider->ApplyDialogueEffects_Implementation(Batch).IsSuccess());
    int64 Count = -1;
    TestTrue(TEXT("Query fixed state"), Provider->QueryDialogueCount_Implementation(NAME_None, TEXT("TestItem"), Count));
    TestEqual(TEXT("Effect not applied"), Count, int64(0));
    const auto After = ReadFiles();
    TestEqual(TEXT("No save file created"), After.Num(), Before.Num());
    for (const auto& Pair : Before)
    {
        const auto* Bytes = After.Find(Pair.Key);
        TestTrue(TEXT("Existing save unchanged"), Bytes && *Bytes == Pair.Value);
    }
    Progress->Deinitialize();
    Quest->Deinitialize();
    Dialogue->Deinitialize();
    Instance->SetTestContext(nullptr);
    GEngine->DestroyWorldContext(World);
    World->DestroyWorld(false);
    return true;
}
#endif
