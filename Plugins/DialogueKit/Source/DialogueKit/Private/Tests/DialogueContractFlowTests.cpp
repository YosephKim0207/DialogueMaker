#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "DialogueGraph.h"
#include "DialogueNodeInfo.h"
#include "DialogueConditionLibrary.h"
#include "DialogueSampleEffectProvider.h"
#include "DialogueSampleCheckpointProvider.h"
#include "NativeGameplayTags.h"
#include "Serialization/ObjectReader.h"
#include "Serialization/ObjectWriter.h"

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_DialogueContractFlowTarget, "DialogueKit.Test.ContractFlowTarget");

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDialogueContractFlowTest,
	"DialogueKit.Contracts.Flow.StateEffectCheckpoint",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// 하나의 Block/선택 정체성을 조건·효과·저장 계약에 전달하는 메모리 연결 예제다.
bool FDialogueContractFlowTest::RunTest(const FString& Parameters)
{
	UDialogueGraph* Graph = NewObject<UDialogueGraph>();
	Graph->InitializeBlockIdentity();
	UDialogueRuntimeNode* ChoiceNode = NewObject<UDialogueRuntimeNode>(Graph);
	ChoiceNode->NodeGuid = FGuid::NewGuid();
	UDialogueNodeInfo* Info = NewObject<UDialogueNodeInfo>(ChoiceNode);
	Info->AddDialogueChoice(FDialogueChoice());
	if (TestEqual(TEXT("Choice created"), Info->GetDialogueChoices().Num(), 1) == false)
	{
		return false;
	}
	ChoiceNode->NodeInfo = Info;
	UDialogueRuntimeNode* NextLine = NewObject<UDialogueRuntimeNode>(Graph);
	NextLine->NodeGuid = FGuid::NewGuid();
	UDialogueSampleEffectProvider* Game = NewObject<UDialogueSampleEffectProvider>();
	FDialogueStateDefinition Item;
	Item.Key.Kind = EDialogueStateKind::Count;
	Item.Key.ItemId = TEXT("Key");
	Game->AddDefinition(Item);
	FDialogueSampleState Initial;
	Initial.Key = Item.Key;
	Initial.Count = 1;
	Game->SetSampleState(Initial);
	FDialogueDiagnosticContext Context;
	Context.Block = Graph->GetBlockReference();
	Context.NodeGuid = ChoiceNode->NodeGuid;
	Context.ExecutionId = FGuid::NewGuid();
	FDialogueCondition Condition;
	Condition.Key = Item.Key;
	Condition.Comparison = EDialogueComparison::GreaterOrEqual;
	Condition.CountOperand = 1;
	FDialogueConditionSet Conditions;
	Conditions.Conditions.Add(Condition);
	TestTrue(TEXT("Game query permits choice"), UDialogueConditionLibrary::EvaluateConditions(Conditions, Game, false, Context, nullptr).bPassed);
	FDialogueEffect Effect;
	Effect.Key = Item.Key;
	Effect.Operation = EDialogueEffectOperation::Remove;
	Effect.Count = 1;
	FDialogueEffectBatch Batch;
	Batch.Context = Context;
	Batch.Effects.Add(Effect);
	TestTrue(TEXT("Choice effect succeeds"), UDialogueEffectLibrary::ApplyEffectBatch(Batch, Game, nullptr).IsSuccess());
	FDialogueCheckpointRequest Request;
	Request.RequestId = FGuid::NewGuid();
	Request.QueueGenerationId = FGuid::NewGuid();
	Request.Sequence = 1;
	Request.Snapshot.CheckpointId = FGuid::NewGuid();
	Request.Snapshot.Resume.Kind = EDialogueResumeKind::BeforeNode;
	Request.Snapshot.Resume.Block = Context.Block;
	Request.Snapshot.Resume.BlockRunId = FGuid::NewGuid();
	Request.Snapshot.Resume.NodeGuid = NextLine->NodeGuid;
	Request.Snapshot.Resume.TargetTag = TAG_DialogueContractFlowTarget;
	Request.Snapshot.Trigger.Reason = EDialogueCheckpointReason::Choice;
	Request.Snapshot.Trigger.OriginNodeGuid = Context.NodeGuid;
	Request.Snapshot.Trigger.ChoiceId = Info->GetDialogueChoices()[0].ChoiceId;
	Request.Snapshot.Trigger.ExecutionId = Context.ExecutionId;
	Request.Snapshot.Trigger.bHasEffects = true;
	FDialogueBlockReadRecord BlockRead;
	BlockRead.BlockId = Graph->GetBlockId();
	FDialogueLineReadRecord Read;
	Read.NodeGuid = ChoiceNode->NodeGuid;
	Read.ReadVersion = Info->GetReadVersion();
	BlockRead.Lines.Add(Read);
	Request.Snapshot.FullReadHistory.Add(BlockRead);
	UDialogueSampleCheckpointProvider* Save = NewObject<UDialogueSampleCheckpointProvider>();
	// 실제 게임은 provider 내부에서 게임 상태와 요청을 같은 저장 단위로 복사한다.
	TArray<uint8> CapturedGame;
	FObjectWriter CaptureGame(Game, CapturedGame);
	TestTrue(TEXT("Dialogue snapshot accepted"), UDialogueCheckpointLibrary::SubmitCheckpoint(Request, Save, nullptr).bAccepted);
	if (TestEqual(TEXT("One request captured"), Save->CapturedRequests.Num(), 1) == false)
	{
		return false;
	}
	Request.Snapshot.Resume.NodeGuid = FGuid::NewGuid();
	Request.Snapshot.Trigger.ChoiceId = FGuid::NewGuid();
	Request.Snapshot.FullReadHistory.Reset();
	Initial.Count = 99;
	Game->SetSampleState(Initial);
	UDialogueSampleEffectProvider* RestoredGame = NewObject<UDialogueSampleEffectProvider>();
	FObjectReader RestoreGame(RestoredGame, CapturedGame);
	int64 Count = -1;
	RestoredGame->QueryDialogueCount_Implementation(NAME_None, Item.Key.ItemId, Count);
	TestEqual(TEXT("Captured game state stays at effect boundary"), Count, int64(0));
	TestTrue(TEXT("Captured ledger prevents replay"), UDialogueEffectLibrary::ApplyEffectBatch(Batch, RestoredGame, nullptr).Status == EDialogueEffectStatus::AlreadyApplied);
	TestFalse(TEXT("Post-effect condition sees captured state"), UDialogueConditionLibrary::EvaluateConditions(Conditions, RestoredGame, false, Context, nullptr).bPassed);
	const FDialogueCheckpointSnapshot& Snapshot = Save->CapturedRequests[0].Snapshot;
	TestTrue(TEXT("Same block identity across contracts"), Snapshot.Resume.Block == Graph->GetBlockReference());
	TestTrue(TEXT("Resume after selected node"), Snapshot.Resume.NodeGuid == NextLine->NodeGuid && Snapshot.Trigger.OriginNodeGuid == ChoiceNode->NodeGuid);
	TestEqual(TEXT("Captured history isolated"), Snapshot.FullReadHistory.Num(), 1);
	TestTrue(TEXT("Choice identity kept"), Snapshot.Trigger.ChoiceId == Info->GetDialogueChoices()[0].ChoiceId);
	TestTrue(TEXT("Execution identity kept"), Snapshot.Trigger.ExecutionId == Batch.Context.ExecutionId);
	return true;
}
#endif
