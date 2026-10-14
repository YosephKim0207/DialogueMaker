#include "DialogueBlockRunner.h"
#include "DialogueLineNodeInfo.h"
#include "DialogueChoiceNodeInfo.h"
#include "DialogueConditionLibrary.h"
#include "DialogueEndNodeInfo.h"

// 명시적으로 전달된 Block을 검사하여 Start에서 실행한다.
bool UDialogueBlockRunner::StartBlock(UDialogueGraph* Block, UObject* NewStateProvider, UObject* NewEffectProvider, UDialogueDiagnosticRouter* NewRouter)
{
	if (bProcessing || Presentation.State == EDialogueRunState::WaitingLine || Presentation.State == EDialogueRunState::WaitingChoice || Presentation.State == EDialogueRunState::Running)
	{
		return false;
	}
	TGuardValue<bool> Guard(bProcessing, true);
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
	return Start ? Pump(Start) : Fail(TEXT("MissingStartNode"));
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
			Presentation.Line = DuplicateObject<UDialogueLineNodeInfo>(Info, this);
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
			Presentation.State = EDialogueRunState::Completed;
			Presentation.Line = nullptr;
			Presentation.DirectNextType = EDialogueType::Unknown;
			Publish();
			return true;
		}
		if (Node->DialogueNodeType == EDialogueType::EndNode)
		{
			if (Cast<UDialogueEndNodeInfo>(Node->NodeInfo) == nullptr || Node->OutputPins.IsEmpty() == false)
			{
				return Fail(TEXT("InvalidPinShape"));
			}
			Presentation.State = EDialogueRunState::Completed;
			Presentation.Line = nullptr;
			Presentation.DirectNextType = EDialogueType::Unknown;
			Publish();
			return true;
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
	return Pump(ResolveOutput(Nodes.FindRef(Presentation.NodeGuid), 0));
}

// 미리보기는 다음 노드의 타입만 읽는다.
EDialogueType UDialogueBlockRunner::GetDirectNextType() const
{
	const UDialogueRuntimeNode* Next = ResolveOutput(Nodes.FindRef(Presentation.NodeGuid), 0);
	return Next ? Next->DialogueNodeType : EDialogueType::Unknown;
}

// 실행 오류를 정상 완료나 게임 저장으로 치환하지 않는다.
bool UDialogueBlockRunner::Fail(FName Code)
{
	Presentation.State = EDialogueRunState::Failed;
	Presentation.ErrorCode = Code;
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
void UDialogueBlockRunner::Publish()
{
	OnPresentationChangedNative.Broadcast(Presentation);
	OnPresentationChanged.Broadcast(Presentation);
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
			Presentation.State = EDialogueRunState::Completed;
			Presentation.Line = nullptr;
			Presentation.ChoiceNodeGuid.Invalidate();
		}
		Publish();
		return false;
	}
	const UDialogueChoiceNodeInfo* Info = CastChecked<UDialogueChoiceNodeInfo>(Node->NodeInfo);
	const int32 Index = Info->Options.IndexOfByPredicate([ChoiceId](const FDialogueOption& Option) { return Option.ChoiceId == ChoiceId; });
	return Pump(ResolveOutput(Node, Index));
}
