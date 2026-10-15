#include "DialogueEffectContract.h"

// 수량 감소는 양수 Remove로 표현하며 부호/overflow 처리는 게임 제공자가 검사한다.
bool FDialogueEffect::IsValid() const
{
	if (Key.IsValid() == false)
	{
		return false;
	}
	switch (Key.Kind)
	{
	case EDialogueStateKind::Tag:
		return Operation == EDialogueEffectOperation::Add || Operation == EDialogueEffectOperation::Remove;
	case EDialogueStateKind::Value:
		return (Operation == EDialogueEffectOperation::Add || Operation == EDialogueEffectOperation::Set) && FMath::IsFinite(Value);
	case EDialogueStateKind::Count:
		return static_cast<uint8>(Operation) <= static_cast<uint8>(EDialogueEffectOperation::Remove) && Count >= 0;
	default:
		return false;
	}
}

// 실제 적용하는 필드만 비교하여 비활성 Details 값의 차이를 무시한다.
bool FDialogueEffect::operator==(const FDialogueEffect& Other) const
{
	return Key == Other.Key && Operation == Other.Operation &&
		(Key.Kind == EDialogueStateKind::Tag || (Key.Kind == EDialogueStateKind::Value ? Value == Other.Value : Count == Other.Count));
}

// 효과 없는 선택은 이 API를 호출하지 않고 선택적 저장 경로를 사용한다.
bool FDialogueEffectBatch::IsValid() const
{
	return Context.ExecutionId.IsValid() && Effects.IsEmpty() == false &&
		Effects.ContainsByPredicate([](const FDialogueEffect& Effect) { return Effect.IsValid() == false; }) == false;
}

// 같은 요청의 다른 내용은 재전달로 간주하지 않는다.
bool FDialogueEffectBatch::HasSamePayload(const FDialogueEffectBatch& Other) const
{
	return Context.ExecutionId == Other.Context.ExecutionId && Context.Block == Other.Context.Block &&
		Context.NodeGuid == Other.Context.NodeGuid && Context.BlockRunId == Other.Context.BlockRunId &&
		ChoiceId == Other.ChoiceId && Effects == Other.Effects;
}

// 미구현 제공자를 성공으로 간주하지 않는다.
FDialogueEffectResult IDialogueEffectProvider::ApplyDialogueEffects_Implementation(const FDialogueEffectBatch& Batch)
{
	return FDialogueEffectResult();
}

// 경계 검증 후 게임 제공자에 한 번만 전달하고 실패를 조건의 True 대체로 처리하지 않는다.
FDialogueEffectResult UDialogueEffectLibrary::ApplyEffectBatch(const FDialogueEffectBatch& Batch, UObject* Provider, UDialogueDiagnosticRouter* Router)
{
	FDialogueEffectResult Result;
	if (Batch.IsValid() == false)
	{
		Result.ErrorCode = TEXT("InvalidEffectBatch");
	}
	else if (IsValid(Provider) == false || Provider->GetClass()->ImplementsInterface(UDialogueEffectProvider::StaticClass()) == false)
	{
		Result.ErrorCode = TEXT("MissingEffectProvider");
	}
	else
	{
		Result = IDialogueEffectProvider::Execute_ApplyDialogueEffects(Provider, Batch);
		if (Result.Status != EDialogueEffectStatus::Failed && Result.IsSuccess() == false)
		{
			Result.Status = EDialogueEffectStatus::Failed;
			Result.ErrorCode = TEXT("InvalidEffectResult");
		}
	}
	if (Result.IsSuccess() == false)
	{
		if (Result.ErrorCode.IsNone())
		{
			Result.ErrorCode = TEXT("EffectRejected");
		}
		FDialogueDiagnostic Diagnostic;
		Diagnostic.Code = Result.ErrorCode;
		Diagnostic.Context = Batch.Context;
		Diagnostic.Message = Result.Message.IsEmpty() ? TEXT("Effect batch failed.") : Result.Message;
		Diagnostic.Handling = TEXT("ReturnFailureToGame");
		// 개발자는 Router에 로그 전송을 연결하고 반환값으로 안내/재시도를 구현한다.
		UDialogueDiagnosticRouter::Dispatch(MoveTemp(Diagnostic), Router);
	}
	if (Result.IsSuccess())
	{
		Result.ErrorCode = NAME_None;
	}
	return Result;
}
