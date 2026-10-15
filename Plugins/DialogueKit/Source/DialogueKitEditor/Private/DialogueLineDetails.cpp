#include "DialogueLineDetails.h"
#include "DialogueLineNodeInfo.h"
#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "DetailWidgetRow.h"
#include "ScopedTransaction.h"
#include "Misc/MessageDialog.h"
#include "Widgets/Input/SButton.h"

// Line Details에 명시적인 열람 버전 갱신 버튼을 제공한다.
TSharedRef<IDetailCustomization> FDialogueLineDetails::MakeInstance()
{
	return MakeShared<FDialogueLineDetails>();
}

// 본문 수정만으로 과거 플레이어의 읽음을 바꾸지 않고 버튼을 눌렀을 때만 갱신한다.
void FDialogueLineDetails::CustomizeDetails(IDetailLayoutBuilder& Builder)
{
	TArray<TWeakObjectPtr<UObject>> Objects;
	Builder.GetObjectsBeingCustomized(Objects);
	Builder.EditCategory("DialogueKit|Line|History").AddCustomRow(NSLOCTEXT("DialogueKit", "RenewRead", "미열람 갱신"))
	.WholeRowContent()[SNew(SButton)
		.Text(NSLOCTEXT("DialogueKit", "RenewRead", "미열람 갱신"))
		.ToolTipText(NSLOCTEXT("DialogueKit", "RenewReadTip", "이 Line의 열람 버전을 올립니다. 기존 노드 ID와 진행 위치는 유지합니다."))
		.OnClicked_Lambda([Objects]()
		{
			const FScopedTransaction Transaction(NSLOCTEXT("DialogueKit", "RenewReadTransaction", "Line 미열람 갱신"));
			for (const TWeakObjectPtr<UObject>& Object : Objects)
			{
				if (UDialogueLineNodeInfo* Line = Cast<UDialogueLineNodeInfo>(Object.Get()))
				{
					Line->Modify();
					if (Line->AdvanceReadVersion() == false)
					{
						FMessageDialog::Open(EAppMsgType::Ok, NSLOCTEXT("DialogueKit", "RenewReadFailed", "열람 버전을 갱신할 수 없습니다. 버전 범위를 확인하세요."));
					}
					Line->PostEditChange();
				}
			}
			return FReply::Handled();
		})];
}
