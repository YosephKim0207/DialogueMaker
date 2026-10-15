#include "DialogueSaveQueue.h"

// 세대 변경은 진행 중 요청을 폐기하는 API가 아니다.
bool UDialogueSaveQueue::InitializeQueue(FGuid NewGeneration)
{
	if (NewGeneration.IsValid() == false || NewGeneration == Generation || Pending.IsEmpty() == false || bPumping || bCompleting)
	{
		return false;
	}
	Generation = NewGeneration;
	LastSequence = 0;
	SeenRequests.Reset();
	SeenCheckpoints.Reset();
	bWriting = false;
	bFailed = false;
	return true;
}

// 게임 자료와 대화 요청을 모두 복사한 뒤에만 접수 성공을 반환한다.
FDialogueCheckpointAcceptance UDialogueSaveQueue::Enqueue(const FDialogueCheckpointRequest& Request, const TArray<uint8>& GamePayload)
{
	FDialogueCheckpointAcceptance Result;
	if (Request.Validate(Result.ErrorCode) == false)
	{
		return Result;
	}
	if (Generation.IsValid() == false || Request.QueueGenerationId != Generation || Request.Sequence <= LastSequence ||
		SeenRequests.Contains(Request.RequestId) || SeenCheckpoints.Contains(Request.Snapshot.CheckpointId) || GamePayload.IsEmpty())
	{
		Result.ErrorCode = TEXT("InvalidSaveQueueRequest");
		return Result;
	}
	FDialogueQueuedSave Save;
	Save.Request = Request;
	Save.GamePayload = GamePayload;
	Pending.Add(MoveTemp(Save));
	SeenRequests.Add(Request.RequestId);
	SeenCheckpoints.Add(Request.Snapshot.CheckpointId);
	LastSequence = Request.Sequence;
	Result.bAccepted = true;
	Result.ErrorCode = NAME_None;
	Pump();
	return Result;
}

// 실패한 항목을 추월하지 않으며 이벤트 중 동기 완료도 지원한다.
void UDialogueSaveQueue::Pump()
{
	if (bPumping || bCompleting)
	{
		return;
	}
	TGuardValue<bool> Guard(bPumping, true);
	while (Pending.IsEmpty() == false && bWriting == false && bFailed == false)
	{
		bWriting = true;
		const FDialogueQueuedSave Frozen = Pending[0];
		if (OnWriteRequestedNative.IsBound() == OnWriteRequested.IsBound())
		{
			FDialogueCheckpointCompletion Failure;
			Failure.RequestId = Frozen.Request.RequestId;
			Failure.QueueGenerationId = Frozen.Request.QueueGenerationId;
			Failure.Sequence = Frozen.Request.Sequence;
			Failure.ErrorCode = TEXT("InvalidSaveWriterBinding");
			CompleteWrite(Failure);
			break;
		}
		{
			TGuardValue<bool> Announce(bAnnouncing, true);
			OnSaveStateChanged.Broadcast(true, false);
		}
		OnWriteRequestedNative.ExecuteIfBound(Frozen);
		// native 저장기가 동기 완료했다면 같은 쓰기를 BP 저장기에 중복 전달하지 않는다.
		if (bWriting && Pending.IsEmpty() == false && Pending[0].Request.RequestId == Frozen.Request.RequestId)
		{
			OnWriteRequested.Broadcast(Frozen);
		}
	}
}

// 실패는 head를 유지하고 성공은 정확히 해당 head만 제거한다.
bool UDialogueSaveQueue::CompleteWrite(const FDialogueCheckpointCompletion& Completion)
{
	if (bCompleting || bAnnouncing || bWriting == false || Pending.IsEmpty() ||
		UDialogueCheckpointLibrary::MatchesCompletion(Pending[0].Request, Completion) == false)
	{
		// 개발자는 오래된/중복 완료가 반복되는 경우 프로젝트 진단 전송을 여기에 연결할 수 있다.
		return false;
	}
	{
		TGuardValue<bool> Guard(bCompleting, true);
		bWriting = false;
		bFailed = Completion.bSaved == false;
		if (Completion.bSaved)
		{
			LastSuccessfulSave = Pending[0];
			Pending.RemoveAt(0);
		}
		if (bFailed)
		{
			FDialogueDiagnostic Diagnostic;
			Diagnostic.Code = Completion.ErrorCode;
			Diagnostic.Message = TEXT("FIFO 저장 요청이 실패했습니다.");
			Diagnostic.Handling = TEXT("KeepFailedHeadAndWaitForRetry");
			Diagnostic.Context.Block = Pending[0].Request.Snapshot.Resume.Block;
			Diagnostic.Context.NodeGuid = Pending[0].Request.Snapshot.Trigger.OriginNodeGuid;
			Diagnostic.Context.ExecutionId = Pending[0].Request.Snapshot.Trigger.ExecutionId;
			Diagnostic.Context.SaveRequestId = Completion.RequestId;
			UDialogueDiagnosticRouter::Dispatch(Diagnostic, DiagnosticRouter);
		}
		// 오류값으로 게임이 재시도/종료 취소/미저장 종료 안내를 결정한다.
		// 실제 개발자 로그 전송은 이 완료 이벤트 구독 지점에 연결한다.
		OnWriteCompleted.Broadcast(Completion);
		OnSaveStateChanged.Broadcast(false, bFailed);
	}
	Pump();
	return true;
}

// 재시도는 뒤 요청보다 먼저 같은 스냅샷을 기록한다.
bool UDialogueSaveQueue::RetryFailedWrite()
{
	if (bFailed == false || bWriting || bCompleting || bPumping || Pending.IsEmpty())
	{
		return false;
	}
	Pending[0].Request.RequestId = FGuid::NewGuid();
	SeenRequests.Add(Pending[0].Request.RequestId);
	bFailed = false;
	Pump();
	return true;
}
