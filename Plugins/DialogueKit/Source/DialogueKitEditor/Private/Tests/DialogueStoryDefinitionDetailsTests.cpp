#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "DialogueStoryDefinition.h"
#include "DialogueStoryEditor.h"
#include "Editor.h"
#include "PropertyEditorModule.h"
#include "IDetailsView.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SWindow.h"
#include "Framework/Application/SlateApplication.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "PackageTools.h"
#include "UObject/SavePackage.h"
#include "UObject/Package.h"

namespace
{
// 정의 에셋의 실제 Details가 기본 배열 대신 전용 생성/소속 UI를 만드는지 확인한다.
bool ContainsStoryText(const TSharedRef<SWidget>& Widget, const FString& Text)
{
	if (Widget->GetType() == FName(TEXT("STextBlock")) && StaticCastSharedRef<STextBlock>(Widget)->GetText().ToString() == Text) return true;
	FChildren* Children = Widget->GetChildren();
	for (int32 Index = 0; Index < Children->Num(); ++Index)
	{
		if (ContainsStoryText(Children->GetChildAt(Index), Text)) return true;
	}
	return false;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDialogueStoryDefinitionDetailsTest, "DialogueKit.Editor.StoryDefinitionDetails", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// 생성·명시적 누락 복구·부모 선택의 안정 ID와 Undo, 파일 재열기를 검사한다.
bool FDialogueStoryDefinitionDetailsTest::RunTest(const FString& Parameters)
{
	auto* Definition = NewObject<UDialogueStoryDefinition>(GetTransientPackage(), NAME_None, RF_Transactional);
	const FGuid Chapter = FDialogueStoryEditor::AddDefinitionItem(Definition, false);
	const FGuid Scene = FDialogueStoryEditor::AddDefinitionItem(Definition, true);
	if (!TestTrue(TEXT("Creation assigns IDs"), Chapter.IsValid() && Scene.IsValid() && Chapter != Scene)) return false;
	TestFalse(TEXT("New Scene starts unassigned"), Definition->FindScene(Scene)->ChapterId.IsValid());
	TestTrue(TEXT("Undo Scene creation"), GEditor->UndoTransaction());
	TestEqual(TEXT("Scene removed on Undo"), Definition->Scenes.Num(), 0);
	TestTrue(TEXT("Redo Scene creation"), GEditor->RedoTransaction());
	TestNotNull(TEXT("Redo retains Scene ID"), Definition->FindScene(Scene));
	TestTrue(TEXT("Select Chapter"), FDialogueStoryEditor::MoveScene(Definition, Scene, Chapter));
	TestEqual(TEXT("Parent assigned"), Definition->FindScene(Scene)->ChapterId, Chapter);
	TestTrue(TEXT("Undo parent"), GEditor->UndoTransaction());
	TestFalse(TEXT("Undo restores unassigned"), Definition->FindScene(Scene)->ChapterId.IsValid());
	TestTrue(TEXT("Redo parent"), GEditor->RedoTransaction());
	TestEqual(TEXT("Redo parent exact ID"), Definition->FindScene(Scene)->ChapterId, Chapter);
	Definition->Chapters.AddDefaulted();
	Definition->Scenes.AddDefaulted();
	TestFalse(TEXT("Invalid definition cannot add more"), FDialogueStoryEditor::AddDefinitionItem(Definition, false).IsValid());
	TestEqual(TEXT("Repair only two zero IDs"), FDialogueStoryEditor::AssignMissingIds(Definition), 2);
	const FGuid RepairedChapter = Definition->Chapters.Last().ChapterId;
	const FGuid RepairedScene = Definition->Scenes.Last().SceneId;
	TestTrue(TEXT("Repaired IDs valid"), RepairedChapter.IsValid() && RepairedScene.IsValid());
	TestFalse(TEXT("Do not guess zero parent"), Definition->Scenes.Last().ChapterId.IsValid());
	TestEqual(TEXT("Existing ID preserved"), Definition->Chapters[0].ChapterId, Chapter);
	TestEqual(TEXT("Existing parent preserved"), Definition->Scenes[0].ChapterId, Chapter);
	TestEqual(TEXT("Repair is idempotent"), FDialogueStoryEditor::AssignMissingIds(Definition), 0);
	TestTrue(TEXT("Undo repair"), GEditor->UndoTransaction());
	TestFalse(TEXT("Undo restores missing ID"), Definition->Chapters.Last().ChapterId.IsValid());
	TestTrue(TEXT("Redo repair"), GEditor->RedoTransaction());
	TestEqual(TEXT("Redo retains repaired ID"), Definition->Chapters.Last().ChapterId, RepairedChapter);

	Definition->Scenes.Last().ChapterId = FGuid::NewGuid();
	Definition->Scenes[0].ChapterId = FGuid::NewGuid();
	TestFalse(TEXT("Other invalid parent prevents partial repair"), FDialogueStoryEditor::MoveScene(Definition, RepairedScene, Chapter));
	Definition->Scenes[0].ChapterId = Chapter;
	TestTrue(TEXT("Explicit selector repairs own missing parent"), FDialogueStoryEditor::MoveScene(Definition, RepairedScene, Chapter));
	TestFalse(TEXT("Unknown destination rejected"), FDialogueStoryEditor::MoveScene(Definition, Scene, FGuid::NewGuid()));
	auto* Duplicate = NewObject<UDialogueStoryDefinition>();
	Duplicate->Chapters = Definition->Chapters;
	const FDialogueChapterDefinition DuplicateChapter = Duplicate->Chapters[0];
	Duplicate->Chapters.Add(DuplicateChapter);
	TestEqual(TEXT("Never rewrite valid duplicate IDs"), FDialogueStoryEditor::AssignMissingIds(Duplicate), 0);
	TArray<FString> Issues;
	TestFalse(TEXT("Duplicates remain diagnosed"), Duplicate->ValidateDefinitions(Issues));

	FDetailsViewArgs ViewArgs;
	TSharedRef<IDetailsView> View = FModuleManager::LoadModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor")).CreateDetailView(ViewArgs);
	View->SetObject(Definition);
	View->ForceRefresh();
	// Details 목록은 배치/틱 이후 행을 생성하므로 검사 전 격리 창에 배치한다.
	TSharedRef<SWindow> Window = SNew(SWindow).Title(FText::FromString(TEXT("Story Definition Automation"))).ClientSize(FVector2D(900, 900))[View];
	FSlateApplication::Get().AddWindow(Window);
	FSlateApplication::Get().Tick();
	FSlateApplication::Get().Tick();
	TestTrue(TEXT("Details has explicit Chapter creation"), ContainsStoryText(View, TEXT("+ Chapter")));
	TestTrue(TEXT("Details has explicit Scene creation"), ContainsStoryText(View, TEXT("+ Scene")));
	TestTrue(TEXT("Details has missing ID repair"), ContainsStoryText(View, TEXT("누락 ID 생성")));
	View->SetObject(nullptr);
	FSlateApplication::Get().RequestDestroyWindow(Window);
	FSlateApplication::Get().Tick();

	const FString Id = FGuid::NewGuid().ToString(EGuidFormats::Digits);
	const FString Mount = TEXT("/DialogueStoryDetailsTest") + Id + TEXT("/");
	const FString Folder = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("AI/StoryDetailsTests") / Id) + TEXT("/");
	IFileManager::Get().MakeDirectory(*Folder, true);
	FPackageName::RegisterMountPoint(Mount, Folder);
	UPackage* Package = CreatePackage(*(Mount + TEXT("Definition")));
	auto* Asset = NewObject<UDialogueStoryDefinition>(Package, TEXT("Definition"), RF_Public | RF_Standalone);
	Asset->Chapters = Definition->Chapters;
	Asset->Scenes = Definition->Scenes;
	FSavePackageArgs Args;
	Args.TopLevelFlags = RF_Public | RF_Standalone;
	Args.SaveFlags = SAVE_NoError;
	if (!TestTrue(TEXT("Save isolated definition"), UPackage::SavePackage(Package, Asset, *(Folder + TEXT("Definition.uasset")), Args))) return false;
	TArray<UPackage*> Packages = {Package};
	if (TestTrue(TEXT("Unload definition"), UPackageTools::UnloadPackages(Packages)))
	{
		UPackage* Reloaded = LoadPackage(nullptr, *(Mount + TEXT("Definition")), LOAD_None);
		auto* Loaded = Reloaded ? FindObject<UDialogueStoryDefinition>(Reloaded, TEXT("Definition")) : nullptr;
		if (TestNotNull(TEXT("Reload definition"), Loaded))
		{
			TestTrue(TEXT("Reloaded definition valid"), Loaded->ValidateDefinitions(Issues));
			TestEqual(TEXT("File preserves Chapter ID"), Loaded->Chapters[0].ChapterId, Chapter);
			TestEqual(TEXT("File preserves repaired Scene ID"), Loaded->Scenes.Last().SceneId, RepairedScene);
			TestEqual(TEXT("File preserves selected parent"), Loaded->Scenes.Last().ChapterId, Chapter);
		}
	}
	// 엔진 지연 검증이 참조하므로 마운트는 프로세스 종료까지 유지한다.
	return true;
}
#endif
