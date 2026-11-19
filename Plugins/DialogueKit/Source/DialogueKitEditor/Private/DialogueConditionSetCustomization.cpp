#include "DialogueConditionSetCustomization.h"
#include "PropertyHandle.h"
#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "Widgets/Text/STextBlock.h"

// 공통 평가 구조를 사용하는 Entry/Branch/Choice에 같은 UI를 제공한다.
TSharedRef<IPropertyTypeCustomization> FDialogueConditionSetCustomization::MakeInstance()
{
	return MakeShared<FDialogueConditionSetCustomization>();
}

// 모든 빈 그룹은 제한하지 않으며 세 그룹은 동시에 적용한다는 의미를 표시한다.
void FDialogueConditionSetCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> Handle, FDetailWidgetRow& Row, IPropertyTypeCustomizationUtils& Utils)
{
	Row.NameContent()[Handle->CreatePropertyNameWidget()]
	.ValueContent()[SNew(STextBlock).Text(FText::FromString(TEXT("필수 + 대안 + 금지")))
		.ToolTipText(FText::FromString(TEXT("필수 모두 만족, 대안 하나 이상 만족, 금지 조건 없음이 동시에 필요합니다. 빈 그룹은 제한하지 않습니다.")))];
}

// 단일 조건 체계의 세 그룹만 편집하도록 제공한다.
void FDialogueConditionSetCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> Handle, IDetailChildrenBuilder& Builder, IPropertyTypeCustomizationUtils& Utils)
{
	for (const FName Name : {FName(TEXT("RequiredConditions")), FName(TEXT("AnyConditions")), FName(TEXT("BlockedConditions"))})
	{
		Builder.AddProperty(Handle->GetChildHandle(Name).ToSharedRef());
	}

}
