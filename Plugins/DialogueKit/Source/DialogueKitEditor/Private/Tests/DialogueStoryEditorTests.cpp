#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "DialogueStoryEditor.h"
#include "DialogueStoryDefinition.h"
#include "DialogueGraph.h"
#include "Editor.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "AssetRegistry/AssetRegistryModule.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDialogueStoryEditorTest, "DialogueKit.Editor.Storyboard", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// 기존 에셋을 삭제하지 않고 임시 객체로 이동·보존·Undo 경계를 검증한다.
bool FDialogueStoryEditorTest::RunTest(const FString& Parameters)
{
	auto* Definition = NewObject<UDialogueStoryDefinition>(GetTransientPackage(), NAME_None, RF_Transactional);
	const FGuid A = Definition->AddChapter(FText::FromString(TEXT("A")));
	const FGuid B = Definition->AddChapter(FText::FromString(TEXT("B")));
	const FGuid Scene = Definition->AddScene(A, FText::FromString(TEXT("Scene")));
	auto* Block = NewObject<UDialogueGraph>(GetTransientPackage(), NAME_None, RF_Transactional);
	Block->InitializeBlockIdentity();
	const auto Identity = Block->GetBlockReference();
	FDialogueStoryScope Scope;
	Scope.Mode = EDialogueStoryScope::Scene;
	Scope.SceneId = Scene;
	TArray<FAssetData> Assets = {FAssetData(Block)};
	TestTrue(TEXT("Assign scene"), FDialogueStoryEditor::MoveBlocks(Definition, Assets, Scope));
	TestEqual(TEXT("Stable block identity"), Block->GetBlockReference().BlockId, Identity.BlockId);
	TestTrue(TEXT("Move scene"), FDialogueStoryEditor::MoveScene(Definition, Scene, B));
	TestEqual(TEXT("Parent changed"), Definition->FindScene(Scene)->ChapterId, B);
	TestEqual(TEXT("Block references stable Scene"), Block->StoryScope.SceneId, Scene);
	TestTrue(TEXT("Undo move"), GEditor->UndoTransaction());
	TestEqual(TEXT("Parent restored"), Definition->FindScene(Scene)->ChapterId, A);
	TestTrue(TEXT("Remove chapter only"), FDialogueStoryEditor::RemoveChapter(Definition, A));
	TestNotNull(TEXT("Child Scene preserved"), Definition->FindScene(Scene));
	TestFalse(TEXT("Scene orphaned"), Definition->FindScene(Scene)->ChapterId.IsValid());
	TestTrue(TEXT("Orphan block excluded"), FDialogueStoryEditor::IsUnassigned(Block->StoryScope, Definition));
	TestTrue(TEXT("Undo chapter removal"), GEditor->UndoTransaction());
	TestEqual(TEXT("Parent and ID restored"), Definition->FindScene(Scene)->ChapterId, A);
	TestTrue(TEXT("Remove scene only"), FDialogueStoryEditor::RemoveScene(Definition, Scene));
	TestTrue(TEXT("Missing scene is unassigned view"), FDialogueStoryEditor::IsUnassigned(Block->StoryScope, Definition));
	TestEqual(TEXT("Block retained reference for Undo"), Block->StoryScope.SceneId, Scene);
	TestTrue(TEXT("Undo scene removal"), GEditor->UndoTransaction());
	TestFalse(TEXT("Reference restored"), FDialogueStoryEditor::IsUnassigned(Block->StoryScope, Definition));
	Scope.SceneId = FGuid::NewGuid();
	TestFalse(TEXT("Unknown destination refused"), FDialogueStoryEditor::MoveBlocks(Definition, Assets, Scope));
	TestEqual(TEXT("Failure preserves assignment"), Block->StoryScope.SceneId, Scene);
	FText Error;
	TestFalse(TEXT("Assigned block cannot be deleted"), FDialogueStoryEditor::DeleteUnassigned(Definition, Assets, Error));
	Block->StoryScope = FDialogueStoryScope();
	Block->Availability = EDialogueBlockAvailability::RestoreOnly;
	TestFalse(TEXT("Restore-only block protected"), FDialogueStoryEditor::DeleteUnassigned(Definition, Assets, Error));
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDialogueStoryDeleteTest, "DialogueKit.Editor.StoryboardIsolatedDelete", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// 실행별 Saved/AI 임시 에셋만 삭제하고 선택 밖 에셋은 파일까지 유지하는지 확인한다.
bool FDialogueStoryDeleteTest::RunTest(const FString& Parameters)
{
	const FString Id = FGuid::NewGuid().ToString(EGuidFormats::Digits);
	const FString Mount = TEXT("/DialogueStoryDelete") + Id + TEXT("/");
	const FString Folder = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("AI/StoryDelete") / Id) + TEXT("/");
	IFileManager::Get().MakeDirectory(*Folder, true);
	FPackageName::RegisterMountPoint(Mount, Folder);
	auto* Definition = NewObject<UDialogueStoryDefinition>();
	TArray<FAssetData> SelectedAssets;
	for (const FString Name : {FString(TEXT("DeleteOnlyThis")), FString(TEXT("PreserveOther"))})
	{
		UPackage* Package = CreatePackage(*(Mount + Name));
		auto* Block = NewObject<UDialogueGraph>(Package, *Name, RF_Public | RF_Standalone);
		Block->InitializeBlockIdentity();
		FSavePackageArgs Args;
		Args.TopLevelFlags = RF_Public | RF_Standalone;
		Args.SaveFlags = SAVE_NoError;
		if (TestTrue(TEXT("Save isolated fixture"), UPackage::SavePackage(Package, Block, *(Folder + Name + TEXT(".uasset")), Args)) == false) return false;
		Package->SetDirtyFlag(false);
		FAssetRegistryModule::AssetCreated(Block);
		if (Name == TEXT("DeleteOnlyThis")) SelectedAssets.Add(FAssetData(Block));
	}
	FText Result;
	TestTrue(TEXT("Delete selected unassigned fixture"), FDialogueStoryEditor::DeleteUnassigned(Definition, SelectedAssets, Result));
	TestFalse(TEXT("Selected file removed"), IFileManager::Get().FileExists(*(Folder + TEXT("DeleteOnlyThis.uasset"))));
	TestTrue(TEXT("Unselected file retained"), IFileManager::Get().FileExists(*(Folder + TEXT("PreserveOther.uasset"))));
	TestFalse(TEXT("Result provided"), Result.IsEmpty());
	return true;
}
#endif
