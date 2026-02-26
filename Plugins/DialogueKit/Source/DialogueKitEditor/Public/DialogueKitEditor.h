// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class FSlateStyleSet;
struct FDialoguePinFactory;
enum class ELanguage : uint8;

class FDialogueKitEditorModule : public IModuleInterface
{
public:
        virtual void StartupModule() override;
        virtual void ShutdownModule() override;

private:
        void RegisterMainMenu();
        void BuildDialogueMainMenu(class UToolMenu* InMenu);
        void OnMakeCSVMenuClicked();
        bool PromptLanguageForCSVExport(ELanguage& OutLanguage, FString& OutCultureCode) const;
        bool OpenDialogueGraphDirectoryDialog(FString& OutSelectedDirectory) const;
        bool ConvertDialogueGraphsInDirectoryToCSV(const FString& SelectedDirectory, const FString& CultureCode) const;
        void ShowCSVBatchResultWindow(const FString& ResultLog, int32 SuccessCount, int32 FailCount, const FString& OutputRootDirectory) const;

private:
        TSharedPtr<FSlateStyleSet> StyleSet = nullptr;
        TSharedPtr<FDialoguePinFactory> PinFactory = nullptr;
};
