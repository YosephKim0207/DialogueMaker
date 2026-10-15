#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "DialogueGraph.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "NativeGameplayTags.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "PackageTools.h"
#include "UObject/SavePackage.h"
#include "UObject/Package.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDialogueRegistryTest, "DialogueKit.Selection.UnloadedRegistry", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// 실제 패키지 헤더의 Registry 정보만으로 평가하고 Graph는 로드하지 않는다.
bool FDialogueRegistryTest::RunTest(const FString& Parameters)
{
	const FString Id = FGuid::NewGuid().ToString(EGuidFormats::Digits);
	const FString Mount = TEXT("/DialogueRegistryTest") + Id + TEXT("/");
	const FString Folder = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("AI/RegistryTests") / Id) + TEXT("/");
	IFileManager::Get().MakeDirectory(*Folder, true);
	FPackageName::RegisterMountPoint(Mount, Folder);
	const FString PackageName = Mount + TEXT("MetadataBlock");
	const FString Filename = Folder + TEXT("MetadataBlock.uasset");
	UPackage* Package = CreatePackage(*PackageName);
	UDialogueGraph* Graph = NewObject<UDialogueGraph>(Package, TEXT("MetadataBlock"), RF_Public | RF_Standalone);
	Graph->InitializeBlockIdentity();
	Graph->DialogueTarget = FGameplayTag::RequestGameplayTag(TEXT("DialogueKit.Test.MetadataTarget"));
	Graph->StoryScope.Mode = EDialogueStoryScope::Unrestricted;
	const FGuid BlockId = Graph->GetBlockId();
	FSavePackageArgs Args;
	Args.TopLevelFlags = RF_Public | RF_Standalone;
	Args.SaveFlags = SAVE_NoError;
	if (TestTrue(TEXT("Save package"), UPackage::SavePackage(Package, Graph, *Filename, Args)) == false)
	{
		return false;
	}
	TArray<UPackage*> Packages = {Package};
	if (TestTrue(TEXT("Unload"), UPackageTools::UnloadPackages(Packages)) == false)
	{
		return false;
	}
	IAssetRegistry& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
	Registry.ScanFilesSynchronous({Filename}, true);
	const FAssetData Asset = Registry.GetAssetByObjectPath(FSoftObjectPath(PackageName + TEXT(".MetadataBlock")));
	TestTrue(TEXT("Registry found saved asset"), Asset.IsValid());
	TestFalse(TEXT("Graph unloaded before evaluation"), Asset.IsAssetLoaded());
	FDialogueBlockMetadata Metadata;
	if (TestTrue(TEXT("Read without Graph load"), FDialogueBlockMetadata::ReadAsset(Asset, Metadata)))
	{
		TestEqual(TEXT("Stable ID from header"), Metadata.Block.BlockId, BlockId);
		TestTrue(TEXT("Evaluate unloaded metadata"), UDialogueMetadataLibrary::IsEligible(Metadata, FGameplayTag::RequestGameplayTag(TEXT("DialogueKit.Test.MetadataTarget")), nullptr, FDialogueStoryProgress(), nullptr, nullptr));
	}
	TestFalse(TEXT("Graph remains unloaded"), Asset.IsAssetLoaded());
	// 후속 에디터 검증이 패키지를 참조할 수 있으므로 mount는 프로세스 종료까지 유지한다.
	return true;
}
#endif
