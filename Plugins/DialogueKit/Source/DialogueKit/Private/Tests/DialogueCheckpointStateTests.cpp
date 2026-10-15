#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "DialogueCheckpointState.h"
#include "NativeGameplayTags.h"
#include "DialogueSampleSaveCoordinator.h"

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
	TestTrue(TEXT("Pass current line"), State->PassLine(First));
	TestTrue(TEXT("Repeated passage deduplicated"), State->PassLine(First));
	FDialogueExecutionBoundary Choice;
	Choice.Context.Block = Block;
	Choice.Context.BlockRunId = State->GetResumeAnchor().BlockRunId;
	Choice.Context.NodeGuid = FGuid::NewGuid();
	Choice.ChoiceId = FGuid::NewGuid();
	Choice.NextNodeGuid = Second;
	Choice.bSaveRequested = true;
	FDialogueCheckpointSnapshot SavedChoice;
	TestTrue(TEXT("Choice boundary"), State->ApplyBoundary(Choice, SavedChoice));
	FName Error;
	TestTrue(TEXT("Valid snapshot"), SavedChoice.Validate(Error));
	TestEqual(TEXT("New runtime schema"), SavedChoice.SchemaVersion, 2);
	TestEqual(TEXT("Unique passage"), SavedChoice.RunEvidence.PassedLines.Num(), 1);
	FDialogueCheckpointSnapshot MissingChoice = SavedChoice;
	MissingChoice.PendingChoices.Reset();
	TestFalse(TEXT("Missing trigger choice rejected"), MissingChoice.Validate(Error));
	TestTrue(TEXT("Read cache merged"), State->HasReadLine(Block.BlockId, First, 1));
	TestFalse(TEXT("Renewed line unread"), State->HasReadLine(Block.BlockId, First, 2));
	State->VisitLine(Second, 1);
	TestEqual(TEXT("Anchor stays checkpoint"), State->GetResumeAnchor().NodeGuid, Second);
	UDialogueCheckpointState* Restored = NewObject<UDialogueCheckpointState>();
	TestTrue(TEXT("Restore"), Restored->RestoreSnapshot(SavedChoice, Error));
	TestFalse(TEXT("Unsaved read not mixed"), Restored->HasReadLine(Block.BlockId, Second, 1));
	FDialogueExecutionBoundary End;
	End.Context.Block = Block;
	End.Context.BlockRunId = State->GetResumeAnchor().BlockRunId;
	End.Context.NodeGuid = FGuid::NewGuid();
	End.bBlockCompleted = true;
	FDialogueCheckpointSnapshot EndSnapshot;
	TestTrue(TEXT("End without disk request"), State->ApplyBoundary(End, EndSnapshot));
	TestTrue(TEXT("End cache includes line"), State->HasReadLine(Block.BlockId, Second, 1));
	TestTrue(TEXT("End valid"), EndSnapshot.Validate(Error));
	TestEqual(TEXT("End retains choice until ack"), EndSnapshot.PendingChoices.Num(), 1);
	UDialogueCheckpointState* EndRestored = NewObject<UDialogueCheckpointState>();
	TestTrue(TEXT("Restore durable End"), EndRestored->RestoreSnapshot(EndSnapshot, Error));
	TestTrue(TEXT("End does not resurrect pending choices"), EndRestored->CaptureGameSave().PendingChoices.IsEmpty());
	TestTrue(TEXT("Next run"), State->BeginBlock(Block, TAG_HistoryTarget, FGuid::NewGuid()));
	Choice.Context.BlockRunId = State->GetResumeAnchor().BlockRunId;
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
	FDialogueCheckpointSnapshot Legacy = SavedChoice;
	Legacy.SchemaVersion = 1;
	Legacy.PendingChoices.Reset();
	Legacy.RunEvidence = FDialogueRunEvidence();
	FDialogueCheckpointSnapshot Converted;
	TestTrue(TEXT("Legacy upgrade"), Legacy.UpgradeToLatest(Converted, Error));
	TestFalse(TEXT("Legacy history remains unknown"), Converted.RunEvidence.bComplete);
	TestTrue(TEXT("No invented passage"), Converted.RunEvidence.PassedLines.IsEmpty());
	TestEqual(TEXT("Known last choice migrated"), Converted.PendingChoices.Num(), 1);
	FDialogueCheckpointSnapshot Again;
	Legacy.UpgradeToLatest(Again, Error);
	TestEqual(TEXT("Migration occurrence deterministic"), Again.PendingChoices[0].OccurrenceId, Converted.PendingChoices[0].OccurrenceId);
	SavedChoice.PendingChoices.Add(FDialogueChoiceRecord());
	TestFalse(TEXT("Malformed choice record rejected"), Restored->RestoreSnapshot(SavedChoice, Error));
	TestTrue(TEXT("Failed restore keeps read"), Restored->HasReadLine(Block.BlockId, First, 1));
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDialogueEvidenceTest, "DialogueKit.Checkpoints.RunEvidence", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// 효과 성공 기록은 이번 실행의 통과 기록과 묶이고 실패 입력은 캐시를 변경하지 않는다.
bool FDialogueEvidenceTest::RunTest(const FString& Parameters)
{
	UDialogueCheckpointState* State = NewObject<UDialogueCheckpointState>();
	FDialogueBlockReference Block;
	Block.BlockId = FGuid::NewGuid();
	State->BeginBlock(Block, TAG_HistoryTarget, FGuid::NewGuid());
	const FGuid Line = FGuid::NewGuid();
	State->VisitLine(Line, 1);
	State->PassLine(Line);
	FDialogueExecutionBoundary Effect;
	Effect.Context.Block = Block;
	Effect.Context.BlockRunId = State->GetResumeAnchor().BlockRunId;
	Effect.Context.NodeGuid = FGuid::NewGuid();
	Effect.Context.ExecutionId = FGuid::NewGuid();
	Effect.NextNodeGuid = FGuid::NewGuid();
	Effect.bHasEffects = true;
	Effect.bSaveRequested = true;
	FDialogueCheckpointSnapshot Snapshot;
	TestTrue(TEXT("Effect boundary"), State->ApplyBoundary(Effect, Snapshot));
	FName Error;
	TestTrue(TEXT("Evidence valid"), Snapshot.Validate(Error));
	TestEqual(TEXT("Related line"), Snapshot.RunEvidence.Effects[0].LineGuid, Line);
	TestTrue(TEXT("Complete current-run evidence"), Snapshot.RunEvidence.bComplete);
	UDialogueSampleEffectProvider* Provider = NewObject<UDialogueSampleEffectProvider>();
	FDialogueStateDefinition Definition;
	Definition.Key.Kind = EDialogueStateKind::Count;
	Definition.Key.TargetId = TEXT("Player");
	Definition.Key.ItemId = TEXT("Coin");
	Provider->AddDefinition(Definition);
	FDialogueEffectBatch Batch;
	Batch.Context = Effect.Context;
	FDialogueEffect Reward;
	Reward.Key = Definition.Key;
	Reward.Count = 1;
	Batch.Effects.Add(Reward);
	Provider->ApplyDialogueEffects_Implementation(Batch);
	UDialogueSampleCombinedSave* Combined = NewObject<UDialogueSampleCombinedSave>();
	Combined->Revision = 1;
	Combined->Request.RequestId = FGuid::NewGuid();
	Combined->Request.QueueGenerationId = FGuid::NewGuid();
	Combined->Request.Sequence = 1;
	Combined->Request.Snapshot = Snapshot;
	Combined->Game = Provider->CaptureEffectSnapshot();
	TestTrue(TEXT("Matching game and dialogue evidence"), Combined->Validate());
	Combined->Game.Requests.FindChecked(Batch.Context.ExecutionId).Context.BlockRunId = FGuid::NewGuid();
	TestFalse(TEXT("Past run ledger cannot prove current run"), Combined->Validate());
	Combined->Game = Provider->CaptureEffectSnapshot();
	Combined->Game.Requests.FindChecked(Batch.Context.ExecutionId).ChoiceId = FGuid::NewGuid();
	TestFalse(TEXT("Different choice ledger rejected"), Combined->Validate());
	FDialogueEffectBatch Conflicting = Batch;
	Conflicting.Context.BlockRunId = FGuid::NewGuid();
	TestFalse(TEXT("Same execution with different run is not replay"), Batch.HasSamePayload(Conflicting));
	TestFalse(TEXT("Duplicate execution rejected"), State->ApplyBoundary(Effect, Snapshot));
	TestEqual(TEXT("Rejected input preserves occurrence count"), State->CaptureGameSave().RunEvidence.Effects.Num(), 1);
	Snapshot.RunEvidence.Effects.Reset();
	TestFalse(TEXT("Effect trigger without evidence rejected"), Snapshot.Validate(Error));
	State->ResetStory(false);
	State->BeginBlock(Block, TAG_HistoryTarget, FGuid::NewGuid());
	TestFalse(TEXT("Previous run boundary rejected"), State->ApplyBoundary(Effect, Snapshot));
	Effect.Context.BlockRunId = State->GetResumeAnchor().BlockRunId;
	TestTrue(TEXT("New run without prior line"), State->ApplyBoundary(Effect, Snapshot));
	TestFalse(TEXT("No fabricated line for autonomous effect"), Snapshot.RunEvidence.Effects[0].LineGuid.IsValid());
	return true;
}
#endif
