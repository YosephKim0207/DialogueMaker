#include "DialogueCheckpointSession.h"

// 제공자와 외부 실행기의 소유/재진입 조건을 확인한 뒤 단일 기록 경계를 연결한다.
bool UDialogueCheckpointSession::Initialize(UObject* NewSaveProvider, UObject* NewStateProvider, UObject* NewEffectProvider, UDialogueDiagnosticRouter* NewRouter, UDialogueBlockRunner* ExistingRunner)
{
	if (Runner || IsValid(NewSaveProvider) == false || NewSaveProvider->GetClass()->ImplementsInterface(UDialogueCheckpointProvider::StaticClass()) == false ||
		(ExistingRunner && (ExistingRunner->CanStartBlock() == false || ExistingRunner->CheckpointCapture.IsBound() || ExistingRunner->CheckpointReady.IsBound() || ExistingRunner->LineVisited.IsBound() || ExistingRunner->LinePassed.IsBound()))) return false;
	SaveProvider = NewSaveProvider;
	StateProvider = NewStateProvider;
	EffectProvider = NewEffectProvider;
	Router = NewRouter;
	Runner = ExistingRunner ? ExistingRunner : NewObject<UDialogueBlockRunner>(this);
	State = NewObject<UDialogueCheckpointState>(this);
	Runner->CheckpointReady.BindUObject(this, &UDialogueCheckpointSession::Ready);
	Runner->CheckpointCapture.BindUObject(this, &UDialogueCheckpointSession::HandleBoundary);
	Runner->LineVisited.BindUObject(this, &UDialogueCheckpointSession::Visit);
	Runner->LinePassed.BindUObject(this, &UDialogueCheckpointSession::Pass);
	return true;
}

// 접수 실패 중 새 실행과 일반 저장으로 최초 자료를 덮지 않는다.
bool UDialogueCheckpointSession::CanOperate() const
{
	return Runner && State && bBusy == false && bPendingCapture == false;
}

// 저장 경계마다 호출한다. 파일 쓰기 대기와 메모리 캡처 불가를 구분한다.
bool UDialogueCheckpointSession::Ready() const
{
	return bPendingCapture == false && IsValid(SaveProvider) && IDialogueCheckpointProvider::Execute_CanCaptureDialogueCheckpoint(SaveProvider);
}

// 첫 자동 효과 전에 실행별 기록을 준비한다.
bool UDialogueCheckpointSession::StartBlock(UDialogueGraph* Block, FGameplayTag Target)
{
	if (CanOperate() == false || Target.IsValid() == false) return false;
	TGuardValue<bool> Guard(bBusy, true);
	if (Ready() == false) return false;
	if (Runner->PrepareBlock(Block, StateProvider, EffectProvider, Router) == false) return false;
	if (State->BeginBlock(Block->GetBlockReference(), Target, Runner->GetBlockRunId()) == false)
	{
		Runner->CancelBlock();
		return false;
	}
	ActiveBlock = Block;
	return Runner->RunPreparedBlock();
}

// 저장 검사를 완료한 뒤 정확 버전/실행 ID/위치로 재개한다.
bool UDialogueCheckpointSession::ResumeBlock(UDialogueGraph* Block, const FDialogueCheckpointSnapshot& Snapshot)
{
	if (CanOperate() == false) return false;
	TGuardValue<bool> Guard(bBusy, true);
	if (Ready() == false) return false;
	FDialogueCheckpointSnapshot Converted;
	if (Snapshot.UpgradeToLatest(Converted, LastError) == false) return false;
	if (Runner->PrepareResume(Block, Converted.Resume, StateProvider, EffectProvider, Router) == false) return false;
	if (State->RestoreSnapshot(Converted, LastError) == false)
	{
		Runner->CancelBlock();
		return false;
	}
	ActiveBlock = Block;
	return Runner->RunPreparedBlock();
}

// 표시 전에 열람 후보를 기록한다.
bool UDialogueCheckpointSession::Visit(FGuid Line, int32 Version)
{
	return bPendingCapture == false && State && State->VisitLine(Line, Version);
}

// 다음 효과가 실행되기 전에 이번 실행의 통과를 기록한다.
bool UDialogueCheckpointSession::Pass(FGuid Line)
{
	return bPendingCapture == false && State && State->PassLine(Line);
}

// 게임 제공자는 최초 게임 복사본까지 보존해야 한다. 실패 시 다음 진행을 중단한다.
bool UDialogueCheckpointSession::Capture(const FDialogueCheckpointSnapshot& Snapshot)
{
	Pending = Snapshot;
	bPendingCapture = true;
	const FDialogueCheckpointAcceptance Result = IDialogueCheckpointProvider::Execute_CaptureDialogueCheckpoint(SaveProvider, Pending);
	LastError = Result.ErrorCode;
	if (Result.bAccepted)
	{
		bPendingCapture = false;
		LastError = NAME_None;
	}
	return Result.bAccepted;
}

// 관찰 이벤트보다 먼저 캐시와 게임 합본을 확정한다.
bool UDialogueCheckpointSession::HandleBoundary(const FDialogueExecutionBoundary& Boundary)
{
	FDialogueCheckpointSnapshot Snapshot;
	if (bPendingCapture || State->ApplyBoundary(Boundary, Snapshot) == false) return false;
	if (Boundary.bSaveRequested == false) return true;
	bResumeAfterCapture = true;
	const bool bAccepted = Capture(Snapshot);
	if (bAccepted) bResumeAfterCapture = false;
	return bAccepted;
}

// 게임의 일반 저장도 같은 조율자를 사용한다.
bool UDialogueCheckpointSession::SaveCurrentState()
{
	if (CanOperate() == false || Runner->IsProcessing()) return false;
	TGuardValue<bool> Guard(bBusy, true);
	if (Ready() == false) return false;
	bResumeAfterCapture = false;
	return Capture(State->CaptureGameSave());
}

// 효과를 다시 적용하지 않고 보존된 같은 자료를 접수한 뒤 후속 노드로 진행한다.
bool UDialogueCheckpointSession::RetryCapture()
{
	if (Runner == nullptr || bBusy || bPendingCapture == false || Runner->IsProcessing()) return false;
	TGuardValue<bool> Guard(bBusy, true);
	const bool bContinue = bResumeAfterCapture;
	if (Capture(Pending) == false) return false;
	bResumeAfterCapture = false;
	return bContinue == false || Runner->ResumeBlock(ActiveBlock, Pending.Resume, StateProvider, EffectProvider, Router);
}

// 큐의 과거 완료는 occurrence 기준으로 정리하므로 새 실행 선택을 지우지 않는다.
void UDialogueCheckpointSession::AcknowledgeSaved(const FDialogueCheckpointSnapshot& Snapshot)
{
	if (State) State->AcknowledgeSavedSnapshot(Snapshot);
}

// 큐 완료는 실제 성공한 고정 snapshot을 제공한다.
void UDialogueCheckpointSession::Saved(const FDialogueQueuedSave& Save)
{
	AcknowledgeSaved(Save.Request.Snapshot);
}

// 게임이 소유한 공통 큐만 연결하며 실행 중 교체하지 않는다.
bool UDialogueCheckpointSession::AttachSaveQueue(UDialogueSaveQueue* Queue)
{
	if (CanOperate() == false || Runner->IsProcessing() || Queue == nullptr) return false;
	if (SaveQueue) return SaveQueue == Queue;
	if (Runner->CanStartBlock() == false || Queue->GetGeneration().IsValid() == false) return false;
	SaveQueue = Queue;
	SaveQueue->OnSavedNative.AddUObject(this, &UDialogueCheckpointSession::Saved);
	return true;
}

// 입력 콜백 중이나 미접수 상태에서는 초기화를 거부한다. 디스크 저장은 별도 호출한다.
bool UDialogueCheckpointSession::ResetStory(bool bNewGame)
{
	if (CanOperate() == false || Runner->CancelBlock() == false) return false;
	State->ResetStory(bNewGame);
	ActiveBlock = nullptr;
	return true;
}
