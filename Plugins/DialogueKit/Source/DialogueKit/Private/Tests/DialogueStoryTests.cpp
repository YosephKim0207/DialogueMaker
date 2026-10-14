#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "DialogueStoryDefinition.h"
#include "DialogueGraph.h"
#include "Serialization/ObjectReader.h"
#include "Serialization/ObjectWriter.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDialogueStoryScopeTest,
	"DialogueKit.Contracts.Story.Scope",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// 소속 범위·부모 정합성과 정의 이동 이후의 안정 ID 동작을 검사한다.
bool FDialogueStoryScopeTest::RunTest(const FString& Parameters)
{
	UDialogueStoryDefinition* Definition = NewObject<UDialogueStoryDefinition>();
	const FGuid ChapterA = Definition->AddChapter(FText::FromString(TEXT("A")));
	const FGuid ChapterB = Definition->AddChapter(FText::FromString(TEXT("B")));
	const FGuid SceneA = Definition->AddScene(ChapterA, FText::FromString(TEXT("A1")));
	const FGuid SceneB = Definition->AddScene(ChapterB, FText::FromString(TEXT("B1")));
	FDialogueStoryScope Scope;
	FDialogueStoryProgress Progress;
	Progress.ChapterId = ChapterA;
	Progress.SceneId = SceneA;
	TestFalse(TEXT("Unassigned excluded"), UDialogueStoryLibrary::EvaluateScope(Definition, Scope, Progress).bMatches);
	Scope.Mode = EDialogueStoryScope::Unrestricted;
	TestTrue(TEXT("Unrestricted ignores missing definitions"), UDialogueStoryLibrary::EvaluateScope(nullptr, Scope, Progress).bMatches);
	Scope.Mode = EDialogueStoryScope::Chapter;
	Scope.ChapterId = ChapterA;
	TestEqual(TEXT("Scoped null definition error"), UDialogueStoryLibrary::EvaluateScope(nullptr, Scope, Progress).ErrorCode, FName(TEXT("MissingStoryDefinition")));
	TestTrue(TEXT("Chapter matches"), UDialogueStoryLibrary::EvaluateScope(Definition, Scope, Progress).bMatches);
	Scope.ChapterId = ChapterB;
	TestFalse(TEXT("Other chapter excluded"), UDialogueStoryLibrary::EvaluateScope(Definition, Scope, Progress).bMatches);
	Scope.Mode = EDialogueStoryScope::Scene;
	Scope.SceneId = SceneA;
	TestFalse(TEXT("Redundant scope parent rejected"), UDialogueStoryLibrary::EvaluateScope(Definition, Scope, Progress).bValid);
	Scope.ChapterId.Invalidate();
	TestTrue(TEXT("Scene matches"), UDialogueStoryLibrary::EvaluateScope(Definition, Scope, Progress).bMatches);
	FDialogueSceneDefinition DuplicateScene = Definition->Scenes[0];
	Definition->Scenes.Add(DuplicateScene);
	TestEqual(TEXT("Duplicate referenced scene rejected"), UDialogueStoryLibrary::EvaluateScope(Definition, Scope, Progress).ErrorCode, FName(TEXT("MissingOrDuplicateScopeScene")));
	Definition->Scenes.Pop();
	DuplicateScene = Definition->Scenes[1];
	Definition->Scenes.Add(DuplicateScene);
	TestTrue(TEXT("Unrelated duplicate does not reject candidate"), UDialogueStoryLibrary::EvaluateScope(Definition, Scope, Progress).bMatches);
	Definition->Scenes.Pop();
	Progress.SceneId = SceneB;
	TestFalse(TEXT("Mismatched current parent rejected"), UDialogueStoryLibrary::EvaluateScope(Definition, Scope, Progress).bValid);
	Progress.SceneId = SceneA;
	Definition->Scenes[0].ChapterId = ChapterB;
	TestFalse(TEXT("Old progress after move rejected"), UDialogueStoryLibrary::EvaluateScope(Definition, Scope, Progress).bValid);
	Progress.ChapterId = ChapterB;
	TestTrue(TEXT("Same scope ID follows moved scene"), UDialogueStoryLibrary::EvaluateScope(Definition, Scope, Progress).bMatches);
	Definition->Scenes[0].ChapterId.Invalidate();
	TestFalse(TEXT("Orphan excluded without becoming unrestricted"), UDialogueStoryLibrary::EvaluateScope(Definition, Scope, Progress).bMatches);
	TestEqual(TEXT("Orphan scope error is explicit"), UDialogueStoryLibrary::EvaluateScope(Definition, Scope, Progress).ErrorCode, FName(TEXT("MissingOrDuplicateScopeChapter")));
	Scope.Mode = EDialogueStoryScope::Unrestricted;
	Scope.ChapterId = FGuid::NewGuid();
	TestTrue(TEXT("Unrestricted ignores inactive IDs"), UDialogueStoryLibrary::EvaluateScope(nullptr, Scope, Progress).bMatches);
	Scope.Mode = EDialogueStoryScope::Unassigned;
	TestTrue(TEXT("Unassigned with inactive IDs remains valid"), UDialogueStoryLibrary::EvaluateScope(nullptr, Scope, Progress).bValid);
	Scope.Mode = EDialogueStoryScope::Chapter;
	Scope.SceneId.Invalidate();
	Scope.ChapterId = ChapterA;
	Progress = FDialogueStoryProgress();
	FDialogueStoryScopeResult NoProgress = UDialogueStoryLibrary::EvaluateScope(Definition, Scope, Progress);
	TestTrue(TEXT("No progress is valid but does not match"), NoProgress.bValid && NoProgress.bMatches == false);
	FDialogueChapterDefinition Duplicate = Definition->Chapters[0];
	Definition->Chapters.Add(Duplicate);
	TestFalse(TEXT("Duplicate referenced chapter excluded"), UDialogueStoryLibrary::EvaluateScope(Definition, Scope, Progress).bValid);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDialogueStoryDefinitionTest,
	"DialogueKit.Contracts.Story.IdentityAndRoundtrip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// 이름/정렬 변경과 직렬화가 정체성을 바꾸지 않고 오류 검사가 입력을 수정하지 않는지 확인한다.
bool FDialogueStoryDefinitionTest::RunTest(const FString& Parameters)
{
	UDialogueStoryDefinition* Definition = NewObject<UDialogueStoryDefinition>();
	const FGuid ChapterId = Definition->AddChapter(FText::FromString(TEXT("Chapter")));
	const FGuid SceneId = Definition->AddScene(ChapterId, FText::FromString(TEXT("Scene")));
	TestFalse(TEXT("Bad parent add fails"), Definition->AddScene(FGuid::NewGuid(), FText()).IsValid());
	TestEqual(TEXT("Failed add leaves list unchanged"), Definition->Scenes.Num(), 1);
	Definition->Chapters[0].DisplayName = FText::FromString(TEXT("Renamed"));
	Definition->Chapters[0].SortOrder = 20;
	Definition->Scenes[0].SortOrder = -10;
	TArray<FString> Issues;
	TestTrue(TEXT("Definitions valid"), Definition->ValidateDefinitions(Issues));
	TArray<uint8> Bytes;
	FObjectWriter Writer(Definition, Bytes);
	UDialogueStoryDefinition* Restored = NewObject<UDialogueStoryDefinition>();
	FObjectReader Reader(Restored, Bytes);
	TestTrue(TEXT("Chapter ID preserved"), Restored->Chapters[0].ChapterId == ChapterId);
	TestTrue(TEXT("Scene ID preserved"), Restored->Scenes[0].SceneId == SceneId);
	TestEqual(TEXT("Sort value preserved"), Restored->Scenes[0].SortOrder, -10);
	TestTrue(TEXT("Display name preserved"), Restored->Chapters[0].DisplayName.EqualTo(Definition->Chapters[0].DisplayName));
	Restored->Scenes[0].ChapterId = FGuid::NewGuid();
	TestFalse(TEXT("Dangling parent reported"), Restored->ValidateDefinitions(Issues));
	Restored->Scenes[0].ChapterId.Invalidate();
	TestTrue(TEXT("Unassigned scene may be preserved"), Restored->ValidateDefinitions(Issues));
	Restored->Scenes[0].SceneId.Invalidate();
	TestFalse(TEXT("Missing identity reported"), Restored->ValidateDefinitions(Issues));
	TestFalse(TEXT("Validation never creates ID"), Restored->Scenes[0].SceneId.IsValid());
	UDialogueGraph* Graph = NewObject<UDialogueGraph>();
	TestTrue(TEXT("New scope defaults unassigned"), Graph->StoryScope.Mode == EDialogueStoryScope::Unassigned);
	Graph->StoryScope.Mode = EDialogueStoryScope::Scene;
	Graph->StoryScope.SceneId = SceneId;
	TArray<uint8> GraphBytes;
	FObjectWriter GraphWriter(Graph, GraphBytes);
	UDialogueGraph* RestoredGraph = NewObject<UDialogueGraph>();
	FObjectReader GraphReader(RestoredGraph, GraphBytes);
	TestTrue(TEXT("Graph scope roundtrip"), RestoredGraph->StoryScope.Mode == EDialogueStoryScope::Scene && RestoredGraph->StoryScope.SceneId == SceneId);
	return true;
}
#endif
