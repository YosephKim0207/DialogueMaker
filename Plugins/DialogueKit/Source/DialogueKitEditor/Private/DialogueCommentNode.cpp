#include "DialogueCommentNode.h"
#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "DetailWidgetRow.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "UObject/MetaData.h"
#include "UObject/Package.h"
#include "ScopedTransaction.h"

// Details 인스턴스 생성이다.
TSharedRef<IDetailCustomization> FDialogueCommentDetails::MakeInstance()
{
	return MakeShared<FDialogueCommentDetails>();
}

// 주석 설명의 저작 중 값을 조회한다. 저장 정본은 EditorLayout이다.
FText FDialogueCommentDetails::GetDescription(const UObject* Node)
{
	return Node ? FText::FromString(Node->GetOutermost()->GetMetaData()->GetValue(Node, TEXT("DialogueDescription"))) : FText();
}

// 호출자가 트랜잭션을 소유한다. import에서도 같은 경로로 설명을 복원한다.
void FDialogueCommentDetails::SetDescription(UObject* Node, const FText& Text)
{
	if (Node)
	{
		Node->GetOutermost()->GetMetaData()->Modify();
		Node->GetOutermost()->GetMetaData()->SetValue(Node, TEXT("DialogueDescription"), *Text.ToString());
	}
}

// Dialogue 편집기의 표준 주석에만 별도 설명 입력을 추가한다.
void FDialogueCommentDetails::CustomizeDetails(IDetailLayoutBuilder& Builder)
{
	TArray<TWeakObjectPtr<UObject>> Objects;
	Builder.GetObjectsBeingCustomized(Objects);
	const TWeakObjectPtr<UObject> Node = Objects.IsEmpty() ? nullptr : Objects[0];
	Builder.EditCategory("Comment").AddCustomRow(FText::FromString(TEXT("설명"))).WholeRowContent()
	[SNew(SMultiLineEditableTextBox).HintText(FText::FromString(TEXT("주석 설명")))
		.Text_Lambda([Node]() { return GetDescription(Node.Get()); })
		.OnTextCommitted_Lambda([Node](const FText& Text, ETextCommit::Type)
		{
			if (Node.IsValid())
			{
				const FScopedTransaction Transaction(NSLOCTEXT("DialogueKit", "CommentDescription", "주석 설명 편집"));
				Node->Modify();
				SetDescription(Node.Get(), Text);
			}
		})];
}
