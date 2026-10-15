#pragma once
#include "IDetailCustomization.h"

class FDialogueLineDetails : public IDetailCustomization
{
public:
	static TSharedRef<IDetailCustomization> MakeInstance();
	virtual void CustomizeDetails(IDetailLayoutBuilder& Builder) override;
};
