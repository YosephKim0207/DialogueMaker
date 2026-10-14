#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "DialogueSampleEffectProvider.h"
#include "Serialization/ObjectReader.h"
#include "Serialization/ObjectWriter.h"
#include <limits>
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDialogueEffectContractTest,
	"DialogueKit.Contracts.Effects.AtomicAndReplay",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// 부분 실패 무변경, 정상 반복과 동일 요청 재전달, 저장용 값 직렬화 경계를 검사한다.
bool FDialogueEffectContractTest::RunTest(const FString& Parameters)
{
	UDialogueSampleEffectProvider* Provider = NewObject<UDialogueSampleEffectProvider>();
	FDialogueEffect Gift;
	Gift.Key.Kind = EDialogueStateKind::Count;
	Gift.Key.ItemId = TEXT("Gift");
	Gift.Operation = EDialogueEffectOperation::Remove;
	Gift.Count = 1;
	FDialogueEffect Charm;
	Charm.Key.Kind = EDialogueStateKind::Value;
	Charm.Key.ItemId = TEXT("Charm");
	Charm.Value = 5.0;
	for (const FDialogueEffect& Effect : {Gift, Charm})
	{
		FDialogueStateDefinition Definition;
		Definition.Key = Effect.Key;
		Provider->AddDefinition(Definition);
	}
	FDialogueSampleState State;
	State.Key = Gift.Key;
	State.Count = 2;
	Provider->SetSampleState(State);
	FDialogueEffect Unknown = Charm;
	Unknown.Key.ItemId = TEXT("Unknown");
	FDialogueEffectBatch Failed;
	Failed.Context.ExecutionId = FGuid::NewGuid();
	Failed.Effects = {Gift, Unknown};
	TestFalse(TEXT("Late failure rejects batch"), Provider->ApplyDialogueEffects_Implementation(Failed).IsSuccess());
	int64 Count = 0;
	Provider->QueryDialogueCount_Implementation(NAME_None, TEXT("Gift"), Count);
	TestEqual(TEXT("No partial consumption"), Count, int64(2));
	FDialogueStateDefinition Added;
	Added.Key = Unknown.Key;
	Provider->AddDefinition(Added);
	TestFalse(TEXT("Failed execution replay stays failed"), Provider->ApplyDialogueEffects_Implementation(Failed).IsSuccess());
	FDialogueEffectBatch Batch;
	Batch.Context.ExecutionId = FGuid::NewGuid();
	Batch.Effects = {Gift, Charm};
	TestTrue(TEXT("Valid batch applied"), UDialogueEffectLibrary::ApplyEffectBatch(Batch, Provider, nullptr).IsSuccess());
	TestTrue(TEXT("Replay already applied"), Provider->ApplyDialogueEffects_Implementation(Batch).Status == EDialogueEffectStatus::AlreadyApplied);
	Provider->QueryDialogueCount_Implementation(NAME_None, TEXT("Gift"), Count);
	TestEqual(TEXT("One consumption only"), Count, int64(1));
	FDialogueEffectBatch Conflict = Batch;
	Conflict.Effects[1].Value = 6.0;
	TestEqual(TEXT("Different payload rejected"), Provider->ApplyDialogueEffects_Implementation(Conflict).ErrorCode, FName(TEXT("ExecutionPayloadConflict")));
	Conflict = Batch;
	Conflict.Context.NodeGuid = FGuid::NewGuid();
	TestFalse(TEXT("Different location rejected"), Provider->ApplyDialogueEffects_Implementation(Conflict).IsSuccess());
	Batch.Context.SaveRequestId = FGuid::NewGuid();
	Batch.Effects[0].Value = 999;
	TestTrue(TEXT("Inactive value and save tracing ignored"), Provider->ApplyDialogueEffects_Implementation(Batch).Status == EDialogueEffectStatus::AlreadyApplied);
	TArray<uint8> Bytes;
	FObjectWriter Writer(Provider, Bytes);
	UDialogueSampleEffectProvider* Restored = NewObject<UDialogueSampleEffectProvider>();
	FObjectReader Reader(Restored, Bytes);
	TestTrue(TEXT("Memory ledger replay remains applied"), Restored->ApplyDialogueEffects_Implementation(Batch).Status == EDialogueEffectStatus::AlreadyApplied);
	Restored->QueryDialogueCount_Implementation(NAME_None, TEXT("Gift"), Count);
	TestEqual(TEXT("State and ledger roundtrip"), Count, int64(1));
	FMapProperty* Requests = FindFProperty<FMapProperty>(Restored->GetClass(), TEXT("Requests"));
	FMapProperty* Results = FindFProperty<FMapProperty>(Restored->GetClass(), TEXT("Results"));
	if (TestNotNull(TEXT("Request ledger reflected"), Requests) && TestNotNull(TEXT("Result ledger reflected"), Results))
	{
		Requests->ClearValue_InContainer(Restored);
		TestEqual(TEXT("Result-only ledger fails closed"), Restored->ApplyDialogueEffects_Implementation(Batch).ErrorCode, FName(TEXT("InvalidExecutionLedger")));
		Restored->QueryDialogueCount_Implementation(NAME_None, TEXT("Gift"), Count);
		TestEqual(TEXT("No duplicate after ledger damage"), Count, int64(1));
		FObjectReader RestoreAgain(Restored, Bytes);
		Results->ClearValue_InContainer(Restored);
		TestEqual(TEXT("Request-only ledger fails closed"), Restored->ApplyDialogueEffects_Implementation(Batch).ErrorCode, FName(TEXT("InvalidExecutionLedger")));
	}
	Batch.Context.ExecutionId = FGuid::NewGuid();
	TestTrue(TEXT("New occurrence allowed"), Provider->ApplyDialogueEffects_Implementation(Batch).IsSuccess());
	double Value = 0;
	Provider->QueryDialogueValue_Implementation(NAME_None, TEXT("Charm"), Value);
	TestEqual(TEXT("Normal repeated reward"), Value, 10.0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDialogueEffectBoundariesTest,
	"DialogueKit.Contracts.Effects.Boundaries",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// 효과 순서와 int64/부동소수점 경계에서 실제 상태가 손상되지 않는지 검사한다.
bool FDialogueEffectBoundariesTest::RunTest(const FString& Parameters)
{
	UDialogueSampleEffectProvider* Provider = NewObject<UDialogueSampleEffectProvider>();
	FDialogueEffect Effect;
	Effect.Key.Kind = EDialogueStateKind::Count;
	Effect.Key.ItemId = TEXT("Coin");
	FDialogueStateDefinition Definition;
	Definition.Key = Effect.Key;
	Provider->AddDefinition(Definition);
	FDialogueEffectBatch Batch;
	Batch.Context.ExecutionId = FGuid::NewGuid();
	Effect.Operation = EDialogueEffectOperation::Set;
	Effect.Count = MAX_int64;
	Batch.Effects = {Effect};
	Effect.Operation = EDialogueEffectOperation::Add;
	Effect.Count = 1;
	Batch.Effects.Add(Effect);
	TestFalse(TEXT("Overflow after set rejects entire batch"), Provider->ApplyDialogueEffects_Implementation(Batch).IsSuccess());
	int64 Count = -1;
	Provider->QueryDialogueCount_Implementation(NAME_None, TEXT("Coin"), Count);
	TestEqual(TEXT("No partial maximum assignment"), Count, int64(0));
	Batch.Context.ExecutionId = FGuid::NewGuid();
	Batch.Effects[0].Count = 4;
	TestTrue(TEXT("Set then add succeeds"), Provider->ApplyDialogueEffects_Implementation(Batch).IsSuccess());
	Provider->QueryDialogueCount_Implementation(NAME_None, TEXT("Coin"), Count);
	TestEqual(TEXT("Same-key order applied"), Count, int64(5));
	Batch.Effects.Swap(0, 1);
	TestFalse(TEXT("Order is part of payload"), Provider->ApplyDialogueEffects_Implementation(Batch).IsSuccess());
	Batch.Context.ExecutionId = FGuid::NewGuid();
	Effect.Operation = EDialogueEffectOperation::Remove;
	Effect.Count = 6;
	Batch.Effects = {Effect};
	TestFalse(TEXT("Underflow rejected"), Provider->ApplyDialogueEffects_Implementation(Batch).IsSuccess());
	Batch.Effects[0].Count = -1;
	TestFalse(TEXT("Negative amount invalid"), Batch.IsValid());
	Effect.Key.Kind = EDialogueStateKind::Value;
	Definition.Key = Effect.Key;
	Provider->AddDefinition(Definition);
	Effect.Operation = EDialogueEffectOperation::Set;
	Effect.Value = std::numeric_limits<double>::max();
	Batch.Context.ExecutionId = FGuid::NewGuid();
	Batch.Effects = {Effect};
	Effect.Operation = EDialogueEffectOperation::Add;
	Batch.Effects.Add(Effect);
	TestFalse(TEXT("Nonfinite result rejected"), Provider->ApplyDialogueEffects_Implementation(Batch).IsSuccess());
	Batch.Effects[0].Value = std::numeric_limits<double>::quiet_NaN();
	TestFalse(TEXT("NaN operand invalid"), Batch.IsValid());
	Batch.Effects.Reset();
	TestFalse(TEXT("Empty batch invalid"), Batch.IsValid());
	return true;
}
#endif
