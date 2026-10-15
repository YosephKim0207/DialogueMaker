#include "DialogueGraphEditor.h"
#include "DialogueGraphSearch.h"
#include "DialogueEdGraphNodeBase.h"
#include "DialogueChoiceNodeInfo.h"
#include "GraphEditor.h"
#include "IDetailsView.h"
#include "PropertyPath.h"
#include "Misc/MessageDialog.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

// 그래프 아래에 입력과 스크롤 결과를 유지한다. 입력 변경만으로 검색하지 않는다.
TSharedRef<SWidget> FDialogueGraphEditor::CreateSearchPanel()
{
	return SNew(SBox).HeightOverride(240).Visibility_Lambda([this]() { return bSearchVisible ? EVisibility::Visible : EVisibility::Collapsed; })
	[SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()[SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1)[SAssignNew(SpeakerSearch, SEditableTextBox).OnTextCommitted_Lambda([this](const FText&, ETextCommit::Type Type) { if (Type == ETextCommit::OnEnter) RunSearch(); }).HintText(FText::FromString(TEXT("화자")))]
			+ SHorizontalBox::Slot().FillWidth(2)[SAssignNew(TextSearch, SEditableTextBox).OnTextCommitted_Lambda([this](const FText&, ETextCommit::Type Type) { if (Type == ETextCommit::OnEnter) RunSearch(); }).HintText(FText::FromString(TEXT("대사")))]
			+ SHorizontalBox::Slot().FillWidth(2)[SAssignNew(ChoiceSearch, SEditableTextBox).OnTextCommitted_Lambda([this](const FText&, ETextCommit::Type Type) { if (Type == ETextCommit::OnEnter) RunSearch(); }).HintText(FText::FromString(TEXT("선택지 (단독 검색)")))]
			+ SHorizontalBox::Slot().AutoWidth()[SNew(SButton).Text(FText::FromString(TEXT("검색"))).OnClicked_Lambda([this]() { RunSearch(); return FReply::Handled(); })]
			+ SHorizontalBox::Slot().AutoWidth()[SNew(SButton).Text(FText::FromString(TEXT("닫기"))).OnClicked_Lambda([this]() { bSearchVisible = false; return FReply::Handled(); })]]
		+ SVerticalBox::Slot().FillHeight(1)[SAssignNew(SearchResults, SScrollBox)]];
}

// 단축키와 메뉴는 같은 패널을 열고 첫 입력칸에 포커스를 준다.
void FDialogueGraphEditor::ShowSearch()
{
	bSearchVisible = true;
	if (SpeakerSearch.IsValid()) FSlateApplication::Get().SetKeyboardFocus(SpeakerSearch);
}

// 결과를 클릭해도 목록과 입력을 유지한다. 혼합 입력은 기존 결과를 변경하지 않는다.
void FDialogueGraphEditor::RunSearch()
{
	if (SearchResults.IsValid() == false) return;
	TArray<FDialogueSearchResult> Results;
	FText Error;
	if (FDialogueGraphSearch::Search(WorkingGraph, SpeakerSearch->GetText().ToString(), TextSearch->GetText().ToString(), ChoiceSearch->GetText().ToString(), Results, Error) == false)
	{
		FMessageDialog::Open(EAppMsgType::Ok, Error);
		return;
	}
	SearchResults->ClearChildren();
	if (Results.IsEmpty())
	{
		FMessageDialog::Open(EAppMsgType::Ok, Error);
		return;
	}
	for (const FDialogueSearchResult& Result : Results)
	{
		SearchResults->AddSlot()[SNew(SButton).OnClicked_Lambda([this, Result]()
		{
			FocusNode(Result.Node, Result.ChoiceId);
			return FReply::Handled();
		})[SNew(STextBlock).Text(Result.Label).AutoWrapText(true)]];
	}
}

// 오래된 결과의 노드/선택지가 삭제됐는지 재검사하고 Details까지 이동한다.
void FDialogueGraphEditor::FocusNode(TWeakObjectPtr<UEdGraphNode> Node, FGuid ChoiceId)
{
	if (Node.IsValid() == false || WorkingGraph->Nodes.Contains(Node.Get()) == false || WorkingGraphUI.IsValid() == false)
	{
		FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(TEXT("해당 노드가 없습니다. 다시 검색하세요.")));
		return;
	}
	WorkingGraphUI->ClearSelectionSet();
	WorkingGraphUI->JumpToNode(Node.Get(), false);
	WorkingGraphUI->SetNodeSelection(Node.Get(), true);
	if (ChoiceId.IsValid())
	{
		auto* Typed = Cast<UDialogueEdGraphNodeBase>(Node.Get());
		auto* Choice = Typed ? Cast<UDialogueChoiceNodeInfo>(Typed->GetNodeInfo()) : nullptr;
		const int32 Index = Choice ? Choice->Options.IndexOfByPredicate([ChoiceId](const FDialogueOption& Option) { return Option.ChoiceId == ChoiceId; }) : INDEX_NONE;
		if (Index == INDEX_NONE)
		{
			FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(TEXT("해당 선택지가 삭제되었습니다. 다시 검색하세요.")));
			return;
		}
		if (SelectedDetailView.IsValid())
		{
			TSharedRef<FPropertyPath> Path = FPropertyPath::CreateEmpty();
			Path->AddProperty(FPropertyInfo(FindFProperty<FProperty>(UDialogueChoiceNodeInfo::StaticClass(), TEXT("Options")), Index));
			SelectedDetailView->ScrollPropertyIntoView(*Path, true);
			SelectedDetailView->HighlightProperty(*Path);
		}
	}
}
