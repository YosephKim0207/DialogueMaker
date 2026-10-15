#pragma once
#include "CoreMinimal.h"
#include "IDetailCustomization.h"

// 표준 주석 노드의 그룹 이동을 유지하면서 설명을 편집 전용 metadata로 제공한다.
class FDialogueCommentDetails : public IDetailCustomization
{
public:
	static TSharedRef<IDetailCustomization> MakeInstance();
	static FText GetDescription(const UObject* Node);
	static void SetDescription(UObject* Node, const FText& Text);
	virtual void CustomizeDetails(IDetailLayoutBuilder& Builder) override;
};
