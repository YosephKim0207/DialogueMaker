#pragma once
#include "IDetailCustomization.h"
#include "EditorUndoClient.h"

class IPropertyUtilities;

// 기본 DataAsset 편집에서도 안정 ID와 이름 기반 소속 선택을 사용한다.
class FDialogueStoryDefinitionDetails : public IDetailCustomization, public FEditorUndoClient
{
public:
	static TSharedRef<IDetailCustomization> MakeInstance();
	virtual ~FDialogueStoryDefinitionDetails() override;
	virtual void CustomizeDetails(IDetailLayoutBuilder& Builder) override;
	virtual void PostUndo(bool bSuccess) override;
	virtual void PostRedo(bool bSuccess) override;
private:
	TWeakPtr<IPropertyUtilities> Utilities;
	bool bRegisteredUndo = false;
};
