#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "DialogueCheckpointSession.h"
#include "DialogueSampleSaveCoordinator.h"
#include "DialogueCaptureFailureProbe.h"
#include "DialogueGraphCodec.h"
#include "DialogueEdGraphSchema.h"
#include "DialogueFlowEdGraphNode.h"
#include "DialogueChoiceEdGraphNode.h"
#include "DialogueStateChangeEdGraphNode.h"
#include "DialogueEdEndGraphNode.h"
#include "DialogueChoiceNodeInfo.h"
#include "DialogueStateChangeNodeInfo.h"
#include "NativeGameplayTags.h"
#include "Kismet/GameplayStatics.h"



// 실제 저작 액션으로 직렬 실행 fixture를 만든다.
static UDialogueEdGraphNodeBase* AddSessionNode(UEdGraph* Graph, UClass* Type, UEdGraphPin* From)
{
	FNewNodeAction Action(Type, FText(), FText(), FText(), 0);
	return CastChecked<UDialogueEdGraphNodeBase>(Action.PerformAction(Graph, From, FVector2D::ZeroVector));
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDialogueSessionTest, "DialogueKit.Checkpoints.SessionFlow", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// 동일 호출의 Line 통과/효과/재개점, 늦은 파일 저장과 복원 위치를 연결한다.
bool FDialogueSessionTest::RunTest(const FString& Parameters)
{
	UDialogueGraph* Block = NewObject<UDialogueGraph>();
	Block->InitializeBlockIdentity();
	Block->DialogueTarget = FGameplayTag::RequestGameplayTag(TEXT("DialogueKit.Test.HistoryTarget"));
	UEdGraph* Graph = NewObject<UEdGraph>(Block);
	Graph->Schema = UDialogueEdGraphSchema::StaticClass();
	Graph->GetSchema()->CreateDefaultNodesForGraph(*Graph);
	auto* Line = AddSessionNode(Graph, UDialogueFlowEdGraphNode::StaticClass(), Graph->Nodes[0]->Pins[0]);
	auto* Choice = AddSessionNode(Graph, UDialogueChoiceEdGraphNode::StaticClass(), Line->Pins[1]);
	auto* ChoiceInfo = CastChecked<UDialogueChoiceNodeInfo>(Choice->GetNodeInfo());
	FDialogueOption Option;
	Option.ChoiceId = FGuid::NewGuid();
	Option.Text = FText::FromString(TEXT("Accept"));
	FDialogueEffect Reward;
	Reward.Key.Kind = EDialogueStateKind::Count;
	Reward.Key.TargetId = TEXT("Player");
	Reward.Key.ItemId = TEXT("Coin");
	Reward.Count = 10;
	Option.Effects.Add(Reward);
	ChoiceInfo->Options = {Option};
	Choice->OnPropertiesChanged();
	auto* Next = AddSessionNode(Graph, UDialogueFlowEdGraphNode::StaticClass(), Choice->Pins[1]);
	AddSessionNode(Graph, UDialogueEdEndGraphNode::StaticClass(), Next->Pins[1]);
	FString ExportError;
	if (TestTrue(TEXT("Export"), FDialogueGraphCodec::ExportGraph(Graph, Block, ExportError)) == false) return false;
	auto* Game = NewObject<UDialogueSampleEffectProvider>();
	FDialogueStateDefinition Definition;
	Definition.Key = Reward.Key;
	Game->AddDefinition(Definition);
	auto* Coordinator = NewObject<UDialogueCaptureFailureProbe>();
	FName Error;
	Coordinator->Initialize(TEXT("Session_") + FGuid::NewGuid().ToString(EGuidFormats::Digits), Game, Error);
	TArray<FDialogueQueuedSave> Writes;
	Coordinator->GetSaveQueue()->OnWriteRequestedNative.BindLambda([&](const FDialogueQueuedSave& Save) { Writes.Add(Save); });
	auto* Session = NewObject<UDialogueCheckpointSession>();
	TestTrue(TEXT("Initialize"), Session->Initialize(Coordinator, Game, Game, nullptr));
	Session->AttachSaveQueue(Coordinator->GetSaveQueue());
	bool bReentryRejected = false;
	Session->GetRunner()->OnPresentationChangedNative.AddLambda([&](const FDialoguePresentation&)
	{
		bReentryRejected = Session->ResetStory(false) == false;
	});
	TestTrue(TEXT("Start"), Session->StartBlock(Block, FGameplayTag::RequestGameplayTag(TEXT("DialogueKit.Test.HistoryTarget"))));
	auto* WrongQueue = NewObject<UDialogueSaveQueue>();
	WrongQueue->InitializeQueue(FGuid::NewGuid());
	TestFalse(TEXT("Cannot replace active save queue"), Session->AttachSaveQueue(WrongQueue));
	TestTrue(TEXT("Callback reset rejected"), bReentryRejected);
	TestFalse(TEXT("Capture rejection stops inline choice"), Session->GetRunner()->SelectChoice(Option.ChoiceId));
	TestTrue(TEXT("Pending capture retained"), Session->HasPendingCapture());
	TestEqual(TEXT("No write before retry"), Writes.Num(), 0);
	FDialogueCheckpointSnapshot External;
	External.CheckpointId = FGuid::NewGuid();
	TestFalse(TEXT("Later direct enqueue cannot overtake"), Coordinator->GetSaveQueue()->Enqueue(Coordinator->GetSaveQueue()->MakeRequest(External), {1}).bAccepted);
	FDialogueEffectBatch Later;
	Later.Context.ExecutionId = FGuid::NewGuid();
	Later.Effects.Add(Reward);
	Game->ApplyDialogueEffects_Implementation(Later);
	TestFalse(TEXT("Cannot reset pending capture"), Session->ResetStory(false));
	TestTrue(TEXT("Retry immutable capture"), Session->RetryCapture());
	TestFalse(TEXT("Pending cleared after accept"), Session->HasPendingCapture());
	if (TestEqual(TEXT("One immutable write"), Writes.Num(), 1) == false) return false;
	const FDialogueCheckpointSnapshot Saved = Writes[0].Request.Snapshot;
	TestTrue(TEXT("Passed line before effect"), Saved.RunEvidence.PassedLines.Contains(Line->NodeGuid));
	TestEqual(TEXT("Related line"), Saved.RunEvidence.Effects[0].LineGuid, Line->NodeGuid);
	TestEqual(TEXT("Same run"), Saved.Resume.BlockRunId, Session->GetRunner()->GetBlockRunId());
	TestEqual(TEXT("Resume after choice"), Saved.Resume.NodeGuid, Next->NodeGuid);
	TestEqual(TEXT("Continue while disk pending"), Session->GetRunner()->GetPresentation().LineNodeGuid, Next->NodeGuid);
	TestTrue(TEXT("End no save"), Session->GetRunner()->AdvanceLine());
	TestEqual(TEXT("End cached only"), Writes.Num(), 1);
	auto* Combined = CastChecked<UDialogueSampleCombinedSave>(UGameplayStatics::LoadGameFromMemory(Writes[0].GamePayload));
	auto* RestoredGame = NewObject<UDialogueSampleEffectProvider>();
	RestoredGame->RestoreEffectSnapshot(Combined->Game, Error);
	auto* OtherCoordinator = NewObject<UDialogueSampleSaveCoordinator>();
	OtherCoordinator->Initialize(TEXT("SessionRestore_") + FGuid::NewGuid().ToString(EGuidFormats::Digits), RestoredGame, Error);
	auto* Restored = NewObject<UDialogueCheckpointSession>();
	Restored->Initialize(OtherCoordinator, RestoredGame, RestoredGame, nullptr);
	TestTrue(TEXT("Resume"), Restored->ResumeBlock(Block, Saved));
	TestEqual(TEXT("Restored run preserved"), Restored->GetRunner()->GetBlockRunId(), Saved.Resume.BlockRunId);
	TestEqual(TEXT("Saved next shown"), Restored->GetRunner()->GetPresentation().NodeGuid, Next->NodeGuid);
	int64 Count = 0;
	RestoredGame->QueryDialogueCount_Implementation(TEXT("Player"), TEXT("Coin"), Count);
	TestEqual(TEXT("No effect replay"), Count, int64(10));
	TestEqual(TEXT("No extra checkpoint"), OtherCoordinator->GetSaveQueue()->GetPendingCount(), 0);
	TestFalse(TEXT("Uncommitted later read not restored"), Restored->GetCheckpointState()->HasReadLine(Block->GetBlockId(), Next->NodeGuid, 1));
	TestTrue(TEXT("Death reset"), Restored->ResetStory(false));
	TestTrue(TEXT("Death keeps prior read"), Restored->GetCheckpointState()->HasReadLine(Block->GetBlockId(), Line->NodeGuid, 1));
	TestTrue(TEXT("New game reset"), Restored->ResetStory(true));
	TestFalse(TEXT("New game clears read"), Restored->GetCheckpointState()->HasReadLine(Block->GetBlockId(), Line->NodeGuid, 1));
	OtherCoordinator->Shutdown();
	FDialogueCheckpointCompletion Done;
	Done.RequestId = Writes[0].Request.RequestId;
	Done.QueueGenerationId = Writes[0].Request.QueueGenerationId;
	Done.Sequence = Writes[0].Request.Sequence;
	Done.bSaved = true;
	Coordinator->GetSaveQueue()->CompleteWrite(Done);
	Coordinator->Shutdown();
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDialogueSessionPrepareTest, "DialogueKit.Checkpoints.PrepareAndAutomaticStart", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// 첫 자동 효과보다 먼저 상태기를 준비하고 준비 실패는 게임 상태를 변경하지 않는다.
bool FDialogueSessionPrepareTest::RunTest(const FString& Parameters)
{
	UDialogueGraph* Block = NewObject<UDialogueGraph>();
	Block->InitializeBlockIdentity();
	UEdGraph* Graph = NewObject<UEdGraph>(Block);
	Graph->Schema = UDialogueEdGraphSchema::StaticClass();
	Graph->GetSchema()->CreateDefaultNodesForGraph(*Graph);
	auto* EffectNode = AddSessionNode(Graph, UDialogueStateChangeEdGraphNode::StaticClass(), Graph->Nodes[0]->Pins[0]);
	auto* Line = AddSessionNode(Graph, UDialogueFlowEdGraphNode::StaticClass(), EffectNode->Pins[1]);
	AddSessionNode(Graph, UDialogueEdEndGraphNode::StaticClass(), Line->Pins[1]);
	FDialogueEffect Reward;
	Reward.Key.Kind = EDialogueStateKind::Count;
	Reward.Key.TargetId = TEXT("Player");
	Reward.Key.ItemId = TEXT("Coin");
	Reward.Count = 2;
	CastChecked<UDialogueStateChangeNodeInfo>(EffectNode->GetNodeInfo())->Effects = {Reward};
	FString Error;
	if (TestTrue(TEXT("Export auto graph"), FDialogueGraphCodec::ExportGraph(Graph, Block, Error)) == false) return false;
	auto* Game = NewObject<UDialogueSampleEffectProvider>();
	FDialogueStateDefinition Definition;
	Definition.Key = Reward.Key;
	Game->AddDefinition(Definition);
	auto* Runner = NewObject<UDialogueBlockRunner>();
	int32 Events = 0;
	Runner->OnPresentationChangedNative.AddLambda([&](const FDialoguePresentation&) { ++Events; });
	TestTrue(TEXT("Prepare only"), Runner->PrepareBlock(Block, Game, Game, nullptr));
	TestEqual(TEXT("No presentation during prepare"), Events, 0);
	TestTrue(TEXT("No effect during prepare"), Game->CaptureEffectSnapshot().Requests.IsEmpty());
	Runner->CancelBlock();
	auto* Coordinator = NewObject<UDialogueSampleSaveCoordinator>();
	FName SaveError;
	Coordinator->Initialize(TEXT("AutoSession_") + FGuid::NewGuid().ToString(EGuidFormats::Digits), Game, SaveError);
	FDialogueQueuedSave Write;
	Coordinator->GetSaveQueue()->OnWriteRequestedNative.BindLambda([&](const FDialogueQueuedSave& Save) { Write = Save; });
	auto* Session = NewObject<UDialogueCheckpointSession>();
	Session->Initialize(Coordinator, Game, Game, nullptr, Runner);
	TestTrue(TEXT("Start with automatic effect"), Session->StartBlock(Block, FGameplayTag::RequestGameplayTag(TEXT("DialogueKit.Test.HistoryTarget"))));
	TestEqual(TEXT("Auto effect captured"), Write.Request.Snapshot.RunEvidence.Effects.Num(), 1);
	TestTrue(TEXT("No fabricated prior line"), Write.Request.Snapshot.RunEvidence.PassedLines.IsEmpty());
	TestEqual(TEXT("Next line shown"), Runner->GetPresentation().NodeGuid, Line->NodeGuid);
	FDialogueResumePosition Wrong = Write.Request.Snapshot.Resume;
	Wrong.Block.ContentVersion++;
	auto* Other = NewObject<UDialogueBlockRunner>();
	TestFalse(TEXT("Wrong version rejected"), Other->ResumeBlock(Block, Wrong, Game, Game, nullptr));
	Wrong = Write.Request.Snapshot.Resume;
	Wrong.NodeGuid = FGuid::NewGuid();
	TestFalse(TEXT("Missing node rejected"), Other->ResumeBlock(Block, Wrong, Game, Game, nullptr));
	FDialogueCheckpointCompletion Done;
	Done.RequestId = Write.Request.RequestId;
	Done.QueueGenerationId = Write.Request.QueueGenerationId;
	Done.Sequence = Write.Request.Sequence;
	Done.bSaved = true;
	Coordinator->GetSaveQueue()->CompleteWrite(Done);
	Coordinator->Shutdown();
	return true;
}
#endif
