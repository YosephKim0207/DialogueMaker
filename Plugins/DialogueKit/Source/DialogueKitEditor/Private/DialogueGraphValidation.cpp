#include "DialogueGraphValidation.h"
#include "DialogueEdGraphNodeBase.h"
#include "DialogueChoiceNodeInfo.h"
#include "DialogueBranchNodeInfoBase.h"
#include "DialogueRerouteNode.h"
#include "DialogueGraph.h"
#include "DialogueGraphCodec.h"
#include "EdGraph/EdGraph.h"

// 저장 차단 항목 하나라도 있으면 계속 저장할 수 없다.
bool FDialogueGraphValidation::CanSave(const TArray<FDialogueGraphIssue>& Issues)
{
	return Issues.ContainsByPredicate([](const FDialogueGraphIssue& Issue) { return Issue.bBlocksSave; }) == false;
}

// 실행하지 않고 구조와 조건 설정만 검사한다. 게임 상태 조회·이력·커서는 변경하지 않는다.
TArray<FDialogueGraphIssue> FDialogueGraphValidation::Inspect(UEdGraph* Graph)
{
	TArray<FDialogueGraphIssue> Issues;
	auto Add = [&Issues](UEdGraphNode* Node, FName Code, const TCHAR* Message, bool bBlock = false)
	{
		Issues.Add({Node, Code, FText::FromString(Message), bBlock});
	};
	if (Graph == nullptr)
	{
		Add(nullptr, TEXT("MissingGraph"), TEXT("작업 그래프가 없습니다."), true);
		return Issues;
	}
	auto CheckConditions = [&Add](UEdGraphNode* Node, const FDialogueConditionSet& Set)
	{
		bool bInvalid = Set.Combine != EDialogueConditionCombine::All && Set.Combine != EDialogueConditionCombine::Any;
		for (const FDialogueCondition& Condition : Set.Conditions)
		{
			bInvalid |= Condition.Key.IsValid() == false || uint8(Condition.Comparison) > uint8(EDialogueComparison::GreaterOrEqual);
			bInvalid |= Condition.Key.Kind == EDialogueStateKind::Tag && Condition.Comparison != EDialogueComparison::Equal && Condition.Comparison != EDialogueComparison::NotEqual;
			bInvalid |= Condition.Key.Kind == EDialogueStateKind::Value && FMath::IsFinite(Condition.ValueOperand) == false;
		}
		if (bInvalid) Add(Node, TEXT("InvalidCondition"), TEXT("불완전한 조건입니다. 실행 시 오류를 기록하고 True로 대체합니다."));
	};
	int32 StartCount = 0;
	for (UEdGraphNode* Node : Graph->Nodes)
	{
		auto* Typed = Cast<UDialogueEdGraphNodeBase>(Node);
		if (Typed == nullptr) continue;
		StartCount += Typed->GetDialogueNodeType() == EDialogueType::StartNode ? 1 : 0;
		for (UEdGraphPin* Pin : Node->Pins)
		{
			if (Pin && Pin->Direction == EGPD_Output && Pin->LinkedTo.IsEmpty())
			{
				Add(Node, TEXT("MissingConnection"), TEXT("연결되지 않은 출력 핀이 있습니다."));
			}
		}
		if (const auto* Branch = Cast<UDialogueBranchNodeInfoBase>(Typed->GetNodeInfo()))
		{
			if (Branch->Conditions.Conditions.IsEmpty()) Add(Node, TEXT("EmptyBranch"), TEXT("빈 Branch 조건: 오류를 기록하고 True로 진행합니다."));
			CheckConditions(Node, Branch->Conditions);
		}
		if (const auto* Choice = Cast<UDialogueChoiceNodeInfo>(Typed->GetNodeInfo()))
		{
			if (Choice->Options.IsEmpty())
			{
				Add(Node, TEXT("EmptyChoice"), TEXT("기본/일반 선택지가 모두 없습니다. 저장할 수 없습니다."), true);
				continue;
			}
			bool bDefault = false;
			bool bUnconditional = false;
			int32 NormalCount = 0;
			for (const FDialogueOption& Option : Choice->Options)
			{
				if (Option.bDefault) bDefault = true;
				else
				{
					++NormalCount;
					bUnconditional |= Option.Conditions.Conditions.IsEmpty();
					CheckConditions(Node, Option.Conditions);
				}
			}
			if (NormalCount == 0) Add(Node, TEXT("DefaultOnly"), TEXT("기본 선택지만 있고 일반 선택지가 없습니다."));
			else if (bDefault == false && bUnconditional == false) Add(Node, TEXT("NoFallback"), TEXT("모든 일반 선택지에 조건이 있고 기본 선택지가 없습니다."));
		}
	}
	if (StartCount != 1) Add(nullptr, TEXT("InvalidStartCount"), TEXT("Start 노드는 정확히 하나여야 합니다."), true);
	// 플레이어 입력을 기다리지 않는 노드들 사이의 순환만 확인한다.
	TMap<UEdGraphNode*, uint8> Colors;
	for (UEdGraphNode* Root : Graph->Nodes)
	{
		TArray<TPair<UEdGraphNode*, bool>> Stack;
		Stack.Emplace(Root, false);
		while (Stack.IsEmpty() == false)
		{
			const auto Entry = Stack.Pop(false);
			UEdGraphNode* Node = Entry.Key;
			auto* Typed = Cast<UDialogueEdGraphNodeBase>(Node);
			if (Typed == nullptr) continue;
			const EDialogueType Type = Typed->GetDialogueNodeType();
			if (Type != EDialogueType::BranchNode && Type != EDialogueType::StateChangeNode && Cast<UDialogueRerouteNode>(Node) == nullptr) continue;
			if (Entry.Value) { Colors.Add(Node, 2); continue; }
			if (Colors.FindRef(Node) == 1) { Add(Node, TEXT("AutomaticCycle"), TEXT("플레이어 입력 없는 자동 순환 가능성이 있습니다.")); continue; }
			if (Colors.FindRef(Node) == 2) continue;
			Colors.Add(Node, 1);
			Stack.Emplace(Node, true);
			for (UEdGraphPin* Pin : Node->Pins)
			{
				if (Pin && Pin->Direction == EGPD_Output)
				{
					for (UEdGraphPin* Target : Pin->LinkedTo)
					{
						if (Target) Stack.Emplace(Target->GetOwningNode(), false);
					}
				}
			}
		}
	}
	FString Error;
	auto* Scratch = NewObject<UDialogueGraph>();
	if (FDialogueGraphCodec::ExportGraph(Graph, Scratch, Error) == false)
	{
		Add(nullptr, TEXT("InvalidStructure"), TEXT("노드·핀·식별자 또는 경유점 구조가 유효하지 않아 저장 변환할 수 없습니다."), true);
	}
	return Issues;
}
