#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "DialogueSaveQueue.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDialogueSaveQueueTest, "DialogueKit.Checkpoints.ImmutableFIFO", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// 실패/역전/재시도에서도 같은 시점 자료와 FIFO 순서를 유지하는지 검사한다.
bool FDialogueSaveQueueTest::RunTest(const FString& Parameters)
{
	UDialogueSaveQueue* Queue = NewObject<UDialogueSaveQueue>();
	const FGuid Generation = FGuid::NewGuid();
	TestTrue(TEXT("Initialize"), Queue->InitializeQueue(Generation));
	TestFalse(TEXT("Same generation cannot reset IDs"), Queue->InitializeQueue(Generation));
	TArray<FDialogueQueuedSave> Writes;
	Queue->OnWriteRequestedNative.BindLambda([&](const FDialogueQueuedSave& Save) { Writes.Add(Save); });
	FDialogueCheckpointRequest A;
	A.RequestId = FGuid::NewGuid();
	A.QueueGenerationId = Generation;
	A.Sequence = 1;
	A.Snapshot.CheckpointId = FGuid::NewGuid();
	TArray<uint8> Game = {10};
	TestTrue(TEXT("Accept A"), Queue->Enqueue(A, Game).bAccepted);
	Game[0] = 20;
	FDialogueCheckpointRequest B = A;
	B.Sequence = 2;
	B.RequestId = FGuid::NewGuid();
	B.Snapshot.CheckpointId = FGuid::NewGuid();
	TestTrue(TEXT("Accept B"), Queue->Enqueue(B, Game).bAccepted);
	TestEqual(TEXT("Only A started"), Writes.Num(), 1);
	TestEqual(TEXT("A immutable bytes"), Writes[0].GamePayload[0], uint8(10));
	FDialogueCheckpointCompletion Completion;
	Completion.RequestId = B.RequestId;
	Completion.QueueGenerationId = Generation;
	Completion.Sequence = 2;
	Completion.bSaved = true;
	TestFalse(TEXT("Out of order ignored"), Queue->CompleteWrite(Completion));
	Completion.RequestId = A.RequestId;
	Completion.Sequence = 1;
	Completion.bSaved = false;
	Completion.ErrorCode = TEXT("InjectedFailure");
	TestTrue(TEXT("A fails"), Queue->CompleteWrite(Completion));
	TestTrue(TEXT("Failure retained"), Queue->HasFailedWrite());
	TestEqual(TEXT("Both retained"), Queue->GetPendingCount(), 2);
	TestFalse(TEXT("Cannot discard failed queue"), Queue->InitializeQueue(FGuid::NewGuid()));
	TestTrue(TEXT("Retry"), Queue->RetryFailedWrite());
	TestEqual(TEXT("Retry before B"), Writes.Num(), 2);
	TestEqual(TEXT("Same snapshot"), Writes[1].Request.Snapshot.CheckpointId, A.Snapshot.CheckpointId);
	TestEqual(TEXT("Same game state"), Writes[1].GamePayload[0], uint8(10));
	Completion.bSaved = true;
	Completion.ErrorCode = NAME_None;
	TestFalse(TEXT("Old attempt ignored"), Queue->CompleteWrite(Completion));
	Completion.RequestId = Writes[1].Request.RequestId;
	TestTrue(TEXT("Retry succeeds"), Queue->CompleteWrite(Completion));
	TestEqual(TEXT("B now starts"), Writes.Num(), 3);
	TestEqual(TEXT("B retained"), Queue->GetPendingCount(), 1);
	TestEqual(TEXT("Durable A only"), Queue->GetLastSuccessfulSave().GamePayload[0], uint8(10));
	Completion.RequestId = B.RequestId;
	Completion.Sequence = 2;
	TestTrue(TEXT("B succeeds"), Queue->CompleteWrite(Completion));
	TestFalse(TEXT("Duplicate ignored"), Queue->CompleteWrite(Completion));
	TestEqual(TEXT("Drained"), Queue->GetPendingCount(), 0);
	TestTrue(TEXT("Reset generation"), Queue->InitializeQueue(FGuid::NewGuid()));
	TestFalse(TEXT("Old generation request rejected"), Queue->Enqueue(A, Game).bAccepted);
	UDialogueSaveQueue* Immediate = NewObject<UDialogueSaveQueue>();
	Immediate->InitializeQueue(Generation);
	Immediate->OnWriteRequestedNative.BindLambda([&](const FDialogueQueuedSave& Save)
	{
		FDialogueCheckpointCompletion Done;
		Done.RequestId = Save.Request.RequestId;
		Done.QueueGenerationId = Save.Request.QueueGenerationId;
		Done.Sequence = Save.Request.Sequence;
		Done.bSaved = true;
		Immediate->CompleteWrite(Done);
	});
	TestTrue(TEXT("Immediate completion accepted"), Immediate->Enqueue(A, Game).bAccepted);
	TestEqual(TEXT("Immediate drained"), Immediate->GetPendingCount(), 0);
	UDialogueSaveQueue* NoWriter = NewObject<UDialogueSaveQueue>();
	NoWriter->InitializeQueue(Generation);
	TestTrue(TEXT("Missing writer keeps accepted data"), NoWriter->Enqueue(A, Game).bAccepted);
	TestTrue(TEXT("Missing writer reported as failure"), NoWriter->HasFailedWrite());
	TestEqual(TEXT("Missing writer retains head"), NoWriter->GetPendingCount(), 1);
	return true;
}
#endif
