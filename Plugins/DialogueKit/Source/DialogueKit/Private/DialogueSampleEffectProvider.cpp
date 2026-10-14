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
