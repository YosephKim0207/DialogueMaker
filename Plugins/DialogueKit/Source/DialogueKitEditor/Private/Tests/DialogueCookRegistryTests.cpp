#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "DialogueGraph.h"
#include "AssetRegistry/AssetRegistryState.h"
#include "Misc/Paths.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDialogueCookRegistryTest, "ProjectCook.DialogueRegistry", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// Package 이후 실행한다. 기존 샘플의 cooked Registry에 새 필터 자료가 보존되는지 검사한다.
bool FDialogueCookRegistryTest::RunTest(const FString& Parameters)
{
	const FString Filename = FPaths::ProjectSavedDir() / TEXT("Cooked/Windows/DialogueMaker/AssetRegistry.bin");
	FAssetRegistryState Registry;
	if (TestTrue(TEXT("Load actual cooked registry"), FAssetRegistryState::LoadFromDisk(*Filename, FAssetRegistryLoadOptions(), Registry)) == false)
	{
		return false;
	}
	TArray<FAssetData> Assets;
	Registry.GetAllAssets(TSet<FName>(), Assets);
	int32 Count = 0;
	for (const FAssetData& Asset : Assets)
	{
		if (Asset.AssetClassPath != UDialogueGraph::StaticClass()->GetClassPathName())
		{
			continue;
		}
		++Count;
		FString Value;
		TestTrue(*FString::Printf(TEXT("Cooked metadata: %s"), *Asset.PackageName.ToString()), Asset.GetTagValue(FDialogueBlockMetadata::RegistryKey(), Value));
		TestTrue(TEXT("Metadata format retained"), Value.Contains(TEXT("MetadataVersion=1")));
		TestTrue(TEXT("Condition definition retained"), Value.Contains(TEXT("EntryConditions=")));
		TestEqual(TEXT("Primary asset type retained"), Asset.GetPrimaryAssetId().PrimaryAssetType.GetName(), FName(TEXT("Dialogue")));
	}
	TestTrue(TEXT("Cooked Dialogue graphs found"), Count > 0);
	AddInfo(FString::Printf(TEXT("Verified metadata on %d cooked DialogueGraph assets; legacy eligibility is not asserted."), Count));
	return true;
}
#endif
