#include "DialogueCheckpointContract.h"

// 종료/시작 위치에는 임의 NodeGuid를 남기지 않는다.
bool FDialogueResumePosition::IsValid() const
{
	if (Kind == EDialogueResumeKind::None)
	{
		return Block.BlockId.IsValid() == false && BlockRunId.IsValid() == false &&
			NodeGuid.IsValid() == false && TargetTag.IsValid() == false;
	}
	if (Block.IsValid() == false || BlockRunId.IsValid() == false || TargetTag.IsValid() == false)
	{
		return false;
	}
	if (Kind == EDialogueResumeKind::BeforeNode)
	{
		return NodeGuid.IsValid();
	}
	return (Kind == EDialogueResumeKind::BlockStart || Kind == EDialogueResumeKind::BlockCompleted) && NodeGuid.IsValid() == false;
}

// 알려지지 않은 저장 스키마는 자동 해석하지 않는다. 게임의 명시적 이전 처리 후 다시 검증한다.
bool FDialogueCheckpointSnapshot::Validate(FName& OutErrorCode) const
{
	OutErrorCode = TEXT("InvalidCheckpointSnapshot");
	if (SchemaVersion != 1 && SchemaVersion != 2)
	{
		OutErrorCode = TEXT("UnsupportedCheckpointSchema");
		return false;
	}
	if (CheckpointId.IsValid() == false || Resume.IsValid() == false)
	{
		return false;
	}
	bool bValidTrigger = false;
	switch (Trigger.Reason)
	{
	case EDialogueCheckpointReason::GameSave:
		bValidTrigger = Trigger.OriginNodeGuid.IsValid() == false && Trigger.ChoiceId.IsValid() == false &&
			Trigger.ExecutionId.IsValid() == false && Trigger.bHasEffects == false;
		break;
	case EDialogueCheckpointReason::Choice:
		bValidTrigger = Resume.Kind == EDialogueResumeKind::BeforeNode && Trigger.OriginNodeGuid.IsValid() &&
			Trigger.ChoiceId.IsValid() && Trigger.bHasEffects == Trigger.ExecutionId.IsValid();
		break;
	case EDialogueCheckpointReason::StateChange:
		bValidTrigger = Resume.Kind == EDialogueResumeKind::BeforeNode && Trigger.OriginNodeGuid.IsValid() &&
			Trigger.ChoiceId.IsValid() == false && Trigger.ExecutionId.IsValid() && Trigger.bHasEffects;
		break;
	case EDialogueCheckpointReason::BlockEnd:
		bValidTrigger = Resume.Kind == EDialogueResumeKind::BlockCompleted && Trigger.OriginNodeGuid.IsValid() &&
			Trigger.ChoiceId.IsValid() == false && Trigger.ExecutionId.IsValid() == false && Trigger.bHasEffects == false;
		break;
	case EDialogueCheckpointReason::Replacement:
		bValidTrigger = Resume.Kind == EDialogueResumeKind::BlockStart && Trigger.ReplacedFromBlock.IsValid() &&
			Trigger.OriginNodeGuid.IsValid() == false && Trigger.ChoiceId.IsValid() == false &&
			Trigger.bHasEffects == false && Trigger.ExecutionId.IsValid() == false;
		break;
	default:
		break;
	}
	if (bValidTrigger == false || (Trigger.Reason != EDialogueCheckpointReason::Replacement && Trigger.ReplacedFromBlock.BlockId.IsValid()))
	{
		OutErrorCode = TEXT("InvalidCheckpointTrigger");
		return false;
	}
	if (SchemaVersion == 1 && (PendingChoices.IsEmpty() == false || RunEvidence.bComplete || RunEvidence.LastPassedLine.IsValid() ||
		RunEvidence.PassedLines.IsEmpty() == false || RunEvidence.Effects.IsEmpty() == false || AppliedCorrections.IsEmpty() == false))
	{
		OutErrorCode = TEXT("UnexpectedLegacyEvidence");
		return false;
	}
	TSet<FGuid> Occurrences;
	for (const FDialogueChoiceRecord& Choice : PendingChoices)
	{
		if (Choice.OccurrenceId.IsValid() == false || Occurrences.Contains(Choice.OccurrenceId) ||
			Choice.Block.IsValid() == false || Choice.BlockRunId.IsValid() == false ||
			Choice.NodeGuid.IsValid() == false || Choice.ChoiceId.IsValid() == false)
		{
			OutErrorCode = TEXT("InvalidChoiceRecord");
			return false;
		}
		Occurrences.Add(Choice.OccurrenceId);
	}
	if (SchemaVersion == 2 && Trigger.Reason == EDialogueCheckpointReason::Choice &&
		PendingChoices.ContainsByPredicate([&](const FDialogueChoiceRecord& Choice)
		{
			return Choice.Block == Resume.Block && Choice.BlockRunId == Resume.BlockRunId &&
				Choice.NodeGuid == Trigger.OriginNodeGuid && Choice.ChoiceId == Trigger.ChoiceId && Choice.ExecutionId == Trigger.ExecutionId;
		}) == false)
	{
		OutErrorCode = TEXT("MissingTriggerChoiceRecord");
		return false;
	}
	TSet<FGuid> Lines;
	for (FGuid Line : RunEvidence.PassedLines)
	{
		if (Line.IsValid() == false || Lines.Contains(Line))
		{
			OutErrorCode = TEXT("InvalidPassedLineEvidence");
			return false;
		}
		Lines.Add(Line);
	}
	if (RunEvidence.LastPassedLine.IsValid() && Lines.Contains(RunEvidence.LastPassedLine) == false)
	{
		OutErrorCode = TEXT("InvalidLastPassedLine");
		return false;
	}
	TSet<FGuid> Executions;
	for (const FDialogueEffectEvidence& Effect : RunEvidence.Effects)
	{
		if (Effect.ExecutionId.IsValid() == false || Effect.NodeGuid.IsValid() == false || Executions.Contains(Effect.ExecutionId) ||
			(Effect.LineGuid.IsValid() && Lines.Contains(Effect.LineGuid) == false))
		{
			OutErrorCode = TEXT("InvalidEffectEvidence");
			return false;
		}
		Executions.Add(Effect.ExecutionId);
	}
	if (SchemaVersion == 2 && Trigger.bHasEffects && RunEvidence.Effects.ContainsByPredicate([&](const FDialogueEffectEvidence& Effect)
		{ return Effect.ExecutionId == Trigger.ExecutionId && Effect.NodeGuid == Trigger.OriginNodeGuid && Effect.ChoiceId == Trigger.ChoiceId; }) == false)
	{
		OutErrorCode = TEXT("MissingTriggerEffectEvidence");
		return false;
	}
	if (Resume.Kind == EDialogueResumeKind::None && (RunEvidence.bComplete || Lines.IsEmpty() == false || Executions.IsEmpty() == false || PendingChoices.IsEmpty() == false))
	{
		OutErrorCode = TEXT("EvidenceWithoutActiveRun");
		return false;
	}
	TSet<FGuid> Corrections;
	for (FGuid Correction : AppliedCorrections)
	{
		if (Correction.IsValid() == false || Corrections.Contains(Correction))
		{
			OutErrorCode = TEXT("InvalidCorrectionHistory");
			return false;
		}
		Corrections.Add(Correction);
	}
	TSet<FGuid> BlockIds;
	for (const FDialogueBlockReadRecord& BlockRecord : FullReadHistory)
	{
		if (BlockRecord.BlockId.IsValid() == false || BlockIds.Contains(BlockRecord.BlockId))
		{
			OutErrorCode = TEXT("InvalidBlockReadHistory");
			return false;
		}
		BlockIds.Add(BlockRecord.BlockId);
		TSet<FGuid> NodeIds;
		for (const FDialogueLineReadRecord& Line : BlockRecord.Lines)
		{
			if (Line.NodeGuid.IsValid() == false || Line.ReadVersion < 1 || NodeIds.Contains(Line.NodeGuid))
			{
				OutErrorCode = TEXT("InvalidLineReadHistory");
				return false;
			}
			NodeIds.Add(Line.NodeGuid);
		}
	}
	if (Resume.Kind == EDialogueResumeKind::BlockCompleted && FullReadHistory.ContainsByPredicate([&](const FDialogueBlockReadRecord& Record)
		{ return Record.BlockId == Resume.Block.BlockId && Record.bCompleted; }) == false)
	{
		OutErrorCode = TEXT("MissingCompletedBlockHistory");
		return false;
	}
	OutErrorCode = NAME_None;
	return true;
}

// 요청 식별자는 게임 공통 저장 조율자가 발급한다.
bool FDialogueCheckpointRequest::Validate(FName& OutErrorCode) const
{
	OutErrorCode = TEXT("InvalidCheckpointRequest");
	return RequestId.IsValid() && QueueGenerationId.IsValid() && Sequence > 0 && Snapshot.Validate(OutErrorCode);
}

// 제공자가 저장 요청을 구현하지 않으면 명시적으로 실패한다.
FDialogueCheckpointAcceptance IDialogueCheckpointProvider::RequestDialogueCheckpoint_Implementation(const FDialogueCheckpointRequest& Request)
{
	return FDialogueCheckpointAcceptance();
}

// 성공은 접수만 뜻한다. 게임은 반환값을 확인해 이후 오류 안내와 흐름을 처리한다.
FDialogueCheckpointAcceptance UDialogueCheckpointLibrary::SubmitCheckpoint(const FDialogueCheckpointRequest& Request,
	UObject* Provider, UDialogueDiagnosticRouter* Router)
{
	FDialogueCheckpointAcceptance Result;
	if (Request.Validate(Result.ErrorCode) == false)
	{
		// 입력 오류는 게임 제공자 호출 전에 거부한다.
	}
	else if (IsValid(Provider) == false || Provider->GetClass()->ImplementsInterface(UDialogueCheckpointProvider::StaticClass()) == false)
	{
		Result.ErrorCode = TEXT("MissingCheckpointProvider");
	}
	else
	{
		Result = IDialogueCheckpointProvider::Execute_RequestDialogueCheckpoint(Provider, Request);
	}
	if (Result.bAccepted)
	{
		Result.ErrorCode = NAME_None;
	}
	else
	{
		if (Result.ErrorCode.IsNone())
		{
			Result.ErrorCode = TEXT("CheckpointRejected");
		}
		FDialogueDiagnostic Diagnostic;
		Diagnostic.Code = Result.ErrorCode;
		Diagnostic.Message = Result.Message.IsEmpty() ? TEXT("Checkpoint request rejected.") : Result.Message;
		Diagnostic.Context.Block = Request.Snapshot.Resume.Block;
		Diagnostic.Context.NodeGuid = Request.Snapshot.Trigger.OriginNodeGuid;
		Diagnostic.Context.ExecutionId = Request.Snapshot.Trigger.ExecutionId;
		Diagnostic.Context.SaveRequestId = Request.RequestId;
		Diagnostic.Handling = TEXT("KeepLastSuccessfulSave");
		// 개발자는 Router에 제작사 로그 전송을 연결한다. 요청 실패를 저장 성공으로 처리하지 않는다.
		UDialogueDiagnosticRouter::Dispatch(MoveTemp(Diagnostic), Router);
	}
	return Result;
}

// 이전 세대/이전 재시도 결과를 현재 요청의 완료로 처리하지 않는다.
bool UDialogueCheckpointLibrary::MatchesCompletion(const FDialogueCheckpointRequest& PendingRequest,
	const FDialogueCheckpointCompletion& Completion)
{
	FName ErrorCode;
	return PendingRequest.Validate(ErrorCode) && Completion.RequestId == PendingRequest.RequestId &&
		Completion.QueueGenerationId == PendingRequest.QueueGenerationId && Completion.Sequence == PendingRequest.Sequence &&
		(Completion.bSaved ? Completion.ErrorCode.IsNone() : Completion.ErrorCode.IsNone() == false);
}

// 기존 버전의 마지막 선택/효과만 옮긴다. 과거 통과 목록이나 지급 이력을 만들어내지 않는다.
bool FDialogueCheckpointSnapshot::UpgradeToLatest(FDialogueCheckpointSnapshot& OutSnapshot, FName& OutErrorCode) const
{
	if (Validate(OutErrorCode) == false)
	{
		return false;
	}
	FDialogueCheckpointSnapshot Converted = *this;
	if (SchemaVersion == 1)
	{
		Converted.SchemaVersion = 2;
		Converted.RunEvidence.bComplete = false;
		if (Trigger.Reason == EDialogueCheckpointReason::Choice)
		{
			FDialogueChoiceRecord Choice;
			Choice.OccurrenceId = CheckpointId;
			Choice.Block = Resume.Block;
			Choice.BlockRunId = Resume.BlockRunId;
			Choice.NodeGuid = Trigger.OriginNodeGuid;
			Choice.ChoiceId = Trigger.ChoiceId;
			Choice.ExecutionId = Trigger.ExecutionId;
			Converted.PendingChoices.Add(Choice);
		}
		if (Trigger.bHasEffects)
		{
			FDialogueEffectEvidence Effect;
			Effect.ExecutionId = Trigger.ExecutionId;
			Effect.NodeGuid = Trigger.OriginNodeGuid;
			Effect.ChoiceId = Trigger.ChoiceId;
			Converted.RunEvidence.Effects.Add(Effect);
		}
	}
	if (Converted.Validate(OutErrorCode) == false)
	{
		return false;
	}
	OutSnapshot = MoveTemp(Converted);
	return true;
}
