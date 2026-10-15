#include "DialogueCheckpointState.h"

// 신규 캡처는 확정된 스키마 2를 사용한다.
UDialogueCheckpointState::UDialogueCheckpointState()
{
	Cached.SchemaVersion = 2;
}

// Block 진입은 아직 디스크 체크포인트가 아니며 처음부터 재개할 기준만 만든다.
bool UDialogueCheckpointState::BeginBlock(FDialogueBlockReference Block, FGameplayTag TargetTag, FGuid BlockRunId)
{
	FDialogueResumePosition Position;
	Position.Kind = EDialogueResumeKind::BlockStart;
	Position.Block = Block;
	Position.BlockRunId = BlockRunId;
	Position.TargetTag = TargetTag;
	if (Position.IsValid() == false || (Cached.Resume.Kind != EDialogueResumeKind::None && Cached.Resume.Kind != EDialogueResumeKind::BlockCompleted))
	{
		return false;
	}
	Cached.Resume = Position;
	Cached.RunEvidence = FDialogueRunEvidence();
	Cached.RunEvidence.bComplete = true;
	PendingReads.Reset();
	CurrentLine.Invalidate();
	return true;
}

// 표시 위치와 미확정 읽음은 저장 재개점을 전진시키지 않는다.
bool UDialogueCheckpointState::VisitLine(FGuid NodeGuid, int32 ReadVersion)
{
	if (NodeGuid.IsValid() == false || ReadVersion < 1 ||
		(Cached.Resume.Kind != EDialogueResumeKind::BlockStart && Cached.Resume.Kind != EDialogueResumeKind::BeforeNode))
	{
		return false;
	}
	CurrentLine = NodeGuid;
	FDialogueLineReadRecord* Existing = PendingReads.FindByPredicate([&](const FDialogueLineReadRecord& Item) { return Item.NodeGuid == NodeGuid; });
	if (Existing)
	{
		Existing->ReadVersion = FMath::Max(Existing->ReadVersion, ReadVersion);
	}
	else
	{
		FDialogueLineReadRecord Line;
		Line.NodeGuid = NodeGuid;
		Line.ReadVersion = ReadVersion;
		PendingReads.Add(Line);
	}
	return true;
}

// 같은 Block 기록 안에 완료와 Line 버전을 유지하며 완료 집합을 따로 만들지 않는다.
void UDialogueCheckpointState::MergePendingReads(bool bCompleted)
{
	if (Cached.Resume.Block.IsValid() == false)
	{
		return;
	}
	FDialogueBlockReadRecord* Record = Cached.FullReadHistory.FindByPredicate([&](const FDialogueBlockReadRecord& Item) { return Item.BlockId == Cached.Resume.Block.BlockId; });
	if (Record == nullptr)
	{
		FDialogueBlockReadRecord NewRecord;
		NewRecord.BlockId = Cached.Resume.Block.BlockId;
		Record = &Cached.FullReadHistory.Add_GetRef(NewRecord);
	}
	Record->bCompleted |= bCompleted;
	for (const FDialogueLineReadRecord& Line : PendingReads)
	{
		FDialogueLineReadRecord* Existing = Record->Lines.FindByPredicate([&](const FDialogueLineReadRecord& Item) { return Item.NodeGuid == Line.NodeGuid; });
		if (Existing)
		{
			Existing->ReadVersion = FMath::Max(Existing->ReadVersion, Line.ReadVersion);
		}
		else
		{
			Record->Lines.Add(Line);
		}
	}
	PendingReads.Reset();
}

// 경계의 유효성을 검사한 뒤에만 캐시를 갱신한다.
bool UDialogueCheckpointState::ApplyBoundary(const FDialogueExecutionBoundary& Boundary, FDialogueCheckpointSnapshot& OutSnapshot)
{
	if (Boundary.Context.BlockRunId != Cached.Resume.BlockRunId || (Boundary.Context.Block == Cached.Resume.Block) == false || Boundary.Context.NodeGuid.IsValid() == false ||
		(Cached.Resume.Kind != EDialogueResumeKind::BlockStart && Cached.Resume.Kind != EDialogueResumeKind::BeforeNode) ||
		(Boundary.bBlockCompleted == false && Boundary.NextNodeGuid.IsValid() == false) ||
		Boundary.bHasEffects != Boundary.Context.ExecutionId.IsValid() || (Boundary.bHasEffects && Boundary.bSaveRequested == false))
	{
		return false;
	}
	if (Boundary.bHasEffects && Cached.RunEvidence.Effects.ContainsByPredicate([&](const FDialogueEffectEvidence& Effect)
		{ return Effect.ExecutionId == Boundary.Context.ExecutionId; }))
	{
		return false;
	}
	FDialogueCheckpointTrigger Trigger;
	Trigger.OriginNodeGuid = Boundary.Context.NodeGuid;
	if (Boundary.bBlockCompleted)
	{
		if (Boundary.ChoiceId.IsValid() || Boundary.bHasEffects || Boundary.NextNodeGuid.IsValid())
		{
			return false;
		}
		MergePendingReads(true);
		Cached.Resume.Kind = EDialogueResumeKind::BlockCompleted;
		Cached.Resume.NodeGuid.Invalidate();
		CurrentLine.Invalidate();
		Trigger.Reason = EDialogueCheckpointReason::BlockEnd;
	}
	else
	{
		if (Boundary.ChoiceId.IsValid() == false && Boundary.bHasEffects == false)
		{
			return false;
		}
		if (Boundary.ChoiceId.IsValid())
		{
			FDialogueChoiceRecord Choice;
			Choice.OccurrenceId = FGuid::NewGuid();
			Choice.Block = Cached.Resume.Block;
			Choice.BlockRunId = Cached.Resume.BlockRunId;
			Choice.NodeGuid = Boundary.Context.NodeGuid;
			Choice.ChoiceId = Boundary.ChoiceId;
			Choice.ExecutionId = Boundary.Context.ExecutionId;
			Cached.PendingChoices.Add(Choice);
		}
		if (Boundary.bHasEffects)
		{
			FDialogueEffectEvidence Effect;
			Effect.ExecutionId = Boundary.Context.ExecutionId;
			Effect.NodeGuid = Boundary.Context.NodeGuid;
			Effect.ChoiceId = Boundary.ChoiceId;
			if (Cached.RunEvidence.PassedLines.Contains(CurrentLine))
			{
				Effect.LineGuid = CurrentLine;
			}
			Cached.RunEvidence.Effects.Add(Effect);
		}
		if (Boundary.bSaveRequested == false)
		{
			OutSnapshot = FDialogueCheckpointSnapshot();
			return true;
		}
		MergePendingReads(false);
		Cached.Resume.Kind = EDialogueResumeKind::BeforeNode;
		Cached.Resume.NodeGuid = Boundary.NextNodeGuid;
		Trigger.Reason = Boundary.ChoiceId.IsValid() ? EDialogueCheckpointReason::Choice : EDialogueCheckpointReason::StateChange;
		Trigger.ChoiceId = Boundary.ChoiceId;
		Trigger.ExecutionId = Boundary.Context.ExecutionId;
		Trigger.bHasEffects = Boundary.bHasEffects;
	}
	OutSnapshot = Capture(Trigger);
	return true;
}

// 값 복사본을 반환하여 후속 변경과 분리한다.
FDialogueCheckpointSnapshot UDialogueCheckpointState::Capture(const FDialogueCheckpointTrigger& Trigger)
{
	FDialogueCheckpointSnapshot Snapshot = Cached;
	Snapshot.CheckpointId = FGuid::NewGuid();
	Snapshot.Trigger = Trigger;
	return Snapshot;
}

// 일반 저장은 마지막 재개 기준과 현재 누적 이력을 함께 복사한다.
FDialogueCheckpointSnapshot UDialogueCheckpointState::CaptureGameSave()
{
	MergePendingReads(false);
	return Capture(FDialogueCheckpointTrigger());
}

// 복원은 전체 검사 후 교체하며 미저장 임시 이력을 혼합하지 않는다.
bool UDialogueCheckpointState::RestoreSnapshot(const FDialogueCheckpointSnapshot& Snapshot, FName& OutError)
{
	FDialogueCheckpointSnapshot Converted;
	if (Snapshot.UpgradeToLatest(Converted, OutError) == false)
	{
		return false;
	}
	Cached = MoveTemp(Converted);
	// 복원 입력은 이미 디스크에 성공한 스냅샷이다. 완료된 저장의 임시 선택을 되살리지 않는다.
	if (Cached.Resume.Kind == EDialogueResumeKind::BlockCompleted)
	{
		Cached.PendingChoices.Reset();
	}
	PendingReads.Reset();
	CurrentLine = Cached.RunEvidence.LastPassedLine;
	return true;
}

// End 성공에 실제 포함된 선택 발생 ID만 정리한다.
void UDialogueCheckpointState::AcknowledgeSavedSnapshot(const FDialogueCheckpointSnapshot& Snapshot)
{
	FName Error;
	if (Snapshot.Validate(Error) == false || Snapshot.Resume.Kind != EDialogueResumeKind::BlockCompleted)
	{
		return;
	}
	TSet<FGuid> Included;
	for (const FDialogueChoiceRecord& Choice : Snapshot.PendingChoices)
	{
		Included.Add(Choice.OccurrenceId);
	}
	Cached.PendingChoices.RemoveAll([&](const FDialogueChoiceRecord& Choice) { return Included.Contains(Choice.OccurrenceId); });
}

// 스토리 초기화와 완전한 새 게임을 구분한다. 디스크 반영은 게임 공통 큐에 제출한다.
void UDialogueCheckpointState::ResetStory(bool bNewGame)
{
	Cached.Resume = FDialogueResumePosition();
	Cached.Trigger = FDialogueCheckpointTrigger();
	Cached.PendingChoices.Reset();
	Cached.RunEvidence = FDialogueRunEvidence();
	PendingReads.Reset();
	CurrentLine.Invalidate();
	if (bNewGame)
	{
		Cached.FullReadHistory.Reset();
		Cached.AppliedCorrections.Reset();
	}
}

// 열람 갱신으로 버전이 높아진 Line은 다시 미열람이다.
bool UDialogueCheckpointState::HasReadLine(FGuid BlockId, FGuid NodeGuid, int32 ReadVersion) const
{
	const FDialogueBlockReadRecord* Block = Cached.FullReadHistory.FindByPredicate([&](const FDialogueBlockReadRecord& Item) { return Item.BlockId == BlockId; });
	return ReadVersion > 0 && Block && Block->Lines.ContainsByPredicate([&](const FDialogueLineReadRecord& Item) { return Item.NodeGuid == NodeGuid && Item.ReadVersion >= ReadVersion; });
}

// 표시된 현재 Line을 벗어날 때만 통과 기록을 추가한다.
bool UDialogueCheckpointState::PassLine(FGuid NodeGuid)
{
	if (NodeGuid.IsValid() == false || NodeGuid != CurrentLine ||
		(Cached.Resume.Kind != EDialogueResumeKind::BlockStart && Cached.Resume.Kind != EDialogueResumeKind::BeforeNode))
	{
		return false;
	}
	Cached.RunEvidence.PassedLines.AddUnique(NodeGuid);
	Cached.RunEvidence.LastPassedLine = NodeGuid;
	return true;
}
