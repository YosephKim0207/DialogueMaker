#pragma once

#include "CoreMinimal.h"
#include "IPropertyTypeCustomization.h"

// 새 조건 세 그룹을 먼저 보여주고 기존 조건의 추가 적용을 명시한다.
class FDialogueConditionSetCustomization : public IPropertyTypeCustomization
{
public:
	// 공통 조건 구조체의 편집 레이아웃을 만든다.
	static TSharedRef<IPropertyTypeCustomization> MakeInstance();
	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> Handle, FDetailWidgetRow& Row, IPropertyTypeCustomizationUtils& Utils) override;
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> Handle, IDetailChildrenBuilder& Builder, IPropertyTypeCustomizationUtils& Utils) override;
};
