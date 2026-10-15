#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "DialogueSampleEffectProvider.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDialogueEffectSnapshotTest, "DialogueKit.Checkpoints.EffectSnapshot", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// 직렬화 후 원장 복원, 정상 반복, 실패 원자성과 스냅샷 불변성을 확인한다.
bool FDialogueEffectSnapshotTest::RunTest(const FString& Parameters)
{
	UDialogueSampleEffectProvider* Provider = NewObject<UDialogueSampleEffectProvider>();
	FDialogueStateDefinition Definition;
	Definition.Key.Kind = EDialogueStateKind::Count;
	Definition.Key.TargetId = TEXT("Player");
	Definition.Key.ItemId = TEXT("Coin");
	TestTrue(TEXT("Definition"), Provider->AddDefinition(Definition));
	FDialogueEffectBatch Batch;
	Batch.Context.ExecutionId = FGuid::NewGuid();
	FDialogueEffect Effect;
	Effect.Key = Definition.Key;
	Effect.Count = 10;
	Batch.Effects.Add(Effect);
	TestTrue(TEXT("First grant"), Provider->ApplyDialogueEffects_Implementation(Batch).IsSuccess());
	FDialogueSampleEffectSnapshot Snapshot = Provider->CaptureEffectSnapshot();
	TArray<uint8> Bytes;
	FMemoryWriter Writer(Bytes);
	FDialogueSampleEffectSnapshot::StaticStruct()->SerializeItem(Writer, &Snapshot, nullptr);
	FDialogueSampleEffectSnapshot Loaded;
	FMemoryReader Reader(Bytes);
	FDialogueSampleEffectSnapshot::StaticStruct()->SerializeItem(Reader, &Loaded, nullptr);
	FDialogueEffectBatch Repeat = Batch;
	Repeat.Context.ExecutionId = FGuid::NewGuid();
	TestTrue(TEXT("Normal repeat allowed"), Provider->ApplyDialogueEffects_Implementation(Repeat).IsSuccess());
	TestEqual(TEXT("Snapshot is immutable"), Snapshot.States[0].Count, int64(10));
	UDialogueSampleEffectProvider* Restored = NewObject<UDialogueSampleEffectProvider>();
	FName Error;
	TestTrue(TEXT("Restore"), Restored->RestoreEffectSnapshot(Loaded, Error));
	TestEqual(TEXT("Replay after restore"), Restored->ApplyDialogueEffects_Implementation(Batch).Status, EDialogueEffectStatus::AlreadyApplied);
	int64 Count = 0;
	Restored->QueryDialogueCount_Implementation(TEXT("Player"), TEXT("Coin"), Count);
	TestEqual(TEXT("No duplicate reward"), Count, int64(10));
	Loaded.Results.Reset();
	TestFalse(TEXT("Reject incomplete ledger"), Restored->RestoreEffectSnapshot(Loaded, Error));
	Restored->QueryDialogueCount_Implementation(TEXT("Player"), TEXT("Coin"), Count);
	TestEqual(TEXT("Failure keeps state"), Count, int64(10));
	TestEqual(TEXT("Failed restore keeps ledger"), Restored->ApplyDialogueEffects_Implementation(Batch).Status, EDialogueEffectStatus::AlreadyApplied);
	Loaded = Snapshot;
	Loaded.States.Reset();
	TestFalse(TEXT("Reject missing applied state"), Restored->RestoreEffectSnapshot(Loaded, Error));
	Loaded = Snapshot;
	const FDialogueEffectResult WrongKeyResult = Loaded.Results.FindChecked(Batch.Context.ExecutionId);
	Loaded.Results.Add(FGuid::NewGuid(), WrongKeyResult);
	Loaded.Results.Remove(Batch.Context.ExecutionId);
	TestFalse(TEXT("Reject mismatched ledger keys"), Restored->RestoreEffectSnapshot(Loaded, Error));
	Loaded = Snapshot;
	Loaded.SchemaVersion = 2;
	TestFalse(TEXT("Reject future schema"), Restored->RestoreEffectSnapshot(Loaded, Error));
	Loaded = Snapshot;
	const FDialogueSampleState DuplicateState = Loaded.States[0];
	Loaded.States.Add(DuplicateState);
	TestFalse(TEXT("Reject duplicate state"), Restored->RestoreEffectSnapshot(Loaded, Error));
	FDialogueEffectBatch Failed = Batch;
	Failed.Context.ExecutionId = FGuid::NewGuid();
	FDialogueEffect Remove = Effect;
	Remove.Operation = EDialogueEffectOperation::Remove;
	Remove.Count = 100;
	Failed.Effects.Add(Remove);
	TestFalse(TEXT("Whole batch fails"), Restored->ApplyDialogueEffects_Implementation(Failed).IsSuccess());
	Restored->QueryDialogueCount_Implementation(TEXT("Player"), TEXT("Coin"), Count);
	TestEqual(TEXT("First effect not partially applied"), Count, int64(10));
	TestTrue(TEXT("Failed ledger is restorable"), Provider->RestoreEffectSnapshot(Restored->CaptureEffectSnapshot(), Error));
	TestFalse(TEXT("Failed replay remains failed"), Provider->ApplyDialogueEffects_Implementation(Failed).IsSuccess());
	return true;
}
#endif
