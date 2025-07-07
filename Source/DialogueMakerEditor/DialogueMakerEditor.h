// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class FDialogueMakerEditorModule : public IModuleInterface
{
	public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	private:
	TSharedPtr<FSlateStyleSet> StyleSet = nullptr;
	TSharedPtr<struct FDialoguePinFactory> PinFactory = nullptr;
};
