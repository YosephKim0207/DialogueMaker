#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "DialogueCheckpointState.h"
#include "NativeGameplayTags.h"

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_HistoryTarget, "DialogueKit.Test.HistoryTarget");
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDialogueCheckpointStateTest, "DialogueKit.Checkpoints.HistoryAndAnchor", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// 임시 열람/누적 캐시/저장 재개점과 후속 선택 정리 범위를 구분한다.
bool FDialogueCheckpointStateTest::RunTest(const FString& Parameters)
{
	UDialogueCheckpointState* State = NewObject<UDialogueCheckpointState>();
	FDialogueBlockReference Block;
	Block.BlockId = FGuid::NewGuid();
	const FGuid First = FGuid::NewGuid();
	const FGuid Second = FGuid::NewGuid();
	TestTrue(TEXT("Begin"), State->BeginBlock(Block, TAG_HistoryTarget, FGuid::NewGuid()));
	TestTrue(TEXT("Visit"), State->VisitLine(First, 1));
	TestFalse(TEXT("Pending not committed read"), State->HasReadLine(Block.BlockId, First, 1));
	TestEqual(TEXT("Live cursor"), State->GetCurrentLine(), First);
	TestEqual(TEXT("No checkpoint means start"), State->GetResumeAnchor().Kind, EDialogueResumeKind::BlockStart);
	FDialogueExecutionBoundary Choice;
	Choice.Context.Block = Block;
	Choice.Context.NodeGuid = FGuid::NewGuid();
	Choice.ChoiceId = FGuid::NewGuid();
	Choice.NextNodeGuid = Second;
	Choice.bSaveRequested = true;
	FDialogueCheckpointSnapshot SavedChoice;
	TestTrue(TEXT("Choice boundary"), State->ApplyBoundary(Choice, SavedChoice));
	FName Error;
	TestTrue(TEXT("Valid snapshot"), SavedChoice.Validate(Error));
	TestTrue(TEXT("Read cache merged"), State->HasReadLine(Block.BlockId, First, 1));
	TestFalse(TEXT("Renewed line unread"), State->HasReadLine(Block.BlockId, First, 2));
	State->VisitLine(Second, 1);
	TestEqual(TEXT("Anchor stays checkpoint"), State->GetResumeAnchor().NodeGuid, Second);
	UDialogueCheckpointState* Restored = NewObject<UDialogueCheckpointState>();
	TestTrue(TEXT("Restore"), Restored->RestoreSnapshot(SavedChoice, Error));
	TestFalse(TEXT("Unsaved read not mixed"), Restored->HasReadLine(Block.BlockId, Second, 1));
	FDialogueExecutionBoundary End;
	End.Context.Block = Block;
	End.Context.NodeGuid = FGuid::NewGuid();
	End.bBlockCompleted = true;
	FDialogueCheckpointSnapshot EndSnapshot;
	TestTrue(TEXT("End without disk request"), State->ApplyBoundary(End, EndSnapshot));
	TestTrue(TEXT("End cache includes line"), State->HasReadLine(Block.BlockId, Second, 1));
	TestTrue(TEXT("End valid"), EndSnapshot.Validate(Error));
	TestEqual(TEXT("End retains choice until ack"), EndSnapshot.PendingChoices.Num(), 1);
	TestTrue(TEXT("Next run"), State->BeginBlock(Block, TAG_HistoryTarget, FGuid::NewGuid()));
	FDialogueCheckpointSnapshot Later;
	State->ApplyBoundary(Choice, Later);
	TestEqual(TEXT("Normal repeat gets another record"), Later.PendingChoices.Num(), 2);
	State->AcknowledgeSavedSnapshot(EndSnapshot);
	TestEqual(TEXT("Old End ack preserves later choice"), State->CaptureGameSave().PendingChoices.Num(), 1);
	State->ResetStory(false);
	TestTrue(TEXT("Death retains read"), State->HasReadLine(Block.BlockId, Second, 1));
	TestEqual(TEXT("Death clears active position"), State->GetResumeAnchor().Kind, EDialogueResumeKind::None);
	State->ResetStory(true);
	TestFalse(TEXT("New game clears read"), State->HasReadLine(Block.BlockId, First, 1));
	SavedChoice.PendingChoices.Add(FDialogueChoiceRecord());
	TestFalse(TEXT("Malformed choice record rejected"), Restored->RestoreSnapshot(SavedChoice, Error));
	TestTrue(TEXT("Failed restore keeps read"), Restored->HasReadLine(Block.BlockId, First, 1));
	return true;
}
#endif
