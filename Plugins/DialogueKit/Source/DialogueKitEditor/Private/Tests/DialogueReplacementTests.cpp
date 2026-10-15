#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "DialogueReplacement.h"
#include "DialogueRestoreController.h"
#include "DialogueCaptureFailureProbe.h"
#include "DialogueGraphCodec.h"
#include "DialogueEdGraphSchema.h"
#include "DialogueFlowEdGraphNode.h"
#include "DialogueChoiceEdGraphNode.h"
#include "DialogueEdEndGraphNode.h"
#include "DialogueChoiceNodeInfo.h"
#include "Kismet/GameplayStatics.h"

// 실제 저작/직렬화 경로를 이용하는 독립 fixture다.
static UDialogueEdGraphNodeBase* AddReplacementNode(UEdGraph* Graph, UClass* Type, UEdGraphPin* From)
{
	FNewNodeAction Action(Type, FText(), FText(), FText(), 0);
	return CastChecked<UDialogueEdGraphNodeBase>(Action.PerformAction(Graph, From, FVector2D::ZeroVector));
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDialogueReplacementTest, "DialogueKit.Restore.SelectiveReplacement", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// 다른 경로 지급 보존, 보정 원장 멱등성, 접수 실패 후 정확히 한 번 교체를 검증한다.
bool FDialogueReplacementTest::RunTest(const FString& Parameters)
{
	auto* Block = NewObject<UDialogueGraph>();
	Block->InitializeBlockIdentity();
	auto* Graph = NewObject<UEdGraph>(Block);
	Graph->Schema = UDialogueEdGraphSchema::StaticClass();
	Graph->GetSchema()->CreateDefaultNodesForGraph(*Graph);
	auto* Line = AddReplacementNode(Graph, UDialogueFlowEdGraphNode::StaticClass(), Graph->Nodes[0]->Pins[0]);
	auto* Choice = AddReplacementNode(Graph, UDialogueChoiceEdGraphNode::StaticClass(), Line->Pins[1]);
	FDialogueOption Option;
	Option.ChoiceId = FGuid::NewGuid();
	Option.Text = FText::FromString(TEXT("Reward"));
	FDialogueEffect Reward;
	Reward.Key.Kind = EDialogueStateKind::Count;
	Reward.Key.TargetId = TEXT("Player");
	Reward.Key.ItemId = TEXT("Coin");
	Reward.Count = 10;
	Option.Effects = {Reward};
	CastChecked<UDialogueChoiceNodeInfo>(Choice->GetNodeInfo())->Options = {Option};
	Choice->OnPropertiesChanged();
	auto* Next = AddReplacementNode(Graph, UDialogueFlowEdGraphNode::StaticClass(), Choice->Pins[1]);
	AddReplacementNode(Graph, UDialogueEdEndGraphNode::StaticClass(), Next->Pins[1]);
	FString ExportError;
	if (TestTrue(TEXT("Source export"), FDialogueGraphCodec::ExportGraph(Graph, Block, ExportError)) == false) return false;
	auto* Target = NewObject<UDialogueGraph>();
	Target->InitializeBlockIdentity();
	auto* TargetGraph = NewObject<UEdGraph>(Target);
	TargetGraph->Schema = UDialogueEdGraphSchema::StaticClass();
	TargetGraph->GetSchema()->CreateDefaultNodesForGraph(*TargetGraph);
	auto* First = AddReplacementNode(TargetGraph, UDialogueFlowEdGraphNode::StaticClass(), TargetGraph->Nodes[0]->Pins[0]);
	AddReplacementNode(TargetGraph, UDialogueEdEndGraphNode::StaticClass(), First->Pins[1]);
	if (TestTrue(TEXT("Target export"), FDialogueGraphCodec::ExportGraph(TargetGraph, Target, ExportError)) == false) return false;
	auto* Game = NewObject<UDialogueSampleEffectProvider>();
	FDialogueStateDefinition Definition;
	Definition.Key = Reward.Key;
	Game->AddDefinition(Definition);
	auto* Save = NewObject<UDialogueSampleSaveCoordinator>();
	FName Error;
	Save->Initialize(TEXT("ReplacementSource_") + FGuid::NewGuid().ToString(EGuidFormats::Digits), Game, Error);
	FDialogueQueuedSave Original;
	Save->GetSaveQueue()->OnWriteRequestedNative.BindLambda([&](const FDialogueQueuedSave& Item) { Original = Item; });
	auto* SourceSession = NewObject<UDialogueCheckpointSession>();
	SourceSession->Initialize(Save, Game, Game, nullptr);
	SourceSession->StartBlock(Block, FGameplayTag::RequestGameplayTag(TEXT("DialogueKit.Test.HistoryTarget")));
	TestTrue(TEXT("Original choice"), SourceSession->GetRunner()->SelectChoice(Option.ChoiceId));
	const FDialogueCheckpointSnapshot Source = Original.Request.Snapshot;
	FDialogueEffectBatch OtherPath;
	OtherPath.Context.ExecutionId = FGuid::NewGuid();
	Reward.Count = 5;
	OtherPath.Effects = {Reward};
	TestTrue(TEXT("Other route reward"), Game->ApplyDialogueEffects_Implementation(OtherPath).IsSuccess());
	FDialogueReplacementRule Rule;
	Rule.RuleId = FGuid::NewGuid();
	Rule.SourceBlockId = Block->GetBlockId();
	Rule.Target = Target;
	Rule.TargetReference = Target->GetBlockReference();
	Rule.Mode = EDialogueCorrectionMode::SelectiveRollback;
	Rule.GameRule = TEXT("RemoveOriginalReward");
	Rule.RequiredPassedLine = Line->NodeGuid;
	Rule.EffectNodeIds = {Choice->NodeGuid};
	FDialogueCorrectionRequest Correction;
	bool bNeeds = false;
	TestTrue(TEXT("Build correction"), FDialogueReplacement::BuildCorrection(Source, Rule, Block, Correction, bNeeds, Error));
	TestTrue(TEXT("Passed source line requires correction"), bNeeds);
	TestEqual(TEXT("Only source execution selected"), Correction.Effects.Num(), 1);
	FDialogueCheckpointSnapshot Incomplete = Source;
	Incomplete.RunEvidence.bComplete = false;
	TestFalse(TEXT("Unknown legacy effects cannot rollback"), FDialogueReplacement::BuildCorrection(Incomplete, Rule, Block, Correction, bNeeds, Error));
	FDialogueReplacementRule Unknown = Rule;
	Unknown.RequiredPassedLine = FGuid::NewGuid();
	TestFalse(TEXT("Unknown line does not silently skip correction"), FDialogueReplacement::BuildCorrection(Source, Unknown, Block, Correction, bNeeds, Error));
	FDialogueCheckpointSnapshot NotPassed = Source;
	Rule.RequiredPassedLine = Next->NodeGuid;
	TestTrue(TEXT("Known line not passed"), FDialogueReplacement::BuildCorrection(NotPassed, Rule, Block, Correction, bNeeds, Error));
	TestFalse(TEXT("No correction before threshold"), bNeeds);
	Rule.RequiredPassedLine = Line->NodeGuid;
	FDialogueReplacement::BuildCorrection(Source, Rule, Block, Correction, bNeeds, Error);
	auto* ReplacementSave = NewObject<UDialogueCaptureFailureProbe>();
	ReplacementSave->Initialize(TEXT("ReplacementTarget_") + FGuid::NewGuid().ToString(EGuidFormats::Digits), Game, Error);
	TArray<FDialogueQueuedSave> Writes;
	ReplacementSave->GetSaveQueue()->OnWriteRequestedNative.BindLambda([&](const FDialogueQueuedSave& Item) { Writes.Add(Item); });
	auto* Session = NewObject<UDialogueCheckpointSession>();
	Session->Initialize(ReplacementSave, Game, Game, nullptr);
	Session->AttachSaveQueue(ReplacementSave->GetSaveQueue());
	TestFalse(TEXT("Capture fails after atomic correction"), Session->ReplaceLoadedBlock(Target, Source, Rule, Block, Game));
	TestTrue(TEXT("Pending replacement retained"), Session->HasPendingCapture());
	int64 Count = 0;
	Game->QueryDialogueCount_Implementation(TEXT("Player"), TEXT("Coin"), Count);
	TestEqual(TEXT("Other route preserved"), Count, int64(5));
	TestTrue(TEXT("Original choice retained until durable replacement"), Session->GetCheckpointState()->CaptureGameSave().PendingChoices.IsEmpty() == false);
	TestTrue(TEXT("Correction same payload idempotent"), Game->CorrectDialogueState_Implementation(Correction).Status == EDialogueEffectStatus::AlreadyApplied);
	FDialogueCorrectionRequest Conflict = Correction;
	Conflict.RuleFingerprint += TEXT("changed");
	TestFalse(TEXT("Same ID different correction refused"), Game->CorrectDialogueState_Implementation(Conflict).IsSuccess());
	Conflict = Correction;
	Conflict.CorrectionId = FGuid::NewGuid();
	TestFalse(TEXT("Different correction cannot remove same grant twice"), Game->CorrectDialogueState_Implementation(Conflict).IsSuccess());
	TestTrue(TEXT("Retry frozen replacement"), Session->RetryCapture());
	if (TestEqual(TEXT("One replacement snapshot"), Writes.Num(), 1) == false) return false;
	TestTrue(TEXT("Durable start before first target line"), Writes[0].Request.Snapshot.Resume.Kind == EDialogueResumeKind::BlockStart);
	TestEqual(TEXT("Replacement starts at first line"), Session->GetRunner()->GetPresentation().NodeGuid, First->NodeGuid);
	TestEqual(TEXT("Original read remains"), Writes[0].Request.Snapshot.FullReadHistory.Num(), Source.FullReadHistory.Num());
	auto* Combined = CastChecked<UDialogueSampleCombinedSave>(UGameplayStatics::LoadGameFromMemory(Writes[0].GamePayload));
	// 커스텀 쓰기 수신기는 실제 writer의 Revision 부여를 대신한다.
	Combined->Revision = 1;
	TestTrue(TEXT("Correction receipt serialized atomically"), Combined->Validate());
	auto* RestoredGame = NewObject<UDialogueSampleEffectProvider>();
	TestTrue(TEXT("Restore corrected ledger"), RestoredGame->RestoreEffectSnapshot(Combined->Game, Error));
	TestTrue(TEXT("Persisted correction still idempotent"), RestoredGame->CorrectDialogueState_Implementation(Correction).Status == EDialogueEffectStatus::AlreadyApplied);
	Combined->Game.Corrections.Reset();
	TestFalse(TEXT("Missing game correction receipt rejected"), Combined->Validate());
	FDialogueCheckpointCompletion Done;
	Done.RequestId = Writes[0].Request.RequestId;
	Done.QueueGenerationId = Writes[0].Request.QueueGenerationId;
	Done.Sequence = Writes[0].Request.Sequence;
	Done.bSaved = true;
	ReplacementSave->GetSaveQueue()->CompleteWrite(Done);
	TestTrue(TEXT("Durable replacement clears old temporary choices"), Session->GetCheckpointState()->CaptureGameSave().PendingChoices.IsEmpty());
	auto* Controller = NewObject<UDialogueRestoreController>();
	Controller->Initialize(Session, Game, nullptr);
	const FDialogueResumePosition BeforeReset = Session->GetCheckpointState()->GetResumeAnchor();
	ReplacementSave->bReady = false;
	TestFalse(TEXT("Reset refuses unready save"), Controller->ResetStoryAndSave(true));
	TestEqual(TEXT("Failed reset preserves active block"), Session->GetCheckpointState()->GetResumeAnchor().Block.BlockId, BeforeReset.Block.BlockId);
	TestTrue(TEXT("Failed new game preserves reads"), Session->GetCheckpointState()->HasReadLine(Block->GetBlockId(), Line->NodeGuid, 1));
	ReplacementSave->bReady = true;
	Session->GetRunner()->CancelBlock();
	FDialogueCheckpointSnapshot Missing = Source;
	Missing.Resume.Block.BlockId = FGuid::NewGuid();
	Missing.Trigger = FDialogueCheckpointTrigger();
	bool bFailedEvent = false;
	Controller->OnRestoredNative.AddLambda([&](bool bSuccess, FName Code) { bFailedEvent = bSuccess == false && Code == TEXT("RestoreBlockNotFound"); });
	TestFalse(TEXT("Immediate missing exact reference returns failure"), Controller->Restore(Missing, nullptr));
	TestTrue(TEXT("Immediate failure event agrees"), bFailedEvent);
	ReplacementSave->Shutdown();
	Done.RequestId = Original.Request.RequestId;
	Done.QueueGenerationId = Original.Request.QueueGenerationId;
	Done.Sequence = Original.Request.Sequence;
	Save->GetSaveQueue()->CompleteWrite(Done);
	Save->Shutdown();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDialogueReplacementRulesTest, "DialogueKit.Restore.VersionRangeRules", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// 범위 포함/제외, 중첩과 순환 거부는 게임 상태를 로드하기 전에 결정한다.
bool FDialogueReplacementRulesTest::RunTest(const FString& Parameters)
{
	auto* Map = NewObject<UDialogueReplacementMap>();
	FDialogueReplacementRule Rule;
	Rule.RuleId = FGuid::NewGuid();
	Rule.SourceBlockId = FGuid::NewGuid();
	Rule.FirstVersion = 2;
	Rule.LastVersion = 4;
	Rule.Target = TSoftObjectPtr<UDialogueGraph>(FSoftObjectPath(TEXT("/Game/Replacement.Target")));
	Rule.TargetReference.BlockId = FGuid::NewGuid();
	Rule.TargetReference.ContentVersion = 1;
	Map->Rules = {Rule};
	FName Error;
	FDialogueReplacementRule Found;
	bool bFound = false;
	FDialogueBlockReference Source;
	Source.BlockId = Rule.SourceBlockId;
	Source.ContentVersion = 2;
	TestTrue(TEXT("Valid rule"), Map->Resolve(Source, Found, bFound, Error));
	TestTrue(TEXT("First inclusive"), bFound);
	Source.ContentVersion = 4;
	Map->Resolve(Source, Found, bFound, Error);
	TestTrue(TEXT("Last inclusive"), bFound);
	Source.ContentVersion = 5;
	Map->Resolve(Source, Found, bFound, Error);
	TestFalse(TEXT("Outside remains exact original"), bFound);
	Map->Rules.Add(Rule);
	TestFalse(TEXT("Overlap refused"), Map->Validate(Error));
	Map->Rules.Pop();
	FDialogueReplacementRule Reverse;
	Reverse.RuleId = FGuid::NewGuid();
	Reverse.SourceBlockId = Rule.TargetReference.BlockId;
	Reverse.Target = TSoftObjectPtr<UDialogueGraph>(FSoftObjectPath(TEXT("/Game/Original.Source")));
	Reverse.TargetReference.BlockId = Rule.SourceBlockId;
	Reverse.TargetReference.ContentVersion = 2;
	Map->Rules.Add(Reverse);
	TestFalse(TEXT("Cycle refused"), Map->Validate(Error));
	return true;
}
#endif
