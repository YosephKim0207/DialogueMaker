#include "DialogueConditionSetCustomization.h"
#include "PropertyHandle.h"
#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "IDetailGroup.h"
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
		.ToolTipText(FText::FromString(TEXT("필수 모두 만족, 대안 하나 이상 만족, 금지 조건 없음이 동시에 필요합니다. 빈 그룹은 제한하지 않습니다. 기존 호환 조건이 있으면 그 결과도 함께 적용합니다.")))];
}

// 기존 조건이 있으면 펼쳐 보여 주고 새 그룹과 AND로 적용됨을 안내한다.
void FDialogueConditionSetCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> Handle, IDetailChildrenBuilder& Builder, IPropertyTypeCustomizationUtils& Utils)
{
	for (const FName Name : {FName(TEXT("RequiredConditions")), FName(TEXT("AnyConditions")), FName(TEXT("BlockedConditions"))})
	{
		Builder.AddProperty(Handle->GetChildHandle(Name).ToSharedRef());
	}
	const TSharedRef<IPropertyHandle> Legacy = Handle->GetChildHandle(TEXT("Conditions")).ToSharedRef();
	const TSharedPtr<IPropertyHandleArray> Array = Legacy->AsArray();
	Builder.AddCustomRow(FText::FromString(TEXT("기존 조건 적용 안내"))).WholeRowContent()
	[
		SNew(STextBlock).AutoWrapText(true).Text_Lambda([Array]()
		{
			uint32 Count = 0;
			if (Array->GetNumElements(Count) != FPropertyAccess::Success) return FText::FromString(TEXT("기존 조건 수가 서로 다릅니다. 선택한 각 에셋의 기존 조건도 함께 적용됩니다."));
			return FText::FromString(Count > 0 ? FString::Printf(TEXT("기존 조건 %u개도 함께 적용됩니다. 아래 호환 조건을 확인하세요."), Count) : TEXT("기존 호환 조건 없음. 새 조건은 위 세 그룹에서 작성하세요."));
		})
		.ToolTipText(FText::FromString(TEXT("기존 Combine/Conditions는 자동 삭제·변환하지 않습니다. 기존 결과와 새 그룹 결과를 AND로 결합합니다.")))
	];
	IDetailGroup& Group = Builder.AddGroup(TEXT("LegacyConditionSet"), FText::FromString(TEXT("고급: 기존 호환 조건")));
	Group.AddPropertyRow(Handle->GetChildHandle(TEXT("Combine")).ToSharedRef());
	Group.AddPropertyRow(Legacy);
	uint32 Count = 0;
	if (Array->GetNumElements(Count) != FPropertyAccess::Success || Count > 0) Group.ToggleExpansion(true);
}
