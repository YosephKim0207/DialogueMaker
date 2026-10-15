#include "DialogueRestoreController.h"

// 복원 로더는 컨트롤러 전용이며 신규 후보 로더와 임의 공유하지 않는다.
bool UDialogueRestoreController::Initialize(UDialogueCheckpointSession* NewSession, UObject* NewCorrectionProvider, UDialogueDiagnosticRouter* NewRouter, UDialogueBlockLoader* NewCandidateLoader)
{
	if (Session || IsValid(NewSession) == false || NewSession->GetRunner() == nullptr) return false;
	Session = NewSession;
	CorrectionProvider = NewCorrectionProvider;
	Router = NewRouter;
	CandidateLoader = NewCandidateLoader;
	Loader = NewObject<UDialogueBlockLoader>(this);
	Loader->OnLoadCompletedNative.AddUObject(this, &UDialogueRestoreController::Loaded);
	return true;
}

// 최신 복원 요청만 남기며 완료한 과거 Block은 다시 실행하지 않는다.
bool UDialogueRestoreController::Restore(const FDialogueCheckpointSnapshot& Snapshot, UDialogueReplacementMap* Map)
{
	if (Session == nullptr || bHandling || Session->HasPendingCapture() || Session->GetRunner()->CanStartBlock() == false) return false;
	FDialogueCheckpointSnapshot Checked;
	FName Error;
	if (Snapshot.UpgradeToLatest(Checked, Error) == false)
	{
		Finish(false, Error);
		return false;
	}
	Loader->EndSession();
	Source = Checked;
	SourceGraph = nullptr;
	bReplacement = false;
	bWaiting = false;
	if (Source.Resume.Kind == EDialogueResumeKind::None || Source.Resume.Kind == EDialogueResumeKind::BlockCompleted)
	{
		const bool bRestored = Session->RestoreInactive(Source);
		Finish(bRestored, bRestored ? NAME_None : Session->GetLastError());
		return bRestored;
	}
	if (Map && Map->Resolve(Source.Resume.Block, Rule, bReplacement, Error) == false)
	{
		Finish(false, Error);
		return false;
	}
	bLastSuccess = false;
	bWaiting = true;
	bLoadingSource = bReplacement && Rule.Mode == EDialogueCorrectionMode::SelectiveRollback;
	if (bReplacement && bLoadingSource == false) Loader->RequestExactPath(Rule.Target, Rule.TargetReference, Router);
	else Loader->RequestExactReference(Source.Resume.Block, Router);
	return bWaiting || bLastSuccess;
}

// 선택적 롤백은 먼저 보존 원본의 노드 기준을 확보하고 그 다음 대체 리소스를 준비한다.
void UDialogueRestoreController::Loaded(const FDialogueBlockLoadResult& Result)
{
	if (bWaiting == false || Loader->IsCurrentRequest(Result.RequestId) == false) return;
	if (Result.bLoaded == false)
	{
		Finish(false, Result.ErrorCode);
		return;
	}
	if (bLoadingSource)
	{
		SourceGraph = Result.Block;
		bLoadingSource = false;
		Loader->RequestExactPath(Rule.Target, Rule.TargetReference, Router);
		return;
	}
	bool bSuccess;
	{
		TGuardValue<bool> Guard(bHandling, true);
		bSuccess = bReplacement ? Session->ReplaceLoadedBlock(Result.Block, Source, Rule, SourceGraph, CorrectionProvider) : Session->ResumeBlock(Result.Block, Source);
	}
	Finish(bSuccess, bSuccess ? NAME_None : (Session->GetLastError().IsNone() ? FName(TEXT("RestoreExecutionFailed")) : Session->GetLastError()));
}

// 성공/실패 안내는 게임이 결정한다. 개발자는 이 진단/이벤트에 외부 로그 전송을 연결한다.
void UDialogueRestoreController::Finish(bool bSuccess, FName ErrorCode)
{
	bWaiting = false;
	bLastSuccess = bSuccess;
	TGuardValue<bool> Guard(bHandling, true);
	if (bSuccess == false)
	{
		FDialogueDiagnostic Diagnostic;
		Diagnostic.Code = ErrorCode;
		Diagnostic.Context.Block = Source.Resume.Block;
		Diagnostic.Context.BlockRunId = Source.Resume.BlockRunId;
		Diagnostic.Message = TEXT("저장 대화 복원/교체에 실패했습니다.");
		Diagnostic.Handling = TEXT("KeepLastSuccessfulSaveAndReturnFailure");
		UDialogueDiagnosticRouter::Dispatch(Diagnostic, Router);
	}
	OnRestoredNative.Broadcast(bSuccess, ErrorCode);
	OnRestored.Broadcast(bSuccess, ErrorCode);
}

// 대기 중 로드만 취소한다. 이미 실행한 게임 효과/저장 자료는 버리지 않는다.
bool UDialogueRestoreController::CancelRestore()
{
	if (bHandling || Loader == nullptr) return false;
	bWaiting = false;
	Loader->EndSession();
	SourceGraph = nullptr;
	return true;
}

// 사망/새게임 초기화는 등록된 후보 로더와 복원 로더의 늦은 완료를 모두 무효화한다.
bool UDialogueRestoreController::ResetStoryAndSave(bool bNewGame)
{
	if (Session == nullptr || bHandling) return false;
	const bool bAccepted = Session->ResetStoryAndSave(bNewGame);
	if (bAccepted || Session->HasPendingCapture())
	{
		CancelRestore();
		if (CandidateLoader) CandidateLoader->EndSession();
	}
	return bAccepted;
}
