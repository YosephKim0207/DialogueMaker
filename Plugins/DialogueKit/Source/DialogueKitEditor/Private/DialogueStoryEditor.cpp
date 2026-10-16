#include "DialogueStoryEditor.h"
#include "DialogueStoryDefinition.h"
#include "DialogueGraph.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/AssetRegistryTagsContext.h"
#include "ScopedTransaction.h"
#include "Misc/OutputDevice.h"
#include "UObject/Package.h"
#include "Editor.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "ObjectTools.h"

// 미완성 저작 에셋도 표시하되 파싱 실패는 삭제 가능 항목으로 간주하지 않는다.
TArray<FDialogueStoryBlock> FDialogueStoryEditor::CollectBlocks()
{
	TArray<FAssetData> Assets;
	FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get().GetAssetsByClass(UDialogueGraph::StaticClass()->GetClassPathName(), Assets, true);
	TArray<FDialogueStoryBlock> Result;
	for (const FAssetData& Asset : Assets)
	{
		FDialogueStoryBlock Row;
		Row.Asset = Asset;
		if (Asset.IsAssetLoaded())
		{
			if (const auto* Graph = Cast<UDialogueGraph>(Asset.GetAsset()))
			{
				Row.Metadata = Graph->GetSelectionMetadata();
				Row.bMetadataReadable = true;
			}
		}
		else
		{
			FString Value;
			FStringOutputDevice Errors;
			if (Asset.GetTagValue(FDialogueBlockMetadata::RegistryKey(), Value))
			{
				const TCHAR* End = FDialogueBlockMetadata::StaticStruct()->ImportText(*Value, &Row.Metadata, nullptr, PPF_None, &Errors, TEXT("DialogueBlockMetadata"));
				Row.bMetadataReadable = End && *End == 0 && Errors.IsEmpty() && Row.Metadata.MetadataVersion == 1;
			}
		}
		Result.Add(Row);
	}
	Result.Sort([](const FDialogueStoryBlock& A, const FDialogueStoryBlock& B) { return A.Asset.AssetName.LexicalLess(B.Asset.AssetName); });
	return Result;
}

// 명시 소속 없음과 사라진 부모 참조를 신규 후보에서 모두 제외하되 UI에서 이름을 구분한다.
bool FDialogueStoryEditor::IsUnassigned(const FDialogueStoryScope& Scope, const UDialogueStoryDefinition* Definition)
{
	if (Scope.Mode == EDialogueStoryScope::Unassigned) return true;
	if (Definition == nullptr) return false;
	if (Scope.Mode == EDialogueStoryScope::Chapter) return Definition->FindChapter(Scope.ChapterId) == nullptr;
	if (Scope.Mode == EDialogueStoryScope::Scene)
	{
		const auto* Scene = Definition->FindScene(Scope.SceneId);
		return Scene == nullptr || Definition->FindChapter(Scene->ChapterId) == nullptr;
	}
	return false;
}

// Chapter만 제거하고 Scene/Block과 안정 ID는 보존한다.
bool FDialogueStoryEditor::RemoveChapter(UDialogueStoryDefinition* Definition, FGuid Id)
{
	if (Definition == nullptr || Definition->FindChapter(Id) == nullptr) return false;
	TArray<FString> Issues;
	if (Definition->ValidateDefinitions(Issues) == false) return false;
	const FScopedTransaction Transaction(NSLOCTEXT("DialogueKit", "RemoveChapter", "Chapter 제거"));
	Definition->Modify();
	Definition->Chapters.RemoveAll([Id](const auto& Chapter) { return Chapter.ChapterId == Id; });
	for (auto& Scene : Definition->Scenes) if (Scene.ChapterId == Id) Scene.ChapterId.Invalidate();
	Definition->MarkPackageDirty();
	return true;
}

// Scene 제거는 Block을 삭제하거나 제한 없음으로 변경하지 않는다. Undo로 참조가 다시 살아난다.
bool FDialogueStoryEditor::RemoveScene(UDialogueStoryDefinition* Definition, FGuid Id)
{
	if (Definition == nullptr || Definition->FindScene(Id) == nullptr) return false;
	TArray<FString> Issues;
	if (Definition->ValidateDefinitions(Issues) == false) return false;
	const FScopedTransaction Transaction(NSLOCTEXT("DialogueKit", "RemoveScene", "Scene 제거"));
	Definition->Modify();
	Definition->Scenes.RemoveAll([Id](const auto& Scene) { return Scene.SceneId == Id; });
	Definition->MarkPackageDirty();
	return true;
}

// 부모만 바꾸므로 Scene과 Block ID 및 실행 이력은 그대로다.
bool FDialogueStoryEditor::MoveScene(UDialogueStoryDefinition* Definition, FGuid SceneId, FGuid ChapterId)
{
	if (Definition == nullptr || Definition->FindScene(SceneId) == nullptr || (ChapterId.IsValid() && Definition->FindChapter(ChapterId) == nullptr)) return false;
	TArray<FString> Issues;
	if (Definition->ValidateDefinitions(Issues) == false)
	{
		// 이 Scene의 잘못된 부모만 명시적으로 고치는 이동은 허용한다.
		// 자체 ID 중복/누락이나 다른 Scene 오류는 우회하지 않는다.
		for (const FString& Issue : Issues) if (Issue != TEXT("MissingOrDuplicateSceneParent")) return false;
		for (const auto& Scene : Definition->Scenes)
		{
			if (Scene.SceneId != SceneId && Scene.ChapterId.IsValid() && Definition->FindChapter(Scene.ChapterId) == nullptr) return false;
		}
	}
	const FScopedTransaction Transaction(NSLOCTEXT("DialogueKit", "MoveScene", "Scene 소속 이동"));
	Definition->Modify();
	for (auto& Scene : Definition->Scenes) if (Scene.SceneId == SceneId) Scene.ChapterId = ChapterId;
	Definition->MarkPackageDirty();
	return true;
}

// 모든 대상을 확인한 뒤 하나의 Undo로 Block 소속을 이동한다.
bool FDialogueStoryEditor::MoveBlocks(UDialogueStoryDefinition* Definition, const TArray<FAssetData>& Assets, const FDialogueStoryScope& Scope)
{
	if (Definition == nullptr || Assets.IsEmpty()) return false;
	TArray<FString> DefinitionIssues;
	if (Definition->ValidateDefinitions(DefinitionIssues) == false) return false;
	if (Scope.Mode == EDialogueStoryScope::Chapter && (Definition->FindChapter(Scope.ChapterId) == nullptr || Scope.SceneId.IsValid())) return false;
	if ((Scope.Mode == EDialogueStoryScope::Unassigned || Scope.Mode == EDialogueStoryScope::Unrestricted) && (Scope.ChapterId.IsValid() || Scope.SceneId.IsValid())) return false;
	if (Scope.Mode == EDialogueStoryScope::Scene && (Definition->FindScene(Scope.SceneId) == nullptr || Scope.ChapterId.IsValid())) return false;
	if (Scope.Mode != EDialogueStoryScope::Unassigned && Scope.Mode != EDialogueStoryScope::Unrestricted && Scope.Mode != EDialogueStoryScope::Chapter && Scope.Mode != EDialogueStoryScope::Scene) return false;
	TArray<UDialogueGraph*> Graphs;
	for (const FAssetData& Asset : Assets)
	{
		auto* Graph = Cast<UDialogueGraph>(Asset.GetAsset());
		if (Graph == nullptr || Graph->GetPathName() != Asset.GetObjectPathString()) return false;
		Graphs.Add(Graph);
	}
	const FScopedTransaction Transaction(NSLOCTEXT("DialogueKit", "MoveBlocks", "Block 소속 이동"));
	for (UDialogueGraph* Graph : Graphs)
	{
		Graph->Modify();
		Graph->StoryScope = Scope;
		Graph->MarkPackageDirty();
		FAssetRegistryModule::GetRegistry().AssetUpdateTags(Graph, EAssetRegistryTagsCaller::Fast);
	}
	return true;
}

// 물리 삭제는 Undo 대상이 아니다. 미저장/열린/복원전용/소속 있는 에셋은 전체 요청을 거부한다.
bool FDialogueStoryEditor::DeleteUnassigned(UDialogueStoryDefinition* Definition, const TArray<FAssetData>& Assets, FText& Result)
{
	if (Definition == nullptr || Assets.IsEmpty())
	{
		Result = FText::FromString(TEXT("삭제 대상 또는 스토리 정의가 없습니다."));
		return false;
	}
	TArray<FString> DefinitionIssues;
	if (Definition->ValidateDefinitions(DefinitionIssues) == false)
	{
		Result = FText::FromString(TEXT("스토리 정의가 유효하지 않습니다: ") + FString::Join(DefinitionIssues, TEXT(", ")));
		return false;
	}
	for (const FAssetData& Asset : Assets)
	{
		auto* Graph = Cast<UDialogueGraph>(Asset.GetAsset());
		if (Graph == nullptr || Graph->GetPathName() != Asset.GetObjectPathString() || Graph->Availability != EDialogueBlockAvailability::Active ||
			IsUnassigned(Graph->StoryScope, Definition) == false || Graph->GetOutermost()->IsDirty() ||
			GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->FindEditorForAsset(Graph, false))
		{
			Result = FText::FromString(TEXT("삭제 대상이 변경되었거나 보호/미저장/열린 에셋입니다: ") + Asset.GetObjectPathString());
			return false;
		}
	}
	ObjectTools::DeleteAssets(Assets, false);
	TArray<FString> Remaining;
	IAssetRegistry& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
	for (const FAssetData& Asset : Assets)
	{
		if (Registry.GetAssetByObjectPath(Asset.GetSoftObjectPath()).IsValid()) Remaining.Add(Asset.GetObjectPathString());
	}
	Result = Remaining.IsEmpty() ? FText::FromString(TEXT("선택한 Block 삭제 완료")) : FText::FromString(TEXT("삭제하지 못한 Block:\n") + FString::Join(Remaining, TEXT("\n")));
	return Remaining.IsEmpty();
}

// 기본 배열 추가 대신 기존 명시적 생성 API를 트랜잭션으로 호출한다.
FGuid FDialogueStoryEditor::AddDefinitionItem(UDialogueStoryDefinition* Definition, bool bScene)
{
	if (Definition == nullptr) return FGuid();
	TArray<FString> Issues;
	if (Definition->ValidateDefinitions(Issues) == false) return FGuid();
	const FScopedTransaction Transaction(NSLOCTEXT("DialogueKit", "AddStoryDefinitionItem", "스토리 정의 항목 추가"));
	Definition->Modify();
	const FGuid Id = bScene ? Definition->AddScene(FGuid(), FText::FromString(TEXT("새 Scene"))) : Definition->AddChapter(FText::FromString(TEXT("새 Chapter")));
	Definition->PostEditChange();
	Definition->MarkPackageDirty();
	return Id;
}

// 명시적인 버튼 동작에서만 누락된 자체 ID를 복구한다. 소속은 추측하지 않는다.
int32 FDialogueStoryEditor::AssignMissingIds(UDialogueStoryDefinition* Definition)
{
	if (Definition == nullptr) return 0;
	int32 Count = 0;
	for (const auto& Chapter : Definition->Chapters) if (Chapter.ChapterId.IsValid() == false) ++Count;
	for (const auto& Scene : Definition->Scenes) if (Scene.SceneId.IsValid() == false) ++Count;
	if (Count == 0) return 0;
	const FScopedTransaction Transaction(NSLOCTEXT("DialogueKit", "AssignMissingStoryIds", "누락 스토리 ID 생성"));
	Definition->Modify();
	for (auto& Chapter : Definition->Chapters) if (Chapter.ChapterId.IsValid() == false) Chapter.ChapterId = FGuid::NewGuid();
	for (auto& Scene : Definition->Scenes) if (Scene.SceneId.IsValid() == false) Scene.SceneId = FGuid::NewGuid();
	Definition->PostEditChange();
	Definition->MarkPackageDirty();
	return Count;
}
