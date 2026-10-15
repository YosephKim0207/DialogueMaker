#include "DialogueLineAuthoring.h"
#include "DialogueFlowEdGraphNode.h"
#include "DialogueLineNodeInfo.h"
#include "EdGraph/EdGraph.h"
#include "Framework/Application/SlateApplication.h"
#include "ScopedTransaction.h"
#include "Widgets/SWindow.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Text/STextBlock.h"

// 화면 위치와 무관하게 역방향 최단 연결 거리의 발화만 찾는다.
TArray<UDialogueFlowEdGraphNode*> FDialogueLineAuthoring::FindNearestLines(const UEdGraphPin* FromPin)
{
	TArray<UDialogueFlowEdGraphNode*> Found;
	if (FromPin == nullptr || FromPin->Direction != EGPD_Output)
	{
		return Found;
	}
	TArray<UEdGraphNode*> Frontier = {FromPin->GetOwningNode()};
	TSet<UEdGraphNode*> Visited;
	while (Frontier.IsEmpty() == false && Found.IsEmpty())
	{
		TArray<UEdGraphNode*> Next;
		for (UEdGraphNode* Node : Frontier)
		{
			if (Node == nullptr || Visited.Contains(Node))
			{
				continue;
			}
			Visited.Add(Node);
			UDialogueFlowEdGraphNode* Line = Cast<UDialogueFlowEdGraphNode>(Node);
			if (Line && Line->GetDialogueNodeType() == EDialogueType::LineNode && Cast<UDialogueLineNodeInfo>(Line->GetNodeInfo()))
			{
				Found.Add(Line);
				continue;
			}
			for (UEdGraphPin* Pin : Node->Pins)
			{
				if (Pin && Pin->Direction == EGPD_Input)
				{
					for (UEdGraphPin* Previous : Pin->LinkedTo)
					{
						if (Previous)
						{
							Next.AddUnique(Previous->GetOwningNode());
						}
					}
				}
			}
		}
		Frontier = MoveTemp(Next);
	}
	Found.Sort([](const UDialogueFlowEdGraphNode& A, const UDialogueFlowEdGraphNode& B) { return A.NodeGuid < B.NodeGuid; });
	return Found;
}

// 선택 확정 후 원본/출발 연결을 재검사하고 한 번의 Undo로 생성과 연결 교체를 복구한다.
UDialogueFlowEdGraphNode* FDialogueLineAuthoring::CreateContinuation(UEdGraph* Graph, UEdGraphPin* FromPin, UDialogueFlowEdGraphNode* Source, FVector2D Location)
{
	if (Graph == nullptr || Graph->bEditable == false || FromPin == nullptr || FromPin->GetOwningNode() == nullptr || FromPin->GetOwningNode()->GetGraph() != Graph ||
		Graph->Nodes.Contains(FromPin->GetOwningNode()) == false || FromPin->GetOwningNode()->Pins.Contains(FromPin) == false ||
		IsValid(Source) == false || FindNearestLines(FromPin).Contains(Source) == false)
	{
		return nullptr;
	}
	const FScopedTransaction Transaction(NSLOCTEXT("DialogueKit", "ContinueSpeaker", "연속 화자 Line 생성"));
	FNewNodeAction Action(UDialogueFlowEdGraphNode::StaticClass(), FText(), FText(), FText(), 0);
	UDialogueFlowEdGraphNode* Created = Cast<UDialogueFlowEdGraphNode>(Action.PerformAction(Graph, FromPin, Location));
	if (Created)
	{
		UDialogueLineNodeInfo* Info = CastChecked<UDialogueLineNodeInfo>(Created->GetNodeInfo());
		Info->Modify();
		Info->CopyContinuationDefaultsFrom(*CastChecked<UDialogueLineNodeInfo>(Source->GetNodeInfo()));
	}
	return Created;
}

// Block 전체 발화 시간을 하나의 트랜잭션으로 바꾼다.
bool FDialogueLineAuthoring::ApplyTiming(UEdGraph* Graph, bool bWholeLine, float Seconds)
{
	if (Graph == nullptr || Graph->bEditable == false || FMath::IsFinite(Seconds) == false || Seconds < 0.0f)
	{
		return false;
	}
	const FScopedTransaction Transaction(NSLOCTEXT("DialogueKit", "BatchTiming", "Block 출력 시간 일괄 변경"));
	Graph->Modify();
	for (UEdGraphNode* Node : Graph->Nodes)
	{
		if (UDialogueFlowEdGraphNode* Flow = Cast<UDialogueFlowEdGraphNode>(Node))
		{
			if (UDialogueLineNodeInfo* Info = Cast<UDialogueLineNodeInfo>(Flow->GetNodeInfo()))
			{
				Info->Modify();
				Info->SetTextTiming(bWholeLine, Seconds);
			}
		}
	}
	Graph->NotifyGraphChanged();
	return true;
}

// 기본 Line 생성과 별개인 상속 액션이다.
FContinueLineAction::FContinueLineAction()
	: FNewNodeAction(UDialogueFlowEdGraphNode::StaticClass(), FText::FromString(TEXT("Nodes")),
	NSLOCTEXT("DialogueKit", "ContinueLine", "연속된 화자의 Line 생성"),
	NSLOCTEXT("DialogueKit", "ContinueLineTip", "가장 가까운 앞선 Line의 화자와 표현 설정을 복사합니다."), 0)
{
}

// 복수 원본은 검색 가능한 미리보기 목록에서 선택하며 취소는 아무것도 만들지 않는다.
UEdGraphNode* FContinueLineAction::PerformAction(UEdGraph* Graph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode)
{
	TArray<UDialogueFlowEdGraphNode*> Candidates = FDialogueLineAuthoring::FindNearestLines(FromPin);
	if (Candidates.Num() == 1)
	{
		return FDialogueLineAuthoring::CreateContinuation(Graph, FromPin, Candidates[0], Location);
	}
	if (Candidates.IsEmpty())
	{
		return nullptr;
	}
	const FEdGraphPinReference PinReference(FromPin);
	TWeakObjectPtr<UEdGraph> WeakGraph(Graph);
	TSharedRef<SWindow> Window = SNew(SWindow).Title(NSLOCTEXT("DialogueKit", "ChooseSource", "복사할 원본 Line 선택"))
		.ClientSize(FVector2D(650, 420)).SupportsMaximize(false).SupportsMinimize(false);
	TSharedRef<FString> Filter = MakeShared<FString>();
	TSharedRef<TWeakObjectPtr<UDialogueFlowEdGraphNode>> Selection = MakeShared<TWeakObjectPtr<UDialogueFlowEdGraphNode>>();
	TSharedRef<SScrollBox> List = SNew(SScrollBox);
	const TWeakPtr<SWindow> WeakWindow(Window);
	for (UDialogueFlowEdGraphNode* Candidate : Candidates)
	{
		const UDialogueLineNodeInfo* Info = CastChecked<UDialogueLineNodeInfo>(Candidate->GetNodeInfo());
		const FSpeakerEmotePair Speaker = Info->GetSpeakerEmotePair();
		const FString Label = FString::Printf(TEXT("%s | %s\nNode %s | 이미지 %s | 표정 %d | 위치 %d (%g,%g) | 반전 %s | 글자당 %gs / 전체 %gs"),
			*StaticEnum<ESpeakerID>()->GetDisplayNameTextByValue(int64(Speaker.Speaker)).ToString(), *Info->GetDialogueText().ToString(),
			*Candidate->NodeGuid.ToString(), *Info->PortraitImage.ToString(), int32(Speaker.EmoteType), int32(Info->GetPortraitData().SidePosition),
			Info->PortraitOffset.X, Info->PortraitOffset.Y, Info->bMirrorPortrait ? TEXT("예") : TEXT("아니요"), Info->SecondsPerCharacter, Info->LineDuration);
		const TWeakObjectPtr<UDialogueFlowEdGraphNode> WeakCandidate(Candidate);
		List->AddSlot()[SNew(SButton)
			.Visibility_Lambda([Filter, Label]() { return Filter->IsEmpty() || Label.Contains(*Filter) ? EVisibility::Visible : EVisibility::Collapsed; })
			.OnClicked_Lambda([Selection, WeakCandidate, WeakWindow]()
			{
				*Selection = WeakCandidate;
				if (const auto Pinned = WeakWindow.Pin())
				{
					Pinned->RequestDestroyWindow();
				}
				return FReply::Handled();
			})[SNew(STextBlock).Text(FText::FromString(Label)).AutoWrapText(true)]];
	}
	Window->SetContent(SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()[SNew(SSearchBox).OnTextChanged_Lambda([Filter](const FText& Text) { *Filter = Text.ToString(); })]
		+ SVerticalBox::Slot().FillHeight(1)[List]
		+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(NSLOCTEXT("DialogueKit", "CancelSource", "닫으면 생성하지 않습니다. 항목을 클릭하면 해당 설정으로 생성합니다."))]);
	FSlateApplication::Get().AddModalWindow(Window, FSlateApplication::Get().GetActiveTopLevelWindow());
	return FDialogueLineAuthoring::CreateContinuation(WeakGraph.Get(), PinReference.Get(), Selection->Get(), Location);
}

// 일괄 시간 편집은 확인 버튼에서만 적용하며 창 닫기는 변경하지 않는다.
void FDialogueLineAuthoring::ShowTimingDialog(UEdGraph* Graph)
{
	if (Graph == nullptr || Graph->bEditable == false)
	{
		return;
	}
	TWeakObjectPtr<UEdGraph> WeakGraph(Graph);
	TSharedRef<float> Seconds = MakeShared<float>(0.0f);
	TSharedRef<SWindow> Window = SNew(SWindow).Title(FText::FromString(TEXT("Block 출력 시간 일괄 변경")))
		.ClientSize(FVector2D(440, 140)).SupportsMaximize(false).SupportsMinimize(false);
	TWeakPtr<SWindow> WeakWindow(Window);
	auto Apply = [WeakGraph, Seconds, WeakWindow](bool bWholeLine)
	{
		if (ApplyTiming(WeakGraph.Get(), bWholeLine, *Seconds))
		{
			if (TSharedPtr<SWindow> Pinned = WeakWindow.Pin())
			{
				Pinned->RequestDestroyWindow();
			}
		}
		return FReply::Handled();
	};
	Window->SetContent(SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(TEXT("모든 Line에 적용합니다. 0초는 즉시 출력입니다.")))]
		+ SVerticalBox::Slot().AutoHeight()[SNew(SNumericEntryBox<float>).MinValue(0.0f)
			.Value_Lambda([Seconds]() { return TOptional<float>(*Seconds); })
			.OnValueChanged_Lambda([Seconds](float Value) { *Seconds = Value; })]
		+ SVerticalBox::Slot().AutoHeight()[SNew(SButton).Text(FText::FromString(TEXT("글자당 시간 적용")))
			.OnClicked_Lambda([Apply]() { return Apply(false); })]
		+ SVerticalBox::Slot().AutoHeight()[SNew(SButton).Text(FText::FromString(TEXT("Line 전체 시간 적용")))
			.OnClicked_Lambda([Apply]() { return Apply(true); })]);
	FSlateApplication::Get().AddModalWindow(Window, FSlateApplication::Get().GetActiveTopLevelWindow());
}
