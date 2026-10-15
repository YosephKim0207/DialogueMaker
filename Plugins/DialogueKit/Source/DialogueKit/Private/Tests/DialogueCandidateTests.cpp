#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "DialogueCandidateSelector.h"
#include "NativeGameplayTags.h"

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_SelectionTarget, "DialogueKit.Test.SelectionTarget");

static FDialogueBlockCandidate MakeCandidate(const TCHAR* Name, int32 Priority, bool bDefault = false)
{
	FDialogueBlockCandidate Candidate;
	Candidate.AssetPath = FSoftObjectPath(FString::Printf(TEXT("/Game/%s.%s"), Name, Name));
	Candidate.Metadata.Block.BlockId = FGuid::NewGuid();
	Candidate.Metadata.TargetTag = TAG_SelectionTarget;
	Candidate.Metadata.StoryScope.Mode = EDialogueStoryScope::Unrestricted;
	Candidate.Metadata.Priority = Priority;
	Candidate.Metadata.bDefault = bDefault;
	return Candidate;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDialogueCandidateTest, "DialogueKit.Selection.PriorityAndSession", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// 정상 완료 소진, 기본 순회, 최우선 반복과 세션 종료 초기화를 확인한다.
bool FDialogueCandidateTest::RunTest(const FString& Parameters)
{
	UDialogueCandidateSelector* Selector = NewObject<UDialogueCandidateSelector>();
	Selector->SetRandomSeed(123);
	Selector->BeginSession(TAG_SelectionTarget);
	TArray<FDialogueBlockCandidate> Candidates = {MakeCandidate(TEXT("A"), 10), MakeCandidate(TEXT("B"), 10), MakeCandidate(TEXT("C"), 5), MakeCandidate(TEXT("Default1"), 0, true), MakeCandidate(TEXT("Default2"), 0, true)};
	auto Select = [&]() { return Selector->Select(Candidates, nullptr, FDialogueStoryProgress(), nullptr, nullptr); };
	FDialogueSelectionResult First = Select();
	TestTrue(TEXT("Select ordinary"), First.bSelected);
	TestEqual(TEXT("Highest priority"), First.Candidate.Metadata.Priority, 10);
	TestTrue(TEXT("Record completed"), Selector->RecordCompleted(First.Candidate.Metadata.Block));
	FDialogueSelectionResult Second = Select();
	TestTrue(TEXT("Other peer"), Second.Candidate.Metadata.Block.BlockId != First.Candidate.Metadata.Block.BlockId);
	TestEqual(TEXT("Same highest tier"), Second.Candidate.Metadata.Priority, 10);
	Selector->RecordCompleted(Second.Candidate.Metadata.Block);
	FDialogueSelectionResult Third = Select();
	TestEqual(TEXT("Lower tier after exhaustion"), Third.Candidate.Metadata.Priority, 5);
	Selector->RecordCompleted(Third.Candidate.Metadata.Block);
	FDialogueSelectionResult Default1 = Select();
	TestTrue(TEXT("Fallback default"), Default1.Candidate.Metadata.bDefault);
	Selector->RecordCompleted(Default1.Candidate.Metadata.Block);
	FDialogueSelectionResult Default2 = Select();
	TestTrue(TEXT("Other default in cycle"), Default1.Candidate.Metadata.Block.BlockId != Default2.Candidate.Metadata.Block.BlockId);
	Selector->RecordCompleted(Default2.Candidate.Metadata.Block);
	FDialogueSelectionResult NewCycle = Select();
	TestEqual(TEXT("New cycle avoids last"), NewCycle.Candidate.Metadata.Block.BlockId, Default1.Candidate.Metadata.Block.BlockId);
	Selector->EndSession();
	Selector->BeginSession(TAG_SelectionTarget);
	TestEqual(TEXT("Session resets ordinary exhaustion"), Select().Candidate.Metadata.Priority, 10);
	FDialogueBlockCandidate Top = MakeCandidate(TEXT("Top"), -100);
	Top.Metadata.bTopPriority = true;
	Candidates.Add(Top);
	TestEqual(TEXT("Top overrides ordinary priority"), Select().Candidate.Metadata.Block.BlockId, Top.Metadata.Block.BlockId);
	Selector->RecordCompleted(Top.Metadata.Block);
	TestEqual(TEXT("Top repeats while eligible"), Select().Candidate.Metadata.Block.BlockId, Top.Metadata.Block.BlockId);
	FDialogueBlockCandidate OtherTop = MakeCandidate(TEXT("OtherTop"), -100);
	OtherTop.Metadata.bTopPriority = true;
	Candidates.Add(OtherTop);
	bool bSameTopSeen = false;
	FGuid LastTop;
	for (int32 Index = 0; Index < 64; ++Index)
	{
		const FDialogueSelectionResult Pick = Select();
		bSameTopSeen |= Pick.Candidate.Metadata.Block.BlockId == LastTop;
		LastTop = Pick.Candidate.Metadata.Block.BlockId;
		Selector->RecordCompleted(Pick.Candidate.Metadata.Block);
	}
	TestTrue(TEXT("Top ties do not exclude previous"), bSameTopSeen);
	Candidates.Add(Top);
	TestFalse(TEXT("Ambiguous active identity rejected"), Select().bSelected);
	Selector->EndSession();
	Selector->BeginSession(TAG_SelectionTarget);
	Candidates = {MakeCandidate(TEXT("Interrupted"), 1), MakeCandidate(TEXT("Fallback"), 0, true)};
	const FDialogueSelectionResult Interrupted = Select();
	Selector->RecordExposed(Interrupted.Candidate.Metadata.Block);
	TestTrue(TEXT("Exposed incomplete ordinary excluded once"), Select().Candidate.Metadata.bDefault);
	return true;
}
#endif
