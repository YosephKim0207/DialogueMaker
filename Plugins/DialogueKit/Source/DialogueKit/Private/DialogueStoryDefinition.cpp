#include "DialogueStoryDefinition.h"

// 중복된 정의도 유효한 단일 Chapter로 취급하지 않는다.
const FDialogueChapterDefinition* UDialogueStoryDefinition::FindChapter(const FGuid& ChapterId) const
{
	if (ChapterId.IsValid() == false)
	{
		return nullptr;
	}
	const FDialogueChapterDefinition* Found = nullptr;
	for (const FDialogueChapterDefinition& Chapter : Chapters)
	{
		if (Chapter.ChapterId == ChapterId)
		{
			if (Found != nullptr)
			{
				return nullptr;
			}
			Found = &Chapter;
		}
	}
	return Found;
}

// Scene ID는 이름이나 정렬 순서와 독립적이다.
const FDialogueSceneDefinition* UDialogueStoryDefinition::FindScene(const FGuid& SceneId) const
{
	if (SceneId.IsValid() == false)
	{
		return nullptr;
	}
	const FDialogueSceneDefinition* Found = nullptr;
	for (const FDialogueSceneDefinition& Scene : Scenes)
	{
		if (Scene.SceneId == SceneId)
		{
			if (Found != nullptr)
			{
				return nullptr;
			}
			Found = &Scene;
		}
	}
	return Found;
}

// 생성자 대신 명시적 저작 동작에서만 새 정체성을 발급한다.
FGuid UDialogueStoryDefinition::AddChapter(const FText& DisplayName)
{
	FDialogueChapterDefinition Chapter;
	Chapter.ChapterId = FGuid::NewGuid();
	Chapter.DisplayName = DisplayName;
	Chapters.Add(Chapter);
	return Chapter.ChapterId;
}

// 잘못된 부모 요청은 기존 정의를 변경하지 않는다.
FGuid UDialogueStoryDefinition::AddScene(const FGuid& ChapterId, const FText& DisplayName)
{
	if (ChapterId.IsValid() && FindChapter(ChapterId) == nullptr)
	{
		return FGuid();
	}
	FDialogueSceneDefinition Scene;
	Scene.SceneId = FGuid::NewGuid();
	Scene.ChapterId = ChapterId;
	Scene.DisplayName = DisplayName;
	Scenes.Add(Scene);
	return Scene.SceneId;
}

// 소속 없는 Scene은 보존 가능한 저작 상태이며 사라진 부모 참조와 구분한다.
bool UDialogueStoryDefinition::ValidateDefinitions(TArray<FString>& OutIssues) const
{
	OutIssues.Reset();
	TSet<FGuid> ChapterIds;
	for (const FDialogueChapterDefinition& Chapter : Chapters)
	{
		if (Chapter.ChapterId.IsValid() == false || ChapterIds.Contains(Chapter.ChapterId))
		{
			OutIssues.Add(TEXT("InvalidOrDuplicateChapterId"));
		}
		ChapterIds.Add(Chapter.ChapterId);
	}
	TSet<FGuid> SceneIds;
	for (const FDialogueSceneDefinition& Scene : Scenes)
	{
		if (Scene.SceneId.IsValid() == false || SceneIds.Contains(Scene.SceneId))
		{
			OutIssues.Add(TEXT("InvalidOrDuplicateSceneId"));
		}
		SceneIds.Add(Scene.SceneId);
		if (Scene.ChapterId.IsValid() && FindChapter(Scene.ChapterId) == nullptr)
		{
			OutIssues.Add(TEXT("MissingOrDuplicateSceneParent"));
		}
	}
	return OutIssues.IsEmpty();
}

// 신규 후보의 소속 조건만 평가하고 진행 상태나 정의를 변경하지 않는다.
FDialogueStoryScopeResult UDialogueStoryLibrary::EvaluateScope(const UDialogueStoryDefinition* Definition,
	const FDialogueStoryScope& Scope, const FDialogueStoryProgress& Progress)
{
	FDialogueStoryScopeResult Result;
	if (Scope.Mode == EDialogueStoryScope::Unassigned || Scope.Mode == EDialogueStoryScope::Unrestricted)
	{
		// 이 두 모드는 소속 ID를 사용하지 않는다. 소속 없음에 남은 참조도 후보가 되지 않는다.
		Result.bValid = true;
		Result.bMatches = Scope.Mode == EDialogueStoryScope::Unrestricted;
		Result.ErrorCode = NAME_None;
		return Result;
	}
	if (IsValid(Definition) == false)
	{
		Result.ErrorCode = TEXT("MissingStoryDefinition");
		return Result;
	}
	FGuid ScopeChapter;
	if (Scope.Mode == EDialogueStoryScope::Chapter && Scope.SceneId.IsValid() == false)
	{
		ScopeChapter = Scope.ChapterId;
	}
	else if (Scope.Mode == EDialogueStoryScope::Scene && Scope.ChapterId.IsValid() == false)
	{
		const FDialogueSceneDefinition* Scene = Definition->FindScene(Scope.SceneId);
		if (Scene == nullptr)
		{
			Result.ErrorCode = TEXT("MissingOrDuplicateScopeScene");
			return Result;
		}
		ScopeChapter = Scene->ChapterId;
	}
	else
	{
		return Result;
	}
	if (Definition->FindChapter(ScopeChapter) == nullptr)
	{
		Result.ErrorCode = TEXT("MissingOrDuplicateScopeChapter");
		return Result;
	}
	if (Progress.ChapterId.IsValid() && Definition->FindChapter(Progress.ChapterId) == nullptr)
	{
		Result.ErrorCode = TEXT("InvalidStoryProgress");
		return Result;
	}
	if (Progress.SceneId.IsValid())
	{
		const FDialogueSceneDefinition* CurrentScene = Definition->FindScene(Progress.SceneId);
		if (CurrentScene == nullptr || Progress.ChapterId.IsValid() == false || CurrentScene->ChapterId != Progress.ChapterId)
		{
			Result.ErrorCode = TEXT("InvalidStoryProgress");
			return Result;
		}
	}
	Result.bValid = true;
	Result.ErrorCode = NAME_None;
	Result.bMatches = ScopeChapter == Progress.ChapterId &&
		(Scope.Mode == EDialogueStoryScope::Chapter || Scope.SceneId == Progress.SceneId);
	return Result;
}
