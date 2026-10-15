#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "DialogueSampleHost.h"
#include "DialogueSampleCaptureProbe.h"
#include "DialogueStateChangeNodeInfo.h"
#include "DialogueSubsystem.h"
#include "DialogueLineNodeInfo.h"
#include "DialogueChoiceNodeInfo.h"
#include "DialogueEndNodeInfo.h"
#include "DialogueSampleSaveCoordinator.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "NativeGameplayTags.h"

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_HostTarget, "DialogueKit.Test.HostTarget");

// 사용자 에셋을 수정하지 않고 실행 가능한 메모리 Graph를 구성한다.
static UDialogueRuntimeNode* AddHostNode(UDialogueGraph* Block, EDialogueType Type)
{
	UDialogueRuntimeNode* Node = NewObject<UDialogueRuntimeNode>(Block->Graph);
	Node->NodeGuid = FGuid::NewGuid();
	Node->DialogueNodeType = Type;
	if (Type != EDialogueType::StartNode)
	{
		Node->InputPin = NewObject<UDialogueRuntimePin>(Node);
		Node->InputPin->PinId = FGuid::NewGuid();
		Node->InputPin->OwnerNodeGuid = Node->NodeGuid;
	}
	if (Type == EDialogueType::LineNode) Node->NodeInfo = NewObject<UDialogueLineNodeInfo>(Node);
	if (Type == EDialogueType::ChoiceNode) Node->NodeInfo = NewObject<UDialogueChoiceNodeInfo>(Node);
	if (Type == EDialogueType::EndNode) Node->NodeInfo = NewObject<UDialogueEndNodeInfo>(Node);
	Block->Graph->Nodes.Add(Node);
	return Node;
}

// 실제 실행기에서 사용하는 핀 연결과 안정 ID를 구성한다.
static void LinkHostNodes(UDialogueRuntimeNode* From, UDialogueRuntimeNode* To, FGuid ChoiceId = FGuid())
{
	UDialogueRuntimePin* Pin = NewObject<UDialogueRuntimePin>(From);
	Pin->PinId = FGuid::NewGuid();
	Pin->OwnerNodeGuid = From->NodeGuid;
	Pin->LinkedToNodeGuid = To->NodeGuid;
	Pin->ChoiceId = ChoiceId;
	Pin->Connections.Add(To->InputPin);
	To->InputPin->Connections.Add(Pin);
	From->OutputPins.Add(Pin);
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDialogueSampleHostFlowTest, "DialogueKit.Integration.SampleHostFlow",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// 후보→Subsystem→선택 효과→합본→같은 세션 복원과 지연 후보 취소를 공개 경계에서 검사한다.
bool FDialogueSampleHostFlowTest::RunTest(const FString& Parameters)
{
	UDialogueGraph* Block = NewObject<UDialogueGraph>();
	Block->InitializeBlockIdentity();
	Block->DialogueTarget = TAG_HostTarget;
	Block->StoryScope.Mode = EDialogueStoryScope::Unrestricted;
	Block->Graph = NewObject<UDialogueRuntimeGraph>(Block);
	auto* Start = AddHostNode(Block, EDialogueType::StartNode);
	auto* Line = AddHostNode(Block, EDialogueType::LineNode);
	auto* Choice = AddHostNode(Block, EDialogueType::ChoiceNode);
	auto* Next = AddHostNode(Block, EDialogueType::LineNode);
	auto* End = AddHostNode(Block, EDialogueType::EndNode);
	FDialogueOption Option;
	Option.ChoiceId = FGuid::NewGuid();
	Option.Text = FText::FromString(TEXT("Accept"));
	FDialogueEffect Effect;
	Effect.Key.Kind = EDialogueStateKind::Count;
	Effect.Key.ItemId = TEXT("Coin");
	Effect.Count = 10;
	Option.Effects.Add(Effect);
	CastChecked<UDialogueChoiceNodeInfo>(Choice->NodeInfo)->Options = {Option};
	LinkHostNodes(Start, Line);
	LinkHostNodes(Line, Choice);
	LinkHostNodes(Choice, Next, Option.ChoiceId);
	LinkHostNodes(Next, End);
	auto* Game = NewObject<UDialogueSampleEffectProvider>();
	FDialogueStateDefinition Definition;
	Definition.Key = Effect.Key;
	Game->AddDefinition(Definition);
	auto* Save = NewObject<UDialogueSampleSaveCoordinator>();
	FName Error;
	TestTrue(TEXT("Save coordinator initialized"), Save->Initialize(TEXT("Host_") + FGuid::NewGuid().ToString(EGuidFormats::Digits), Game, Error));
	TArray<FDialogueQueuedSave> Writes;
	Save->GetSaveQueue()->OnWriteRequestedNative.BindLambda([&](const FDialogueQueuedSave& Queued) { Writes.Add(Queued); });
	auto* Dialogue = NewObject<UDialogueSubsystem>(NewObject<UGameInstance>());
	auto* Host = NewObject<UDialogueSampleHost>();
	if (!TestTrue(TEXT("Host initialized"), Host->Initialize(Dialogue, Game, Game, Save, Save->GetSaveQueue(), Game, nullptr))) return false;
	Host->GetLoader()->LoadOverride.BindLambda([&](FGuid Id, const FSoftObjectPath&) { return Host->GetLoader()->CompleteRequest(Id, Block); });
	FDialogueBlockCandidate Candidate;
	Candidate.AssetPath = FSoftObjectPath(Block);
	Candidate.Metadata = Block->GetSelectionMetadata();
	const FGuid Request = Host->RequestCandidates({Candidate}, TAG_HostTarget, nullptr, FDialogueStoryProgress());
	TestTrue(TEXT("Synchronous request starts"), Request.IsValid() && Dialogue->GetBlockPresentation().Choices.Num() == 1);
	TestFalse(TEXT("Active dialogue rejects another start"), Host->RequestCandidates({Candidate}, TAG_HostTarget, nullptr, FDialogueStoryProgress()).IsValid());
	TestTrue(TEXT("Choice through Subsystem"), Dialogue->SelectBlockChoice(Option.ChoiceId));
	if (!TestEqual(TEXT("One atomic capture"), Writes.Num(), 1)) return false;
	TestEqual(TEXT("Game continues while write pending"), Dialogue->GetBlockPresentation().LineNodeGuid, Next->NodeGuid);
	const FDialogueCheckpointSnapshot Snapshot = Writes[0].Request.Snapshot;
	TestTrue(TEXT("Same checkpoint has passed Line and effect"), Snapshot.RunEvidence.PassedLines.Contains(Line->NodeGuid) && Snapshot.RunEvidence.Effects.Num() == 1);
	TestTrue(TEXT("Normal End"), Dialogue->GetBlockRunner()->AdvanceLine());
	TestFalse(TEXT("Completion consumed once"), Host->GetLoader()->RecordBlockCompleted(Request, Block->GetBlockReference()));
	FDialogueCheckpointCompletion Completion;
	Completion.RequestId = Writes[0].Request.RequestId;
	Completion.QueueGenerationId = Writes[0].Request.QueueGenerationId;
	Completion.Sequence = Writes[0].Request.Sequence;
	Completion.bSaved = true;
	Save->GetSaveQueue()->CompleteWrite(Completion);
	TestTrue(TEXT("End session"), Host->EndConversation());
	// 저장 파일 검증은 별도 재시작 테스트가 담당한다. 여기서는 같은 합본의 메모리 복원을 검사한다.
	auto* Combined = CastChecked<UDialogueSampleCombinedSave>(UGameplayStatics::LoadGameFromMemory(Writes[0].GamePayload));
	TestTrue(TEXT("Game restored"), Game->RestoreEffectSnapshot(Combined->Game, Error));
	TestTrue(TEXT("Subsystem exact resume"), Dialogue->ResumeDialogueBlock(Block, Snapshot));
	TestEqual(TEXT("Resume position"), Dialogue->GetBlockPresentation().LineNodeGuid, Next->NodeGuid);
	int64 Count = 0;
	Game->QueryDialogueCount_Implementation(NAME_None, TEXT("Coin"), Count);
	TestEqual(TEXT("No duplicate reward"), Count, int64(10));
	TestEqual(TEXT("Resume adds no write"), Writes.Num(), 1);
	Host->EndConversation();
	TestFalse(TEXT("Unfinished saved dialogue requires resume or explicit reset"), Host->RequestCandidates({Candidate}, TAG_HostTarget, nullptr, FDialogueStoryProgress()).IsValid());
	TestTrue(TEXT("Explicit story reset before new session"), Dialogue->GetCheckpointSession()->ResetStory(false));
	// 공통 인사 에셋은 target이 없어도 요청 대상이 저장돼야 한다.
	Block->DialogueTarget = FGameplayTag();
	Host->GetLoader()->CommonGreetingBlock = Block;
	Host->RequestCandidates({}, TAG_HostTarget, nullptr, FDialogueStoryProgress());
	TestTrue(TEXT("Greeting starts without asset target"), Dialogue->GetBlockPresentation().Choices.Num() == 1);
	TestTrue(TEXT("Greeting saves request target"), Dialogue->GetCheckpointSession()->GetCheckpointState()->GetResumeAnchor().TargetTag == TAG_HostTarget);
	Host->EndConversation();
	Host->GetLoader()->LoadOverride.BindLambda([](FGuid, const FSoftObjectPath&) { return true; });
	Dialogue->GetCheckpointSession()->ResetStory(false);
	const FGuid Pending = Host->RequestCandidates({Candidate}, TAG_HostTarget, nullptr, FDialogueStoryProgress());
	FDialogueCheckpointSnapshot Inactive;
	// 정상 초기화 snapshot을 사용해 복원 시작의 후보 취소를 검증한다.
	Dialogue->GetCheckpointSession()->ResetStory(false);
	Inactive = Dialogue->GetCheckpointSession()->GetCheckpointState()->CaptureGameSave();
	TestTrue(TEXT("Inactive restore"), Host->RestoreDialogue(Inactive, nullptr));
	TestFalse(TEXT("Restore cancels late candidate"), Host->GetLoader()->CompleteRequest(Pending, Block));
	TestTrue(TEXT("Coordinator shuts down"), Save->Shutdown());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDialogueSampleHostRetryTest, "DialogueKit.Integration.SampleHostInitialCaptureRetry",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// 최초 자동 효과의 접수 실패와 Start→End 동기 완료에서 요청/소진 기록을 유지한다.
bool FDialogueSampleHostRetryTest::RunTest(const FString& Parameters)
{
	for (bool bWaitAtLine : {false, true})
	{
		auto* Block = NewObject<UDialogueGraph>();
		Block->InitializeBlockIdentity();
		Block->DialogueTarget = TAG_HostTarget;
		Block->StoryScope.Mode = EDialogueStoryScope::Unrestricted;
		Block->Graph = NewObject<UDialogueRuntimeGraph>(Block);
		auto* Start = AddHostNode(Block, EDialogueType::StartNode);
		auto* Change = AddHostNode(Block, EDialogueType::StateChangeNode);
		auto* ChangeInfo = NewObject<UDialogueStateChangeNodeInfo>(Change);
		Change->NodeInfo = ChangeInfo;
		FDialogueEffect Effect;
		Effect.Key.Kind = EDialogueStateKind::Count;
		Effect.Key.ItemId = TEXT("Coin");
		Effect.Count = 1;
		ChangeInfo->Effects.Add(Effect);
		auto* End = AddHostNode(Block, EDialogueType::EndNode);
		LinkHostNodes(Start, Change);
		if (bWaitAtLine)
		{
			auto* Line = AddHostNode(Block, EDialogueType::LineNode);
			LinkHostNodes(Change, Line);
			LinkHostNodes(Line, End);
		}
		else
		{
			LinkHostNodes(Change, End);
		}
		auto* Game = NewObject<UDialogueSampleEffectProvider>();
		FDialogueStateDefinition Definition;
		Definition.Key = Effect.Key;
		Game->AddDefinition(Definition);
		auto* Probe = NewObject<UDialogueSampleCaptureProbe>();
		auto* Dialogue = NewObject<UDialogueSubsystem>(NewObject<UGameInstance>());
		auto* Host = NewObject<UDialogueSampleHost>();
		TestTrue(TEXT("Retry host initializes"), Host->Initialize(Dialogue, Game, Game, Probe, nullptr, Game, nullptr));
		Host->GetLoader()->LoadOverride.BindLambda([&](FGuid Id, const FSoftObjectPath&) { return Host->GetLoader()->CompleteRequest(Id, Block); });
		FDialogueBlockCandidate Candidate;
		Candidate.AssetPath = FSoftObjectPath(Block);
		Candidate.Metadata = Block->GetSelectionMetadata();
		const FGuid Request = Host->RequestCandidates({Candidate}, TAG_HostTarget, nullptr, FDialogueStoryProgress());
		TestTrue(TEXT("Initial capture retained"), Dialogue->GetCheckpointSession()->HasPendingCapture());
		TestTrue(TEXT("Host capture retry"), Host->RetryCapture());
		int64 Count = 0;
		Game->QueryDialogueCount_Implementation(NAME_None, TEXT("Coin"), Count);
		TestEqual(TEXT("Retry never repeats effect"), Count, int64(1));
		if (bWaitAtLine) TestTrue(TEXT("Delayed End"), Dialogue->GetBlockRunner()->AdvanceLine());
		TestTrue(TEXT("Completed presentation"), Dialogue->GetBlockPresentation().State == EDialogueRunState::Completed);
		TestFalse(TEXT("Host retained request through retry and consumed once"), Host->GetLoader()->RecordBlockCompleted(Request, Block->GetBlockReference()));
		Host->EndConversation();
	}
	return true;
}
#endif
