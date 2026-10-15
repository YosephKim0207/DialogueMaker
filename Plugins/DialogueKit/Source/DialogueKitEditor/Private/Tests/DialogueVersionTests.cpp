#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "DialogueVersionLibrary.h"
#include "DialogueBlockLoader.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/Package.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDialogueExactVersionTest, "DialogueKit.Restore.ExactVersionCatalog", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// 신규 후보 조건과 무관하게 구버전/복원 전용을 찾고 모호한 ID는 거부한다.
bool FDialogueExactVersionTest::RunTest(const FString& Parameters)
{
	const FString Root = TEXT("/Game/DialogueVersionTest_") + FGuid::NewGuid().ToString(EGuidFormats::Digits);
	UPackage* Package = CreatePackage(*(Root + TEXT("/Old")));
	UDialogueGraph* Old = NewObject<UDialogueGraph>(Package, TEXT("Old"), RF_Public | RF_Standalone);
	Old->InitializeBlockIdentity();
	Old->Availability = EDialogueBlockAvailability::RestoreOnly;
	FAssetRegistryModule::AssetCreated(Old);
	FName Error;
	UDialogueGraph* New = UDialogueVersionLibrary::CreateContentVersion(Old, Root + TEXT("/New"), TEXT("New"), 2, Error);
	if (TestNotNull(TEXT("New version created"), New) == false) return false;
	TestEqual(TEXT("Stable identity"), New->GetBlockId(), Old->GetBlockId());
	TestEqual(TEXT("Separate version"), New->GetBlockReference().ContentVersion, 2);
	TestEqual(TEXT("Original untouched"), Old->GetBlockReference().ContentVersion, 1);
	TestEqual(TEXT("Original availability preserved"), Old->Availability, EDialogueBlockAvailability::RestoreOnly);
	FSoftObjectPath Found;
	TestTrue(TEXT("Find old without target or scope"), UDialogueBlockLoader::FindExactReference(Old->GetBlockReference(), Found, Error));
	TestEqual(TEXT("Exact old path"), Found, FSoftObjectPath(Old));
	TestTrue(TEXT("Find new"), UDialogueBlockLoader::FindExactReference(New->GetBlockReference(), Found, Error));
	TestEqual(TEXT("Exact new path"), Found, FSoftObjectPath(New));
	auto* Loader = NewObject<UDialogueBlockLoader>();
	Loader->LoadOverride.BindLambda([&](FGuid Request, const FSoftObjectPath& Path)
	{
		return Loader->CompleteRequest(Request, Path == FSoftObjectPath(Old) ? Old : New);
	});
	Loader->RequestExactReference(Old->GetBlockReference(), nullptr);
	TestTrue(TEXT("RestoreOnly loads"), Loader->GetLastResult().bLoaded);
	TestEqual(TEXT("No latest substitution"), Loader->GetLastResult().Block.Get(), Old);
	TestFalse(TEXT("Restoration not candidate completion"), Loader->RecordBlockCompleted(Loader->GetLastResult().RequestId, Old->GetBlockReference()));
	FDialogueBlockReference Missing = Old->GetBlockReference();
	Missing.ContentVersion = 99;
	Loader->RequestExactReference(Missing, nullptr);
	TestEqual(TEXT("Missing version is error"), Loader->GetLastResult().ErrorCode, FName(TEXT("RestoreBlockNotFound")));
	TestFalse(TEXT("No greeting fallback for restore"), Loader->GetLastResult().bLoaded);
	TestNull(TEXT("Destination collision refused"), UDialogueVersionLibrary::CreateContentVersion(Old, Root + TEXT("/New"), TEXT("New"), 3, Error));
	UPackage* DuplicatePackage = CreatePackage(*(Root + TEXT("/Duplicate")));
	UDialogueGraph* Duplicate = DuplicateObject<UDialogueGraph>(Old, DuplicatePackage, TEXT("Duplicate"));
	Duplicate->SetFlags(RF_Public | RF_Standalone);
	// 손상 카탈로그 fixture만 reflection으로 중복 식별자를 주입한다.
	*FindFProperty<FStructProperty>(UDialogueGraph::StaticClass(), TEXT("BlockId"))->ContainerPtrToValuePtr<FGuid>(Duplicate) = Old->GetBlockId();
	FindFProperty<FIntProperty>(UDialogueGraph::StaticClass(), TEXT("ContentVersion"))->SetPropertyValue_InContainer(Duplicate, 2);
	FAssetRegistryModule::AssetCreated(Duplicate);
	TestFalse(TEXT("Duplicate version refused"), UDialogueBlockLoader::FindExactReference(New->GetBlockReference(), Found, Error));
	TestEqual(TEXT("Ambiguity returned"), Error, FName(TEXT("AmbiguousRestoreReference")));
	Loader->EndSession();
	// 실제 디스크 에셋은 만들지 않았으며 이번 테스트가 등록한 객체만 색인에서 제거한다.
	for (UDialogueGraph* Asset : {Old, New, Duplicate})
	{
		FAssetRegistryModule::AssetDeleted(Asset);
		Asset->ClearFlags(RF_Public | RF_Standalone);
		Asset->GetOutermost()->SetDirtyFlag(false);
	}
	return true;
}
#endif
