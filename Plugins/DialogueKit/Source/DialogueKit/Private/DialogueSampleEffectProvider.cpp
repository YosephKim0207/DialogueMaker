#include "DialogueSampleEffectProvider.h"

// 모든 효과를 임시 값에 적용한 뒤 상태와 실행 기록을 같은 호출에서 확정한다.
FDialogueEffectResult UDialogueSampleEffectProvider::ApplyDialogueEffects_Implementation(const FDialogueEffectBatch& Batch)
{
	FDialogueEffectResult Result;
	if (Batch.IsValid() == false)
	{
		Result.ErrorCode = TEXT("InvalidEffectBatch");
		return Result;
	}
	const FGuid ExecutionId = Batch.Context.ExecutionId;
	if (Requests.Contains(ExecutionId) != Results.Contains(ExecutionId))
	{
		Result.ErrorCode = TEXT("InvalidExecutionLedger");
		return Result;
	}
	if (const FDialogueEffectBatch* Previous = Requests.Find(ExecutionId))
	{
		if (Previous->HasSamePayload(Batch) == false)
		{
			Result.ErrorCode = TEXT("ExecutionPayloadConflict");
			return Result;
		}
		const FDialogueEffectResult* PreviousResult = Results.Find(ExecutionId);
		if (PreviousResult == nullptr)
		{
			Result.ErrorCode = TEXT("InvalidExecutionLedger");
			return Result;
		}
		Result = *PreviousResult;
		if (Result.IsSuccess())
		{
			Result.Status = EDialogueEffectStatus::AlreadyApplied;
		}
		return Result;
	}
	TArray<FDialogueSampleState> Pending = States;
	Result.ErrorCode = NAME_None;
	for (const FDialogueEffect& Effect : Batch.Effects)
	{
		FDialogueSampleState Current;
		if (GetSampleState(Effect.Key, Current) == false)
		{
			Result.ErrorCode = TEXT("UnknownEffectTarget");
			break;
		}
		int32 Index = Pending.IndexOfByPredicate([&](const FDialogueSampleState& Item) { return Item.Key == Effect.Key; });
		if (Index == INDEX_NONE)
		{
			Index = Pending.Add(Current);
		}
		FDialogueSampleState& State = Pending[Index];
		if (Effect.Key.Kind == EDialogueStateKind::Tag)
		{
			State.bTagValue = Effect.Operation == EDialogueEffectOperation::Add;
		}
		else if (Effect.Key.Kind == EDialogueStateKind::Value)
		{
			// Set은 손상된 Value도 유한한 값으로 교체할 수 있다.
			const double NewValue = Effect.Operation == EDialogueEffectOperation::Set ? Effect.Value : State.Value + Effect.Value;
			if (FMath::IsFinite(NewValue) == false)
			{
				Result.ErrorCode = TEXT("ValueOverflow");
				break;
			}
			State.Value = NewValue;
		}
		else
		{
			if (State.Count < 0)
			{
				Result.ErrorCode = TEXT("InvalidStoredCount");
				break;
			}
			if (Effect.Operation == EDialogueEffectOperation::Set)
			{
				State.Count = Effect.Count;
			}
			else if (Effect.Operation == EDialogueEffectOperation::Add)
			{
				if (State.Count > MAX_int64 - Effect.Count)
				{
					Result.ErrorCode = TEXT("CountOverflow");
					break;
				}
				State.Count += Effect.Count;
			}
			else
			{
				if (State.Count < Effect.Count)
				{
					Result.ErrorCode = TEXT("InsufficientCount");
					break;
				}
				State.Count -= Effect.Count;
			}
		}
	}
	if (Result.ErrorCode.IsNone())
	{
		States = MoveTemp(Pending);
		Result.Status = EDialogueEffectStatus::Applied;
	}
	// 실제 게임은 이 상태/실행 기록과 대화 재개 위치를 함께 저장해야 한다.
	Requests.Add(ExecutionId, Batch);
	Results.Add(ExecutionId, Result);
	return Result;
}

// 자료 손상과 알 수 없는 버전을 상태 교체 전에 거부한다.
bool FDialogueSampleEffectSnapshot::Validate(FName& OutError) const
{
	OutError = TEXT("InvalidEffectSnapshot");
	if (SchemaVersion != 1 || Requests.Num() != Results.Num())
	{
		return false;
	}
	TSet<FDialogueStateKey> Keys;
	for (const FDialogueStateDefinition& Definition : Definitions)
	{
		if (Definition.Key.IsValid() == false || Keys.Contains(Definition.Key))
		{
			return false;
		}
		Keys.Add(Definition.Key);
	}
	TSet<FDialogueStateKey> StateKeys;
	for (const FDialogueSampleState& State : States)
	{
		if (Keys.Contains(State.Key) == false || StateKeys.Contains(State.Key) ||
			(State.Key.Kind == EDialogueStateKind::Value && FMath::IsFinite(State.Value) == false) ||
			(State.Key.Kind == EDialogueStateKind::Count && State.Count < 0))
		{
			return false;
		}
		StateKeys.Add(State.Key);
	}
	for (const TPair<FGuid, FDialogueEffectBatch>& Entry : Requests)
	{
		const FDialogueEffectResult* Result = Results.Find(Entry.Key);
		if (Entry.Value.IsValid() == false || Entry.Key != Entry.Value.Context.ExecutionId || Result == nullptr)
		{
			return false;
		}
		if ((Result->Status != EDialogueEffectStatus::Applied && Result->Status != EDialogueEffectStatus::Failed) ||
			(Result->IsSuccess() ? Result->ErrorCode.IsNone() == false : Result->ErrorCode.IsNone()))
		{
			return false;
		}
		if (Result->IsSuccess())
		{
			for (const FDialogueEffect& Effect : Entry.Value.Effects)
			{
				if (Keys.Contains(Effect.Key) == false || StateKeys.Contains(Effect.Key) == false)
				{
					return false;
				}
			}
		}
	}
	TSet<FGuid> CorrectionIds;
	TSet<FGuid> Corrected;
	for (const FDialogueSampleCorrectionReceipt& Receipt : Corrections)
	{
		if (Receipt.CorrectionId.IsValid() == false || CorrectionIds.Contains(Receipt.CorrectionId) || Receipt.RuleFingerprint.IsEmpty() ||
			Receipt.Source.IsValid() == false || Receipt.BlockRunId.IsValid() == false || Receipt.Executions.IsEmpty()) return false;
		CorrectionIds.Add(Receipt.CorrectionId);
		for (FGuid Id : Receipt.Executions)
		{
			const FDialogueEffectBatch* Batch = Requests.Find(Id);
			const FDialogueEffectResult* Result = Results.Find(Id);
			if (Corrected.Contains(Id) || Batch == nullptr || Result == nullptr || Result->IsSuccess() == false ||
				(Batch->Context.Block == Receipt.Source) == false || Batch->Context.BlockRunId != Receipt.BlockRunId) return false;
			for (const FDialogueEffect& Effect : Batch->Effects)
			{
				if (Effect.Key.Kind != EDialogueStateKind::Count || Effect.Operation != EDialogueEffectOperation::Add || Effect.Count < 0) return false;
			}
			Corrected.Add(Id);
		}
	}
	OutError = NAME_None;
	return true;
}

// 상태와 원장을 같은 호출에서 값 복사한다. 게임 스레드에서만 사용한다.
FDialogueSampleEffectSnapshot UDialogueSampleEffectProvider::CaptureEffectSnapshot() const
{
	FDialogueSampleEffectSnapshot Snapshot;
	Snapshot.Definitions = Definitions;
	Snapshot.States = States;
	Snapshot.Requests = Requests;
	Snapshot.Results = Results;
	Snapshot.Corrections = Corrections;
	return Snapshot;
}

// 모든 검사 이후에만 상태를 교체하여 복원 실패의 부분 적용을 막는다.
bool UDialogueSampleEffectProvider::RestoreEffectSnapshot(const FDialogueSampleEffectSnapshot& Snapshot, FName& OutError)
{
	if (Snapshot.Validate(OutError) == false)
	{
		// 게임 개발자는 이 오류 반환 지점에 프로젝트 로그 전송을 연결할 수 있다.
		return false;
	}
	Definitions = Snapshot.Definitions;
	States = Snapshot.States;
	Requests = Snapshot.Requests;
	Results = Snapshot.Results;
	Corrections = Snapshot.Corrections;
	return true;
}

// 원장으로 입증된 Count Add만 제거한다. 소비/설정/외부 쓰기가 섞이면 전량 거부한다.
FDialogueEffectResult UDialogueSampleEffectProvider::CorrectDialogueState_Implementation(const FDialogueCorrectionRequest& Request)
{
	FDialogueEffectResult Failure;
	Failure.ErrorCode = TEXT("UnsupportedSampleCorrection");
	FName Error;
	if (Request.Mode != EDialogueCorrectionMode::SelectiveRollback || Request.CorrectionId.IsValid() == false || Request.RuleFingerprint.IsEmpty() ||
		Request.Source.Validate(Error) == false || Request.Source.RunEvidence.bComplete == false || Request.Effects.IsEmpty()) return Failure;
	TArray<FGuid> Selected;
	for (const FDialogueEffectEvidence& Evidence : Request.Effects)
	{
		if (Selected.Contains(Evidence.ExecutionId)) return Failure;
		const FDialogueEffectEvidence* Saved = Request.Source.RunEvidence.Effects.FindByPredicate([&](const FDialogueEffectEvidence& Item) { return Item.ExecutionId == Evidence.ExecutionId; });
		if (Saved == nullptr || Saved->NodeGuid != Evidence.NodeGuid || Saved->ChoiceId != Evidence.ChoiceId || Saved->LineGuid != Evidence.LineGuid) return Failure;
		const FDialogueEffectBatch* Batch = Requests.Find(Evidence.ExecutionId);
		const FDialogueEffectResult* Result = Results.Find(Evidence.ExecutionId);
		if (Batch == nullptr || Result == nullptr || Result->IsSuccess() == false || (Batch->Context.Block == Request.Source.Resume.Block) == false ||
			Batch->Context.BlockRunId != Request.Source.Resume.BlockRunId || Batch->Context.NodeGuid != Evidence.NodeGuid || Batch->ChoiceId != Evidence.ChoiceId) return Failure;
		Selected.Add(Evidence.ExecutionId);
	}
	if (const FDialogueSampleCorrectionReceipt* Existing = Corrections.FindByPredicate([&](const FDialogueSampleCorrectionReceipt& Receipt) { return Receipt.CorrectionId == Request.CorrectionId; }))
	{
		if (Existing->RuleFingerprint != Request.RuleFingerprint || Existing->Executions != Selected || (Existing->Source == Request.Source.Resume.Block) == false || Existing->BlockRunId != Request.Source.Resume.BlockRunId)
		{
			Failure.ErrorCode = TEXT("CorrectionPayloadConflict");
			return Failure;
		}
		Failure.Status = EDialogueEffectStatus::AlreadyApplied;
		Failure.ErrorCode = NAME_None;
		return Failure;
	}
	TSet<FGuid> AlreadyCorrected;
	for (const FDialogueSampleCorrectionReceipt& Receipt : Corrections)
	{
		for (FGuid Id : Receipt.Executions) AlreadyCorrected.Add(Id);
	}
	TMap<FDialogueStateKey, int64> Remove;
	for (FGuid Id : Selected)
	{
		if (AlreadyCorrected.Contains(Id))
		{
			Failure.ErrorCode = TEXT("ExecutionAlreadyCorrected");
			return Failure;
		}
		for (const FDialogueEffect& Effect : Requests.FindChecked(Id).Effects)
		{
			if (Effect.Key.Kind != EDialogueStateKind::Count || Effect.Operation != EDialogueEffectOperation::Add || Effect.Count < 0) return Failure;
			int64& Total = Remove.FindOrAdd(Effect.Key);
			if (Total > MAX_int64 - Effect.Count) return Failure;
			Total += Effect.Count;
		}
	}
	TArray<FDialogueSampleState> CorrectedStates = States;
	for (const auto& Removal : Remove)
	{
		int64 Expected = 0;
		for (const auto& Entry : Requests)
		{
			const FDialogueEffectResult* Result = Results.Find(Entry.Key);
			if (Result == nullptr || Result->IsSuccess() == false) continue;
			for (const FDialogueEffect& Effect : Entry.Value.Effects)
			{
				if ((Effect.Key == Removal.Key) == false) continue;
				if (Effect.Operation != EDialogueEffectOperation::Add || Effect.Count < 0) return Failure;
				if (AlreadyCorrected.Contains(Entry.Key)) continue;
				if (Expected > MAX_int64 - Effect.Count) return Failure;
				Expected += Effect.Count;
			}
		}
		FDialogueSampleState* State = CorrectedStates.FindByPredicate([&](const FDialogueSampleState& Value) { return Value.Key == Removal.Key; });
		if (State == nullptr || State->Count != Expected || Expected < Removal.Value)
		{
			Failure.ErrorCode = TEXT("UnprovenSampleCountProvenance");
			return Failure;
		}
		State->Count -= Removal.Value;
	}
	FDialogueSampleCorrectionReceipt Receipt;
	Receipt.CorrectionId = Request.CorrectionId;
	Receipt.RuleFingerprint = Request.RuleFingerprint;
	Receipt.Source = Request.Source.Resume.Block;
	Receipt.BlockRunId = Request.Source.Resume.BlockRunId;
	Receipt.Executions = MoveTemp(Selected);
	States = MoveTemp(CorrectedStates);
	Corrections.Add(MoveTemp(Receipt));
	Failure.Status = EDialogueEffectStatus::Applied;
	Failure.ErrorCode = NAME_None;
	return Failure;
}
