#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "DialogueGraph.h"
#include "DialogueGraphFactory.h"
#include "UObject/Package.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDialogueBlockFactoryIdentityTest,
    "DialogueKit.Contracts.BlockIdentity.Factory",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// 콘텐츠 브라우저가 사용하는 Factory가 서로 다른 신규 Block ID를 만드는지 검사한다.
bool FDialogueBlockFactoryIdentityTest::RunTest(const FString& Parameters)
{
    UDialogueGraphFactory* Factory = NewObject<UDialogueGraphFactory>();
    TSet<FGuid> Ids;
    for (int32 Index = 0; Index < 2; ++Index)
    {
        const FName Name = MakeUniqueObjectName(GetTransientPackage(), UDialogueGraph::StaticClass(), TEXT("FactoryBlock"));
        UDialogueGraph* Graph = Cast<UDialogueGraph>(Factory->FactoryCreateNew(UDialogueGraph::StaticClass(),
            GetTransientPackage(), Name, RF_Transient, nullptr, GWarn));
        if (!TestNotNull(TEXT("Factory produced graph"), Graph)) { return false; }
        TestTrue(TEXT("Factory initialized identity"), Graph->GetBlockReference().IsValid());
        TestFalse(TEXT("New blocks have distinct IDs"), Ids.Contains(Graph->GetBlockId()));
        Ids.Add(Graph->GetBlockId());
    }
    return true;
}
#endif
