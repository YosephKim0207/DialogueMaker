#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "DialogueGraph.h"
#include "Serialization/ObjectReader.h"
#include "Serialization/ObjectWriter.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"
#include "UObject/Package.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDialogueBlockIdentityLifecycleTest,
    "DialogueKit.Contracts.BlockIdentity.Lifecycle",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// 이름 변경과 복제의 실제 UObject 경로에서 정체성 보존/분리를 확인한다.
bool FDialogueBlockIdentityLifecycleTest::RunTest(const FString& Parameters)
{
    UDialogueGraph* Defaults = GetMutableDefault<UDialogueGraph>();
    TestFalse(TEXT("CDO cannot acquire identity"), Defaults->InitializeBlockIdentity());
    TestFalse(TEXT("CDO ID remains invalid"), Defaults->GetBlockId().IsValid());
    UDialogueGraph* Source = NewObject<UDialogueGraph>();
    TestFalse(TEXT("Legacy/missing identity stays invalid on lookup"), Source->GetBlockReference().IsValid());
    TestTrue(TEXT("Explicit initialization succeeds once"), Source->InitializeBlockIdentity());
    const FDialogueBlockReference Reference = Source->GetBlockReference();
    TestTrue(TEXT("New identity is valid"), Reference.IsValid());
    TestEqual(TEXT("Initial content version"), Reference.ContentVersion, 1);
    TestFalse(TEXT("Reinitialization is rejected"), Source->InitializeBlockIdentity());
    TestTrue(TEXT("Identity is stable"), Source->GetBlockReference() == Reference);
    const FName NewName = MakeUniqueObjectName(GetTransientPackage(), UDialogueGraph::StaticClass(), TEXT("RenamedBlock"));
    TestTrue(TEXT("Rename succeeds"), Source->Rename(*NewName.ToString(), nullptr, REN_DontCreateRedirectors | REN_DoNotDirty));
    TestTrue(TEXT("Rename preserves stable reference"), Source->GetBlockReference() == Reference);
    TestEqual(TEXT("Legacy PrimaryAssetId still uses asset name"), Source->GetPrimaryAssetId().PrimaryAssetName, NewName);

    for (EDuplicateMode::Type Mode : {EDuplicateMode::Normal, EDuplicateMode::World, EDuplicateMode::PIE})
    {
        UDialogueGraph* Copy = Cast<UDialogueGraph>(StaticDuplicateObject(Source, GetTransientPackage(), NAME_None,
            RF_AllFlags, nullptr, Mode));
        if (!TestNotNull(TEXT("Duplicate exists"), Copy)) { return false; }
        TestTrue(TEXT("Duplicate reference valid"), Copy->GetBlockReference().IsValid());
        TestEqual(TEXT("Only Normal gets a new identity"), Copy->GetBlockReference() == Reference, Mode != EDuplicateMode::Normal);
        TestTrue(TEXT("Duplicate did not mutate original"), Source->GetBlockReference() == Reference);
    }
    UDialogueGraph* Legacy = NewObject<UDialogueGraph>();
    UDialogueGraph* LegacyCopy = DuplicateObject<UDialogueGraph>(Legacy, GetTransientPackage());
    TestTrue(TEXT("Duplicating legacy produces a new authored block"), LegacyCopy->GetBlockReference().IsValid());
    TestFalse(TEXT("Legacy original remains unchanged"), Legacy->GetBlockReference().IsValid());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDialogueBlockIdentityArchiveTest,
    "DialogueKit.Contracts.BlockIdentity.SerializationAndCatalog",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// UPROPERTY 및 참조 구조체 직렬화 왕복과 정확한 ID/버전 카탈로그 중복을 검사한다.
bool FDialogueBlockIdentityArchiveTest::RunTest(const FString& Parameters)
{
    UDialogueGraph* Source = NewObject<UDialogueGraph>();
    Source->InitializeBlockIdentity();
    TArray<uint8> Bytes;
    FObjectWriter Writer(Source, Bytes);
    UDialogueGraph* Restored = NewObject<UDialogueGraph>();
    FObjectReader Reader(Restored, Bytes);
    TestFalse(TEXT("Object archive read succeeded"), Reader.IsError());
    TestTrue(TEXT("Reflected private identity roundtrips"), Restored->GetBlockReference() == Source->GetBlockReference());

    FDialogueBlockReference VersionTwo = Source->GetBlockReference();
    VersionTwo.ContentVersion = 2;
    Bytes.Reset();
    FMemoryWriter StructWriter(Bytes);
    FDialogueBlockReference::StaticStruct()->SerializeItem(StructWriter, &VersionTwo, nullptr);
    FDialogueBlockReference Loaded;
    FMemoryReader StructReader(Bytes);
    FDialogueBlockReference::StaticStruct()->SerializeItem(StructReader, &Loaded, nullptr);
    TestTrue(TEXT("Reference DTO preserves explicit version"), Loaded == VersionTwo);
    TArray<int32> Invalid, Duplicates;
    const TArray<FDialogueBlockReference> DistinctVersions = {Source->GetBlockReference(), VersionTwo};
    TestTrue(TEXT("Same Block different versions are allowed"), FDialogueBlockIdentity::ValidateCatalog(DistinctVersions, Invalid, Duplicates));
    FDialogueBlockReference InvalidVersion = VersionTwo;
    InvalidVersion.ContentVersion = 0;
    const TArray<FDialogueBlockReference> Catalog = {Source->GetBlockReference(), VersionTwo, VersionTwo, {}, InvalidVersion};
    TestFalse(TEXT("Invalid/duplicate catalog rejected"), FDialogueBlockIdentity::ValidateCatalog(Catalog, Invalid, Duplicates));
    TestEqual(TEXT("Invalid indices"), Invalid.Num(), 2);
    TestTrue(TEXT("Missing and zero version are invalid"), Invalid.Contains(3) && Invalid.Contains(4));
    TestEqual(TEXT("Exact duplicate indices"), Duplicates.Num(), 1);
    TestTrue(TEXT("Only second occurrence reported"), Duplicates.Contains(2));
    TestTrue(TEXT("Result arrays reset each call"), FDialogueBlockIdentity::ValidateCatalog(DistinctVersions, Invalid, Duplicates));
    TestTrue(TEXT("No stale diagnostics"), Invalid.IsEmpty() && Duplicates.IsEmpty());
    return true;
}
#endif
