#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "AssetRegistry/AssetRegistryState.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDialogueShippingIsolationTest, "ProjectShipping.DialogueIsolation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// fixture 없는 Shipping cook 직후 실행한다. 개발용 Block과 테스트 대상 태그 유출을 검사한다.
bool FDialogueShippingIsolationTest::RunTest(const FString& Parameters)
{
	FAssetRegistryState Registry;
	if (!TestTrue(TEXT("Cooked Registry available"), FAssetRegistryState::LoadFromDisk(
		*(FPaths::ProjectSavedDir() / TEXT("Cooked/Windows/DialogueMaker/AssetRegistry.bin")), FAssetRegistryLoadOptions(), Registry))) return false;
	TArray<FAssetData> Assets;
	Registry.GetAllAssets(TSet<FName>(), Assets);
	TestTrue(TEXT("Nonempty cooked assets"), Assets.Num() > 0);
	for (const FAssetData& Asset : Assets)
	{
		TestFalse(TEXT("No validation fixture in production cook"), Asset.PackageName.ToString().StartsWith(TEXT("/Game/DialogueValidation/")));
		Asset.TagsAndValues.ForEach([this](const TPair<FName, FAssetTagValueRef>& Tag)
		{
			TestFalse(TEXT("No development dialogue target metadata"), Tag.Value.AsString().Contains(TEXT("DialogueKit.Test.")));
		});
	}
	return true;
}
#endif
