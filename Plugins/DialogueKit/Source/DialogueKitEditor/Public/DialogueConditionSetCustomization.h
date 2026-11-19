#pragma once

#include "CoreMinimal.h"
#include "IPropertyTypeCustomization.h"

// 공통 조건의 필수·대안·금지 세 그룹과 설명을 표시한다.
class FDialogueConditionSetCustomization : public IPropertyTypeCustomization
{
public:
	// 공통 조건 구조체의 편집 레이아웃을 만든다.
	static TSharedRef<IPropertyTypeCustomization> MakeInstance();
	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> Handle, FDetailWidgetRow& Row, IPropertyTypeCustomizationUtils& Utils) override;
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> Handle, IDetailChildrenBuilder& Builder, IPropertyTypeCustomizationUtils& Utils) override;
};
