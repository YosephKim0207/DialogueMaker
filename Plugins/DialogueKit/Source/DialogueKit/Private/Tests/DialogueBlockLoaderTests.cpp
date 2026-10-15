#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "DialogueBlockLoader.h"
#include "DialogueSampleStateProvider.h"
#include "NativeGameplayTags.h"

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_LoadTarget, "DialogueKit.Test.LoadTarget");

// 기존 콘텐츠를 수정하지 않고 메모리 Block과 메타데이터를 준비한다.
static FDialogueBlockCandidate MakeLoadCandidate(UDialogueGraph*& OutBlock)
{
	OutBlock = NewObject<UDialogueGraph>();
	OutBlock->InitializeBlockIdentity();
	OutBlock->DialogueTarget = TAG_LoadTarget;
	OutBlock->StoryScope.Mode = EDialogueStoryScope::Unrestricted;
	FDialogueBlockCandidate Candidate;
	Candidate.AssetPath = FSoftObjectPath(OutBlock);
	Candidate.Metadata = OutBlock->GetSelectionMetadata();
	return Candidate;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDialogueBlockLoaderTest, "DialogueKit.Loading.LatestRequest", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// 역순 완료, 취소, 실패, 즉시 완료와 조건 고정을 확인한다.
bool FDialogueBlockLoaderTest::RunTest(const FString& Parameters)
{
	UDialogueBlockLoader* Loader = NewObject<UDialogueBlockLoader>();
	Loader->LoadOverride.BindLambda([](FGuid, const FSoftObjectPath&) { return true; });
	UDialogueGraph* A;
	UDialogueGraph* B;
	const FDialogueBlockCandidate CandidateA = MakeLoadCandidate(A);
	FDialogueBlockCandidate CandidateB = MakeLoadCandidate(B);
	int32 Published = 0;
	Loader->OnLoadCompletedNative.AddLambda([&](const FDialogueBlockLoadResult&) { ++Published; });
	auto Request = [&](const FDialogueBlockCandidate& Candidate)
	{
		return Loader->RequestCandidates({Candidate}, TAG_LoadTarget, nullptr, FDialogueStoryProgress(), nullptr, nullptr);
	};
	const FGuid First = Request(CandidateA);
	const FGuid Second = Request(CandidateB);
	TestFalse(TEXT("Superseded completion ignored"), Loader->CompleteRequest(First, A));
	TestFalse(TEXT("Older completion record ignored"), Loader->RecordBlockCompleted(First, B->GetBlockReference()));
	TestTrue(TEXT("Latest completes"), Loader->CompleteRequest(Second, B));
	TestEqual(TEXT("Only latest published"), Published, 1);
	TestFalse(TEXT("Duplicate completion ignored"), Loader->CompleteRequest(Second, B));
	TestTrue(TEXT("Explicit exposure"), Loader->RecordBlockExposed(Second, B->GetBlockReference()));
	TestTrue(TEXT("Explicit normal completion"), Loader->RecordBlockCompleted(Second, B->GetBlockReference()));
	Loader->EndSession();
	const FGuid SameOld = Request(CandidateA);
	const FGuid SameNew = Request(CandidateA);
	TestTrue(TEXT("Latest identical block completes"), Loader->CompleteRequest(SameNew, A));
	TestFalse(TEXT("Old identical block exposure rejected"), Loader->RecordBlockExposed(SameOld, A->GetBlockReference()));
	TestFalse(TEXT("Old identical block End rejected"), Loader->RecordBlockCompleted(SameOld, A->GetBlockReference()));
	Loader->CancelPending();
	TestTrue(TEXT("Cancel pending preserves completed request"), Loader->IsCurrentRequest(SameNew));
	TestTrue(TEXT("Correct identical block End accepted"), Loader->RecordBlockCompleted(SameNew, A->GetBlockReference()));
	Loader->EndSession();
	const FGuid Canceled = Request(CandidateA);
	Loader->CancelPending();
	TestFalse(TEXT("Canceled callback ignored"), Loader->CompleteRequest(Canceled, A));
	const FGuid OldSuccess = Request(CandidateA);
	const FGuid LatestFailure = Request(CandidateB);
	TestFalse(TEXT("Wrong block fails latest"), Loader->CompleteRequest(LatestFailure, A));
	TestFalse(TEXT("Old success cannot replace failure"), Loader->CompleteRequest(OldSuccess, A));
	TestEqual(TEXT("Failure retained"), Loader->GetLastResult().ErrorCode, FName(TEXT("InvalidLoadedBlock")));
	Loader->EndSession();
	Loader->LoadOverride.BindLambda([&](FGuid Id, const FSoftObjectPath&) { return Loader->CompleteRequest(Id, A); });
	const FGuid Immediate = Request(CandidateA);
	TestTrue(TEXT("Immediate completion supported"), Loader->GetLastResult().bLoaded);
	TestEqual(TEXT("Immediate request identity"), Loader->GetLastResult().RequestId, Immediate);
	Loader->EndSession();
	Loader->LoadOverride.BindLambda([](FGuid, const FSoftObjectPath&) { return true; });
	UDialogueSampleStateProvider* Provider = NewObject<UDialogueSampleStateProvider>();
	FDialogueCondition Condition;
	Condition.Key.Kind = EDialogueStateKind::Count;
	Condition.Key.ItemId = TEXT("Key");
	Condition.CountOperand = 1;
	FDialogueStateDefinition Definition;
	Definition.Key = Condition.Key;
	Provider->AddDefinition(Definition);
	FDialogueSampleState State;
	State.Key = Condition.Key;
	State.Count = 1;
	Provider->SetSampleState(State);
	CandidateB.Metadata.EntryConditions.Conditions.Add(Condition);
	const FGuid Eligible = Loader->RequestCandidates({CandidateB}, TAG_LoadTarget, nullptr, FDialogueStoryProgress(), Provider, nullptr);
	State.Count = 0;
	Provider->SetSampleState(State);
	TestTrue(TEXT("Request eligibility not reevaluated at completion"), Loader->CompleteRequest(Eligible, B));
	Loader->EndSession();
	Loader->CommonGreetingBlock = A;
	const FGuid Greeting = Loader->RequestCandidates({}, TAG_LoadTarget, nullptr, FDialogueStoryProgress(), nullptr, nullptr);
	TestTrue(TEXT("Configured common greeting fallback"), Loader->CompleteRequest(Greeting, A));
	TestFalse(TEXT("Greeting does not consume normal candidate"), Loader->RecordBlockCompleted(Greeting, A->GetBlockReference()));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDialogueBlockLoaderReentryTest, "DialogueKit.Loading.ReentrantSelection", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// 조건 조회 오류 구독자가 새 요청을 보내도 이전 선택기가 최신 선택 기록을 덮지 않는다.
bool FDialogueBlockLoaderReentryTest::RunTest(const FString& Parameters)
{
	UDialogueBlockLoader* Loader = NewObject<UDialogueBlockLoader>();
	Loader->LoadOverride.BindLambda([](FGuid, const FSoftObjectPath&) { return true; });
	UDialogueGraph* A;
	UDialogueGraph* B;
	FDialogueBlockCandidate CandidateA = MakeLoadCandidate(A);
	const FDialogueBlockCandidate CandidateB = MakeLoadCandidate(B);
	FDialogueCondition Missing;
	Missing.Key.Kind = EDialogueStateKind::Count;
	Missing.Key.ItemId = TEXT("Missing");
	CandidateA.Metadata.EntryConditions.Conditions.Add(Missing);
	UDialogueDiagnosticRouter* Router = NewObject<UDialogueDiagnosticRouter>();
	FGuid Latest;
	bool bReentered = false;
	Router->OnDiagnosticNative.AddLambda([&](const FDialogueDiagnostic&)
	{
		if (bReentered == false)
		{
			bReentered = true;
			Latest = Loader->RequestCandidates({CandidateB}, TAG_LoadTarget, nullptr, FDialogueStoryProgress(), nullptr, nullptr);
		}
	});
	const FGuid Older = Loader->RequestCandidates({CandidateA}, TAG_LoadTarget, nullptr, FDialogueStoryProgress(), nullptr, Router);
	TestTrue(TEXT("Callback started newer request"), bReentered);
	TestFalse(TEXT("Older ignored"), Loader->CompleteRequest(Older, A));
	TestTrue(TEXT("New request completes"), Loader->CompleteRequest(Latest, B));
	TestTrue(TEXT("Selector retained newer selection"), Loader->RecordBlockCompleted(Latest, B->GetBlockReference()));
	return true;
}
#endif
