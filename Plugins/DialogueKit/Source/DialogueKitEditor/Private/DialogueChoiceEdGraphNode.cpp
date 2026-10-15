#include "DialogueChoiceEdGraphNode.h"
#include "DialogueChoiceNodeInfo.h"
#include "EdGraph/EdGraph.h"

// Choice는 화자 발화와 독립된 선택 목록이다.
FText UDialogueChoiceEdGraphNode::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return FText::FromString(TEXT("Choice"));
}

// 생성 시 기본 일반 선택지 하나를 제공한다.
void UDialogueChoiceEdGraphNode::InitNodeInfo(UObject* Outer)
{
	UDialogueChoiceNodeInfo* Info = NewObject<UDialogueChoiceNodeInfo>(Outer, NAME_None, RF_Transactional);
	FDialogueOption Option;
	Option.Text = FText::FromString(TEXT("Continue"));
	Info->Options.Add(Option);
	Info->EnsureOptionIds();
	NodeInfo = Info;
}

// 각 선택지의 ID에 대응하는 출력 핀을 생성한다.
void UDialogueChoiceEdGraphNode::CreateDefaultOutputPin()
{
	OnPropertiesChanged();
}

// 항목 재정렬은 기존 핀/연결을 유지하고 삭제된 항목의 연결만 제거한다.
void UDialogueChoiceEdGraphNode::OnPropertiesChanged()
{
	UDialogueChoiceNodeInfo* Info = Cast<UDialogueChoiceNodeInfo>(NodeInfo);
	if (Info == nullptr)
	{
		return;
	}
	Modify();
	Info->Modify();
	Info->EnsureOptionIds();
	TArray<UEdGraphPin*> OldOutputs;
	TArray<UEdGraphPin*> Ordered;
	for (UEdGraphPin* Pin : Pins)
	{
		if (Pin->Direction == EGPD_Output)
		{
			OldOutputs.Add(Pin);
		}
		else
		{
			Ordered.Add(Pin);
		}
	}
	for (const FDialogueOption& Option : Info->Options)
	{
		UEdGraphPin* Matched = nullptr;
		for (UEdGraphPin* Pin : OldOutputs)
		{
			if (Pin->PersistentGuid == Option.ChoiceId)
			{
				Matched = Pin;
				break;
			}
		}
		if (Matched == nullptr)
		{
			Matched = CreateCustomPin(EGPD_Output, NAME_None);
		}
		Matched->PersistentGuid = Option.ChoiceId;
		Matched->PinName = FName(Option.Text.ToString());
		Ordered.Add(Matched);
		OldOutputs.Remove(Matched);
	}
	for (UEdGraphPin* Pin : OldOutputs)
	{
		Pin->BreakAllPinLinks();
		RemovePin(Pin);
	}
	Pins = MoveTemp(Ordered);
	GetGraph()->NotifyGraphChanged();
}

// 노드와 입출력 핀에 같은 역할 색상을 사용한다.
FLinearColor UDialogueChoiceEdGraphNode::GetNodeTitleColor() const
{
	return FLinearColor(0.65f, 0.2f, 0.85f);
}
