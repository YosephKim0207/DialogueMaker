#include "DialogueGraphEditor.h"
#include "DialogueGraphValidation.h"
#include "Framework/Application/SlateApplication.h"
#include "Misc/MessageDialog.h"
#include "Widgets/SWindow.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Text/STextBlock.h"

// 수동 검사는 저장을 실행하지 않는다.
void FDialogueGraphEditor::CheckGraph()
{
	ConfirmGraphSave(false);
}

// 경고는 선택적으로 저장하지만 차단 항목이 있으면 계속 버튼을 비활성화한다.
bool FDialogueGraphEditor::ConfirmGraphSave(bool bSaving)
{
	const TArray<FDialogueGraphIssue> Issues = FDialogueGraphValidation::Inspect(WorkingGraph);
	if (Issues.IsEmpty())
	{
		if (bSaving == false) FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(TEXT("그래프 검사: 문제가 없습니다.")));
		return true;
	}
	TSharedRef<bool> Approved = MakeShared<bool>(false);
	TSharedRef<SWindow> Window = SNew(SWindow).Title(FText::FromString(TEXT("DialogueMaker 그래프 검사")))
		.ClientSize(FVector2D(720, 420)).SupportsMaximize(false).SupportsMinimize(false);
	TWeakPtr<SWindow> WeakWindow(Window);
	TSharedRef<SScrollBox> List = SNew(SScrollBox);
	for (const FDialogueGraphIssue& Issue : Issues)
	{
		const FString Label = FString::Printf(TEXT("%s [%s] %s\nNode: %s"), Issue.bBlocksSave ? TEXT("저장 차단") : TEXT("경고"),
			*Issue.Code.ToString(), *Issue.Message.ToString(), Issue.Node.IsValid() ? *Issue.Node->NodeGuid.ToString() : TEXT("Graph"));
		List->AddSlot()[SNew(SButton).OnClicked_Lambda([this, Issue]()
		{
			if (Issue.Node.IsValid()) FocusNode(Issue.Node);
			return FReply::Handled();
		})[SNew(STextBlock).Text(FText::FromString(Label)).AutoWrapText(true)]];
	}
	Window->SetContent(SNew(SVerticalBox)
		+ SVerticalBox::Slot().FillHeight(1)[List]
		+ SVerticalBox::Slot().AutoHeight()[SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1)[SNew(SButton).Text(FText::FromString(bSaving ? TEXT("저장 취소") : TEXT("닫기")))
				.OnClicked_Lambda([WeakWindow]() { if (auto Pinned = WeakWindow.Pin()) Pinned->RequestDestroyWindow(); return FReply::Handled(); })]
			+ SHorizontalBox::Slot().FillWidth(1)[SNew(SButton).Text(FText::FromString(TEXT("그래도 저장"))).Visibility(bSaving ? EVisibility::Visible : EVisibility::Collapsed)
				.IsEnabled(FDialogueGraphValidation::CanSave(Issues)).OnClicked_Lambda([Approved, WeakWindow]()
				{
					*Approved = true;
					if (auto Pinned = WeakWindow.Pin()) Pinned->RequestDestroyWindow();
					return FReply::Handled();
				})]]);
	FSlateApplication::Get().AddModalWindow(Window, FSlateApplication::Get().GetActiveTopLevelWindow());
	return *Approved;
}
