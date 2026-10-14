#include "DialogueBlockRunner.h"
#include "DialogueLineNodeInfo.h"
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
