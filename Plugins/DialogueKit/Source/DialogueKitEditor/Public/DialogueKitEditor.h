// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class FSlateStyleSet;
struct FDialoguePinFactory;

class FDialogueKitEditorModule : public IModuleInterface
{
public:
        virtual void StartupModule() override;
        virtual void ShutdownModule() override;

private:
        TSharedPtr<FSlateStyleSet> StyleSet = nullptr;
        TSharedPtr<FDialoguePinFactory> PinFactory = nullptr;
};
