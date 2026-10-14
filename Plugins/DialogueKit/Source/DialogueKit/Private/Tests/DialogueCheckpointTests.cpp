#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "DialogueSampleCheckpointProvider.h"
#include "NativeGameplayTags.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_DialogueCheckpointTarget, "DialogueKit.Test.CheckpointTarget");

// 실제 게임/파일 없이 유효한 선택 직후 스냅샷을 만든다.
static FDialogueCheckpointRequest MakeCheckpointRequest()
{
	FDialogueCheckpointRequest Request;
	Request.RequestId = FGuid::NewGuid();
	Request.QueueGenerationId = FGuid::NewGuid();
	Request.Sequence = 1;
	Request.Snapshot.CheckpointId = FGuid::NewGuid();
	Request.Snapshot.Resume.Kind = EDialogueResumeKind::BeforeNode;
	Request.Snapshot.Resume.Block.BlockId = FGuid::NewGuid();
	Request.Snapshot.Resume.BlockRunId = FGuid::NewGuid();
	Request.Snapshot.Resume.NodeGuid = FGuid::NewGuid();
	Request.Snapshot.Resume.TargetTag = TAG_DialogueCheckpointTarget;
	Request.Snapshot.Trigger.Reason = EDialogueCheckpointReason::Choice;
	Request.Snapshot.Trigger.OriginNodeGuid = FGuid::NewGuid();
	Request.Snapshot.Trigger.ChoiceId = FGuid::NewGuid();
	FDialogueBlockReadRecord Read;
	Read.BlockId = Request.Snapshot.Resume.Block.BlockId;
	FDialogueLineReadRecord Line;
	Line.NodeGuid = FGuid::NewGuid();
	Line.ReadVersion = 2;
	Read.Lines.Add(Line);
	Request.Snapshot.FullReadHistory.Add(Read);
	return Request;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDialogueCheckpointValidationTest,
	"DialogueKit.Contracts.Checkpoints.Validation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// 스키마/트리거/열람 불변식과 비동기 결과의 상관관계를 검사한다.
bool FDialogueCheckpointValidationTest::RunTest(const FString& Parameters)
{
	FDialogueCheckpointRequest Request = MakeCheckpointRequest();
	FName Error;
	TestTrue(TEXT("Optional choice save valid"), Request.Validate(Error));
	Request.Snapshot.Trigger.bHasEffects = true;
	TestFalse(TEXT("Effect requires execution"), Request.Validate(Error));
	Request.Snapshot.Trigger.ExecutionId = FGuid::NewGuid();
	TestTrue(TEXT("Effect choice valid"), Request.Validate(Error));
	Request.Snapshot.Trigger.Reason = EDialogueCheckpointReason::StateChange;
	TestFalse(TEXT("StateChange cannot have ChoiceId"), Request.Validate(Error));
	Request.Snapshot.Trigger.ChoiceId.Invalidate();
	TestTrue(TEXT("StateChange valid"), Request.Validate(Error));
	Request.Snapshot.Trigger = FDialogueCheckpointTrigger();
	Request.Snapshot.Trigger.Reason = EDialogueCheckpointReason::BlockEnd;
	Request.Snapshot.Trigger.OriginNodeGuid = FGuid::NewGuid();
	TestFalse(TEXT("End needs completed cursor"), Request.Validate(Error));
	Request.Snapshot.Resume.Kind = EDialogueResumeKind::BlockCompleted;
	Request.Snapshot.Resume.NodeGuid.Invalidate();
	TestFalse(TEXT("Completed cursor requires completed history"), Request.Validate(Error));
	Request.Snapshot.FullReadHistory[0].bCompleted = true;
	TestTrue(TEXT("Completed valid"), Request.Validate(Error));
	Request.Snapshot.Trigger = FDialogueCheckpointTrigger();
	Request.Snapshot.Trigger.Reason = EDialogueCheckpointReason::Replacement;
	Request.Snapshot.Trigger.ReplacedFromBlock.BlockId = FGuid::NewGuid();
	Request.Snapshot.Resume.Kind = EDialogueResumeKind::BlockStart;
	TestTrue(TEXT("Replacement starts at block start"), Request.Validate(Error));
	Request.Snapshot.Trigger = FDialogueCheckpointTrigger();
	Request.Snapshot.Resume = FDialogueResumePosition();
	TestTrue(TEXT("Game save with no active dialogue"), Request.Validate(Error));
	Request.Snapshot.SchemaVersion = 2;
	TestFalse(TEXT("Unknown schema rejected"), Request.Validate(Error));
	Request.Snapshot.SchemaVersion = 0;
	TestFalse(TEXT("Zero schema rejected"), Request.Validate(Error));
	Request = MakeCheckpointRequest();
	FDialogueBlockReadRecord Duplicate = Request.Snapshot.FullReadHistory[0];
	Request.Snapshot.FullReadHistory.Add(Duplicate);
	TestFalse(TEXT("Duplicate block history rejected"), Request.Validate(Error));
	Request.Snapshot.FullReadHistory.Pop();
	FDialogueLineReadRecord DuplicateLine = Duplicate.Lines[0];
	Request.Snapshot.FullReadHistory[0].Lines.Add(DuplicateLine);
	TestFalse(TEXT("Duplicate line history rejected"), Request.Validate(Error));
	Request.Snapshot.FullReadHistory[0].Lines.Pop();
	Request.Snapshot.FullReadHistory[0].Lines[0].ReadVersion = 0;
	TestFalse(TEXT("Zero read version rejected"), Request.Validate(Error));
	Request = MakeCheckpointRequest();
	FDialogueCheckpointCompletion Completion;
	Completion.RequestId = Request.RequestId;
	Completion.QueueGenerationId = Request.QueueGenerationId;
	Completion.Sequence = Request.Sequence;
	Completion.bSaved = true;
	TestTrue(TEXT("Exact completion matches"), UDialogueCheckpointLibrary::MatchesCompletion(Request, Completion));
	Completion.QueueGenerationId = FGuid::NewGuid();
	TestFalse(TEXT("Old generation rejected"), UDialogueCheckpointLibrary::MatchesCompletion(Request, Completion));
	Completion.QueueGenerationId = Request.QueueGenerationId;
	Completion.RequestId = FGuid::NewGuid();
	TestFalse(TEXT("Old attempt rejected"), UDialogueCheckpointLibrary::MatchesCompletion(Request, Completion));
	Completion.RequestId = Request.RequestId;
	Completion.Sequence++;
	TestFalse(TEXT("Wrong sequence rejected"), UDialogueCheckpointLibrary::MatchesCompletion(Request, Completion));
	Completion.Sequence = Request.Sequence;
	Completion.bSaved = false;
	TestFalse(TEXT("Failure needs error"), UDialogueCheckpointLibrary::MatchesCompletion(Request, Completion));
	Completion.ErrorCode = TEXT("DiskFailure");
	TestTrue(TEXT("Correlated failure is not save success"), UDialogueCheckpointLibrary::MatchesCompletion(Request, Completion));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDialogueCheckpointCopyTest,
	"DialogueKit.Contracts.Checkpoints.CopyAndRoundtrip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// 접수 값의 격리와 DTO 전체 왕복을 검증하며 실제 파일 저장은 하지 않는다.
bool FDialogueCheckpointCopyTest::RunTest(const FString& Parameters)
{
	FDialogueCheckpointRequest Request = MakeCheckpointRequest();
	UDialogueSampleCheckpointProvider* Provider = NewObject<UDialogueSampleCheckpointProvider>();
	TestTrue(TEXT("Accepted only"), UDialogueCheckpointLibrary::SubmitCheckpoint(Request, Provider, nullptr).bAccepted);
	Request.Snapshot.FullReadHistory[0].Lines[0].ReadVersion = 8;
	TestEqual(TEXT("Captured copy isolated"), Provider->CapturedRequests[0].Snapshot.FullReadHistory[0].Lines[0].ReadVersion, 2);
	TArray<uint8> Bytes;
	FMemoryWriter Writer(Bytes);
	FDialogueCheckpointRequest::StaticStruct()->SerializeItem(Writer, &Request, nullptr);
	FDialogueCheckpointRequest Restored;
	FMemoryReader Reader(Bytes);
	FDialogueCheckpointRequest::StaticStruct()->SerializeItem(Reader, &Restored, nullptr);
	FName Error;
	TestTrue(TEXT("Restored valid"), Restored.Validate(Error));
	TestTrue(TEXT("All fields reflected roundtrip"), FDialogueCheckpointRequest::StaticStruct()->CompareScriptStruct(&Request, &Restored, 0));
	TestFalse(TEXT("Missing provider rejected"), UDialogueCheckpointLibrary::SubmitCheckpoint(Request, nullptr, nullptr).bAccepted);
	Request.Sequence = 0;
	TestFalse(TEXT("Invalid request rejected before provider"), UDialogueCheckpointLibrary::SubmitCheckpoint(Request, Provider, nullptr).bAccepted);
	TestEqual(TEXT("Rejected request not captured"), Provider->CapturedRequests.Num(), 1);
	struct FDefaultProvider : IDialogueCheckpointProvider {};
	FDefaultProvider DefaultProvider;
	TestFalse(TEXT("Unimplemented default fails"), DefaultProvider.RequestDialogueCheckpoint_Implementation(Restored).bAccepted);
	return true;
}
#endif
