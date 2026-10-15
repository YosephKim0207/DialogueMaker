#include "DialogueBlockRunner.h"
#include "DialogueLineNodeInfo.h"
#include "DialogueChoiceNodeInfo.h"
#include "DialogueConditionLibrary.h"
#include "DialogueBranchNodeInfoBase.h"
#include "DialogueStateChangeNodeInfo.h"
#include "DialogueEffectContract.h"
#include "DialogueEndNodeInfo.h"

// 명시적으로 전달된 Block을 검사하여 Start에서 실행한다.
bool UDialogueBlockRunner::StartBlock(UDialogueGraph* Block, UObject* NewStateProvider, UObject* NewEffectProvider, UDialogueDiagnosticRouter* NewRouter)
{
	return PrepareBlock(Block, NewStateProvider, NewEffectProvider, NewRouter) && RunPreparedBlock();
}

// 준비 단계는 게임 상태 변경/대사 표시 없이 그래프와 재개 노드를 검사한다.
bool UDialogueBlockRunner::PrepareAtNode(UDialogueGraph* Block, UObject* NewStateProvider, UObject* NewEffectProvider, UDialogueDiagnosticRouter* NewRouter, FGuid EntryNode)
{
	if (CanStartBlock() == false)
	{
		return false;
	}
	TGuardValue<bool> Guard(bProcessing, true);
	bPreparedCompleted = false;
	BlockRunId = FGuid::NewGuid();
	ActiveBlock = Block;
	StateProvider = NewStateProvider;
	EffectProvider = NewEffectProvider;
	Router = NewRouter;
	Presentation = FDialoguePresentation();
	Nodes.Reset();
	if (IsValid(Block) == false || Block->GetBlockReference().IsValid() == false || IsValid(Block->Graph) == false)
	{
		return Fail(TEXT("InvalidBlock"));
	}
	TArray<FString> Issues;
	if (Block->Graph->ValidateIdentifiers(Issues) == false)
	{
		return Fail(TEXT("InvalidGraphIdentity"));
	}
	UDialogueRuntimeNode* Start = nullptr;
	for (UDialogueRuntimeNode* Node : Block->Graph->Nodes)
	{
		if (IsValid(Node) == false || Node->NodeGuid.IsValid() == false || Nodes.Contains(Node->NodeGuid))
		{
			return Fail(TEXT("InvalidNodeIdentity"));
		}
		// 수동 재작성 중 구형/알 수 없는 노드가 섞였으면 어떤 효과도 실행하기 전에 거부한다.
		if (Node->DialogueNodeType != EDialogueType::StartNode && Node->DialogueNodeType != EDialogueType::LineNode &&
			Node->DialogueNodeType != EDialogueType::ChoiceNode && Node->DialogueNodeType != EDialogueType::BranchNode &&
			Node->DialogueNodeType != EDialogueType::StateChangeNode && Node->DialogueNodeType != EDialogueType::EndNode)
		{
			Presentation.NodeGuid = Node->NodeGuid;
			return Fail(TEXT("UnsupportedNodeType"));
		}
		Nodes.Add(Node->NodeGuid, Node);
		if (Node->DialogueNodeType == EDialogueType::StartNode)
		{
			if (Start != nullptr)
			{
				return Fail(TEXT("MultipleStartNodes"));
			}
			Start = Node;
		}
	}
	if (Start == nullptr)
	{
		return Fail(TEXT("MissingStartNode"));
	}
	if (EntryNode.IsValid() && Nodes.Contains(EntryNode) == false)
	{
		return Fail(TEXT("ResumeNodeNotFound"));
	}
	PreparedEntry = EntryNode.IsValid() ? EntryNode : Start->NodeGuid;
	Presentation.State = EDialogueRunState::Prepared;
	return true;
}

// 목적지 객체와 저장된 목적지 ID가 일치하는 연결만 반환한다.
UDialogueRuntimeNode* UDialogueBlockRunner::ResolveOutput(const UDialogueRuntimeNode* Node, int32 Index) const
{
	if (Node == nullptr || Node->OutputPins.IsValidIndex(Index) == false)
	{
		return nullptr;
	}
	const UDialogueRuntimePin* Pin = Node->OutputPins[Index];
	if (IsValid(Pin) == false || Pin->OwnerNodeGuid != Node->NodeGuid || Pin->Connections.Num() != 1)
	{
		return nullptr;
	}
	UDialogueRuntimeNode* Target = Nodes.FindRef(Pin->LinkedToNodeGuid);
	return IsValid(Target) && IsValid(Target->InputPin) && Target->InputPin == Pin->Connections[0] &&
		Target->InputPin->OwnerNodeGuid == Target->NodeGuid ? Target : nullptr;
}

// 한 번의 입력 요청에서만 자동 구간을 해석한다.
bool UDialogueBlockRunner::Pump(UDialogueRuntimeNode* Node)
{
	Presentation.State = EDialogueRunState::Running;
	Presentation.Choices.Reset();
	Presentation.ChoiceNodeGuid.Invalidate();
	TSet<FGuid> VisitedBranches;
	for (int32 Step = 0; Step < 4096; ++Step)
	{
		if (Node == nullptr)
		{
			return Fail(TEXT("InvalidConnection"));
		}
		Presentation.NodeGuid = Node->NodeGuid;
		if (Node->DialogueNodeType == EDialogueType::StartNode)
		{
			if (Node->InputPin != nullptr || Node->NodeInfo != nullptr || Node->OutputPins.Num() != 1)
			{
				return Fail(TEXT("InvalidPinShape"));
			}
			Node = ResolveOutput(Node, 0);
			continue;
		}
		if (Node->DialogueNodeType == EDialogueType::LineNode)
		{
			UDialogueLineNodeInfo* Info = Cast<UDialogueLineNodeInfo>(Node->NodeInfo);
			if (Info == nullptr || Info->GetReadVersion() < 1 || Info->GetDialogueChoices().IsEmpty() == false || Node->OutputPins.Num() != 1)
			{
				return Fail(TEXT("InvalidLine"));
			}
			if (LineVisited.IsBound() && LineVisited.Execute(Node->NodeGuid, Info->GetReadVersion()) == false)
			{
				return Fail(TEXT("LineVisitRejected"));
			}
			Presentation.Line = DuplicateObject<UDialogueLineNodeInfo>(Info, this);
			Presentation.LineNodeGuid = Node->NodeGuid;
			Presentation.State = EDialogueRunState::WaitingLine;
			Presentation.DirectNextType = GetDirectNextType();
			if (Presentation.DirectNextType == EDialogueType::ChoiceNode)
			{
				UDialogueRuntimeNode* Choice = ResolveOutput(Node, 0);
				if (GetChoiceViews(Choice, Presentation.Choices) == false)
				{
					return Fail(TEXT("InvalidChoice"));
				}
				Presentation.ChoiceNodeGuid = Choice->NodeGuid;
			}
			Publish();
			return true;
		}
		if (Node->DialogueNodeType == EDialogueType::StateChangeNode)
		{
			const UDialogueStateChangeNodeInfo* Info = Cast<UDialogueStateChangeNodeInfo>(Node->NodeInfo);
			if (Info == nullptr || Info->Effects.IsEmpty() || Node->OutputPins.Num() != 1)
			{
				return Fail(TEXT("InvalidStateChange"));
			}
			UDialogueRuntimeNode* Next = ResolveOutput(Node, 0);
			if (ApplyEffects(Node, Next, Info->Effects, FGuid(), true) == false)
			{
				return false;
			}
			Node = Next;
			continue;
		}
		if (Node->DialogueNodeType == EDialogueType::BranchNode)
		{
			const UDialogueBranchNodeInfoBase* Info = Cast<UDialogueBranchNodeInfoBase>(Node->NodeInfo);
			if (Info == nullptr || Node->OutputPins.Num() != 2 || Node->OutputPins[0]->PinName != TEXT("True") || Node->OutputPins[1]->PinName != TEXT("False"))
			{
				return Fail(TEXT("InvalidBranch"));
			}
			if (VisitedBranches.Contains(Node->NodeGuid))
			{
				// 제작사는 Router에 자동 순환 오류 전송을 연결한다. 재평가 전에 중단한다.
				return Fail(TEXT("AutomaticBranchCycle"));
			}
			VisitedBranches.Add(Node->NodeGuid);
			FDialogueDiagnosticContext Context;
			Context.Block = ActiveBlock->GetBlockReference();
			Context.NodeGuid = Node->NodeGuid;
			// 빈 조건/조회 오류의 True 대체와 로그는 공통 평가기에서 처리한다.
			const bool bPassed = UDialogueConditionLibrary::EvaluateConditions(Info->Conditions, StateProvider, true, Context, Router).bPassed;
			Node = ResolveOutput(Node, bPassed ? 0 : 1);
			continue;
		}
		if (Node->DialogueNodeType == EDialogueType::ChoiceNode)
		{
			if (GetChoiceViews(Node, Presentation.Choices) == false)
			{
				return Fail(TEXT("InvalidChoice"));
			}
			if (Presentation.Choices.IsEmpty() == false)
			{
				Presentation.State = EDialogueRunState::WaitingChoice;
				Presentation.ChoiceNodeGuid = Node->NodeGuid;
				Presentation.DirectNextType = EDialogueType::Unknown;
				Publish();
				return true;
			}
			// 제작사는 이 방어 경로의 진단 Router에 오류 전송을 연결한다.
			Report(TEXT("NoAvailableChoice"), TEXT("CompleteNormally"));
			return Complete(false);
		}
		if (Node->DialogueNodeType == EDialogueType::EndNode)
		{
			if (Cast<UDialogueEndNodeInfo>(Node->NodeInfo) == nullptr || Node->OutputPins.IsEmpty() == false)
			{
				return Fail(TEXT("InvalidPinShape"));
			}
			return Complete(CastChecked<UDialogueEndNodeInfo>(Node->NodeInfo)->bSaveOnEnd);
		}
		return Fail(TEXT("UnsupportedNodeType"));
	}
	// 게임 제작자는 과도한 자동 작업 진단을 수집할 수 있다. 특정 StateChange 반복 정책은 게임 책임이다.
	return Fail(TEXT("AutomaticWorkBudgetExceeded"));
}

// 발화 완료 입력 하나는 직접 다음 노드 진입 한 번만 시작한다.
bool UDialogueBlockRunner::AdvanceLine()
{
	if (bProcessing || Presentation.State != EDialogueRunState::WaitingLine)
	{
		return false;
	}
	TGuardValue<bool> Guard(bProcessing, true);
	UDialogueRuntimeNode* Next = ResolveOutput(Nodes.FindRef(Presentation.NodeGuid), 0);
	if (Next == nullptr) return Fail(TEXT("InvalidConnection"));
	if (LinePassed.IsBound() && LinePassed.Execute(Presentation.LineNodeGuid) == false) return Fail(TEXT("LinePassRejected"));
	return Pump(Next);
}

// 미리보기는 다음 노드의 타입만 읽는다.
EDialogueType UDialogueBlockRunner::GetDirectNextType() const
{
	if (Presentation.State != EDialogueRunState::WaitingLine)
	{
		return EDialogueType::Unknown;
	}
	const UDialogueRuntimeNode* Next = ResolveOutput(Nodes.FindRef(Presentation.NodeGuid), 0);
	return Next ? Next->DialogueNodeType : EDialogueType::Unknown;
}

// 실행 오류를 정상 완료나 게임 저장으로 치환하지 않는다.
bool UDialogueBlockRunner::Fail(FName Code)
{
	Presentation.State = EDialogueRunState::Failed;
	Presentation.ErrorCode = Code;
	Presentation.Choices.Reset();
	Presentation.ChoiceNodeGuid.Invalidate();
	Presentation.DirectNextType = EDialogueType::Unknown;
	Report(Code, TEXT("StopAndKeepLastSuccessfulSave"));
	Publish();
	return false;
}

// 개발자는 Router에 제작사 로그 전송을 연결한다. 함수 반환값으로 안내/복구 흐름을 처리한다.
void UDialogueBlockRunner::Report(FName Code, const FString& Handling)
{
	FDialogueDiagnostic Diagnostic;
	Diagnostic.Code = Code;
	Diagnostic.Message = Code.ToString();
	Diagnostic.Handling = Handling;
	Diagnostic.Context.Block = ActiveBlock ? ActiveBlock->GetBlockReference() : FDialogueBlockReference();
	Diagnostic.Context.NodeGuid = Presentation.NodeGuid;
	UDialogueDiagnosticRouter::Dispatch(MoveTemp(Diagnostic), Router);
}

// C++/Blueprint UI가 동일한 표시 데이터를 받는다.
void UDialogueBlockRunner::Publish(bool bChoicesOnly)
{
	FDialoguePresentation Event = Presentation;
	Event.bChoicesOnlyUpdate = bChoicesOnly;
	OnPresentationChangedNative.Broadcast(Event);
	OnPresentationChanged.Broadcast(Event);
}

// 일반 선택지가 하나라도 통과하면 기본 선택지는 숨긴다.
bool UDialogueBlockRunner::GetChoiceViews(UDialogueRuntimeNode* Node, TArray<FDialogueChoiceView>& OutViews)
{
	OutViews.Reset();
	const UDialogueChoiceNodeInfo* Info = Node ? Cast<UDialogueChoiceNodeInfo>(Node->NodeInfo) : nullptr;
	if (Info == nullptr || Info->HasValidOptions() == false || Node->OutputPins.Num() != Info->Options.Num())
	{
		return false;
	}
	const FDialogueOption* Default = nullptr;
	FDialogueDiagnosticContext Context;
	Context.Block = ActiveBlock->GetBlockReference();
	Context.NodeGuid = Node->NodeGuid;
	for (int32 Index = 0; Index < Info->Options.Num(); ++Index)
	{
		const FDialogueOption& Option = Info->Options[Index];
		if (Node->OutputPins[Index] == nullptr || Node->OutputPins[Index]->ChoiceId != Option.ChoiceId)
		{
			return false;
		}
		if (Option.bDefault)
		{
			Default = &Option;
		}
		else if (UDialogueConditionLibrary::EvaluateConditions(Option.Conditions, StateProvider, false, Context, Router).bPassed)
		{
			FDialogueChoiceView View;
			View.ChoiceId = Option.ChoiceId;
			View.Text = Option.Text;
			OutViews.Add(View);
		}
	}
	if (OutViews.IsEmpty() && Default)
	{
		FDialogueChoiceView View;
		View.ChoiceId = Default->ChoiceId;
		View.Text = Default->Text;
		OutViews.Add(View);
	}
	return true;
}

// 사용자가 실제 선택할 때 다시 자격을 평가하고 대응 출력 한 곳으로만 진행한다.
bool UDialogueBlockRunner::SelectChoice(FGuid ChoiceId)
{
	if (bProcessing || (Presentation.State != EDialogueRunState::WaitingLine && Presentation.State != EDialogueRunState::WaitingChoice))
	{
		return false;
	}
	TGuardValue<bool> Guard(bProcessing, true);
	UDialogueRuntimeNode* Node = Nodes.FindRef(Presentation.ChoiceNodeGuid);
	TArray<FDialogueChoiceView> Views;
	if (Node == nullptr || GetChoiceViews(Node, Views) == false)
	{
		return false;
	}
	if (Presentation.Choices.ContainsByPredicate([ChoiceId](const FDialogueChoiceView& View) { return View.ChoiceId == ChoiceId; }) == false ||
		Views.ContainsByPredicate([ChoiceId](const FDialogueChoiceView& View) { return View.ChoiceId == ChoiceId; }) == false)
	{
		Presentation.Choices = MoveTemp(Views);
		if (Presentation.State == EDialogueRunState::WaitingChoice && Presentation.Choices.IsEmpty())
		{
			Report(TEXT("NoAvailableChoice"), TEXT("CompleteNormally"));
			Complete(false);
			return false;
		}
		Publish(true);
		return false;
	}
	const UDialogueChoiceNodeInfo* Info = CastChecked<UDialogueChoiceNodeInfo>(Node->NodeInfo);
	const int32 Index = Info->Options.IndexOfByPredicate([ChoiceId](const FDialogueOption& Option) { return Option.ChoiceId == ChoiceId; });
	UDialogueRuntimeNode* Next = ResolveOutput(Node, Index);
	if (Next == nullptr) return Fail(TEXT("InvalidConnection"));
	if (Presentation.State == EDialogueRunState::WaitingLine && LinePassed.IsBound() && LinePassed.Execute(Presentation.LineNodeGuid) == false)
	{
		return Fail(TEXT("LinePassRejected"));
	}
	if (ApplyEffects(Node, Next, Info->Options[Index].Effects, ChoiceId, Info->bSaveOnSelection) == false)
	{
		return false;
	}
	return Pump(Next);
}

// 실제 진입만 효과를 적용한다. 다음 연결이 끊겼으면 지급 전에 실패한다.
bool UDialogueBlockRunner::ApplyEffects(UDialogueRuntimeNode* Origin, UDialogueRuntimeNode* Next, const TArray<FDialogueEffect>& Effects, FGuid ChoiceId, bool bSaveRequested)
{
	Presentation.State = EDialogueRunState::Running;
	Presentation.NodeGuid = Origin->NodeGuid;
	Presentation.DirectNextType = EDialogueType::Unknown;
	Presentation.Choices.Reset();
	Presentation.ChoiceNodeGuid.Invalidate();
	if (Next == nullptr)
	{
		return Fail(TEXT("InvalidConnection"));
	}
	FDialogueExecutionBoundary Boundary;
	Boundary.Context.Block = ActiveBlock->GetBlockReference();
	Boundary.Context.BlockRunId = BlockRunId;
	Boundary.Context.NodeGuid = Origin->NodeGuid;
	Boundary.ChoiceId = ChoiceId;
	Boundary.NextNodeGuid = Next->NodeGuid;
	Boundary.bHasEffects = Effects.IsEmpty() == false;
	Boundary.bSaveRequested = bSaveRequested || Boundary.bHasEffects;
	if (Boundary.bSaveRequested && CheckpointCapture.IsBound() && (CheckpointReady.IsBound() == false || CheckpointReady.Execute() == false))
	{
		return Fail(TEXT("CheckpointUnavailable"));
	}
	if (Boundary.bHasEffects)
	{
		Boundary.Context.ExecutionId = FGuid::NewGuid();
		FDialogueEffectBatch Batch;
		Batch.Context = Boundary.Context;
		Batch.Effects = Effects;
		Batch.ChoiceId = ChoiceId;
		const FDialogueEffectResult Result = UDialogueEffectLibrary::ApplyEffectBatch(Batch, EffectProvider, Router);
		if (Result.IsSuccess() == false)
		{
			// 제작사는 제공자 오류와 실행 ID가 담긴 Router 진단을 외부 로그로 전송한다.
			return Fail(Result.ErrorCode);
		}
	}
	// 후속 노드가 상태를 변경하기 전에 게임이 불변 스냅샷을 확보해야 한다.
	if (CheckpointCapture.IsBound() && CheckpointCapture.Execute(Boundary) == false)
	{
		return Fail(TEXT("CheckpointCaptureFailed"));
	}
	OnExecutionBoundaryNative.Broadcast(Boundary);
	OnExecutionBoundary.Broadcast(Boundary);
	return true;
}

// 완료는 저장 성공과 다르다. 게임은 완료 이력/종료 상태를 캐시에 반영한 뒤 선택적으로 저장한다.
bool UDialogueBlockRunner::Complete(bool bSaveRequested)
{
	if (bSaveRequested && CheckpointCapture.IsBound() && (CheckpointReady.IsBound() == false || CheckpointReady.Execute() == false))
	{
		return Fail(TEXT("CheckpointUnavailable"));
	}
	Presentation.State = EDialogueRunState::Completed;
	Presentation.Line = nullptr;
	Presentation.LineNodeGuid.Invalidate();
	Presentation.DirectNextType = EDialogueType::Unknown;
	Presentation.Choices.Reset();
	Presentation.ChoiceNodeGuid.Invalidate();
	FDialogueExecutionBoundary Boundary;
	Boundary.Context.Block = ActiveBlock->GetBlockReference();
	Boundary.Context.BlockRunId = BlockRunId;
	Boundary.Context.NodeGuid = Presentation.NodeGuid;
	Boundary.bBlockCompleted = true;
	Boundary.bSaveRequested = bSaveRequested;
	if (CheckpointCapture.IsBound() && CheckpointCapture.Execute(Boundary) == false)
	{
		return Fail(TEXT("CheckpointCaptureFailed"));
	}
	OnExecutionBoundaryNative.Broadcast(Boundary);
	OnExecutionBoundary.Broadcast(Boundary);
	Publish();
	return true;
}

// 실행 중 콜백에서 새 시작이 현재 실행 문맥을 덮어쓰지 않게 한다.
bool UDialogueBlockRunner::CanStartBlock() const
{
	return bProcessing == false && (Presentation.State == EDialogueRunState::Idle || Presentation.State == EDialogueRunState::Completed || Presentation.State == EDialogueRunState::Failed);
}

// 준비된 노드부터 실행한다. 준비 후 캡처와 게임 보정은 호출자가 완료해야 한다.
bool UDialogueBlockRunner::RunPreparedBlock()
{
	if (bProcessing || Presentation.State != EDialogueRunState::Prepared)
	{
		return false;
	}
	TGuardValue<bool> Guard(bProcessing, true);
	if (bPreparedCompleted)
	{
		Presentation.State = EDialogueRunState::Completed;
		Publish();
		return true;
	}
	return Pump(Nodes.FindRef(PreparedEntry));
}

// 활성 저장은 후보 조건을 재평가하지 않고 같은 ID와 콘텐츠 버전으로 복원한다.
bool UDialogueBlockRunner::PrepareResume(UDialogueGraph* Block, const FDialogueResumePosition& Position, UObject* NewStateProvider, UObject* NewEffectProvider, UDialogueDiagnosticRouter* NewRouter)
{
	if (Position.IsValid() == false || IsValid(Block) == false || (Position.Block == Block->GetBlockReference()) == false ||
		Position.Kind == EDialogueResumeKind::None)
	{
		return false;
	}
	if (PrepareAtNode(Block, NewStateProvider, NewEffectProvider, NewRouter, Position.NodeGuid) == false)
	{
		return false;
	}
	if (Position.Kind == EDialogueResumeKind::BeforeNode && Nodes.FindRef(Position.NodeGuid)->DialogueNodeType == EDialogueType::StartNode)
	{
		return Fail(TEXT("InvalidResumeNode"));
	}
	BlockRunId = Position.BlockRunId;
	bPreparedCompleted = Position.Kind == EDialogueResumeKind::BlockCompleted;
	return true;
}

// 명시적 세션 종료는 그래프 실행만 취소하고 저장 큐와 누적 이력을 건드리지 않는다.
bool UDialogueBlockRunner::CancelBlock()
{
	if (bProcessing)
	{
		return false;
	}
	TGuardValue<bool> Guard(bProcessing, true);
	ActiveBlock = nullptr;
	Nodes.Reset();
	Presentation = FDialoguePresentation();
	PreparedEntry.Invalidate();
	Publish();
	return true;
}

// 검증된 위치 준비 이후에만 실행한다.
bool UDialogueBlockRunner::ResumeBlock(UDialogueGraph* Block, const FDialogueResumePosition& Position, UObject* NewStateProvider, UObject* NewEffectProvider, UDialogueDiagnosticRouter* NewRouter)
{
	return PrepareResume(Block, Position, NewStateProvider, NewEffectProvider, NewRouter) && RunPreparedBlock();
}

// 신규 실행 준비의 진입점은 Start로 고정한다.
bool UDialogueBlockRunner::PrepareBlock(UDialogueGraph* Block, UObject* NewStateProvider, UObject* NewEffectProvider, UDialogueDiagnosticRouter* NewRouter)
{
	return PrepareAtNode(Block, NewStateProvider, NewEffectProvider, NewRouter, FGuid());
}

// 조건 갱신은 동일 Line의 표시 시간을 유지하며 자동 경로를 미리 실행하지 않는다.
bool UDialogueBlockRunner::RefreshChoices()
{
	if (bProcessing || (Presentation.State != EDialogueRunState::WaitingLine && Presentation.State != EDialogueRunState::WaitingChoice))
	{
		return false;
	}
	TGuardValue<bool> Guard(bProcessing, true);
	if (Presentation.ChoiceNodeGuid.IsValid() == false)
	{
		return true;
	}
	TArray<FDialogueChoiceView> Choices;
	if (GetChoiceViews(Nodes.FindRef(Presentation.ChoiceNodeGuid), Choices) == false)
	{
		return false;
	}
	Presentation.Choices = MoveTemp(Choices);
	if (Presentation.State == EDialogueRunState::WaitingChoice && Presentation.Choices.IsEmpty())
	{
		Report(TEXT("NoAvailableChoice"), TEXT("CompleteNormally"));
		return Complete(false);
	}
	Publish(true);
	return true;
}
