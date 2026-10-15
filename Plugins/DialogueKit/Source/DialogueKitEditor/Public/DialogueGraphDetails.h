#pragma once
#include "CoreMinimal.h"
#include "IDetailCustomization.h"
class FDialogueGraphDetails : public IDetailCustomization
{
public:
	static TSharedRef<IDetailCustomization> MakeInstance();
	virtual void CustomizeDetails(IDetailLayoutBuilder& Builder) override;
};
