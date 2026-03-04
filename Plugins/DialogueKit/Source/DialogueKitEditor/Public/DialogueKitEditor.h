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
        void BuildSelectDialogueLanguageMenu(class UToolMenu* InMenu);
        void OnMakeCSVMenuClicked();
        void OnMakeDialogueLocalizationDataAssetMenuClicked();
        void OnSelectDialogueLanguage(ELanguage InLanguage);
        bool IsDialogueLanguageSelected(ELanguage InLanguage) const;
        void LoadDialogueLanguageOverrideFromConfig();
        void SaveDialogueLanguageOverrideToConfig(const FString& CultureCode) const;
        FString GetEffectiveDialogueCultureCode() const;
        bool PromptLanguageForCSVExport(ELanguage& OutLanguage, FString& OutCultureCode) const;
        bool OpenDialogueGraphDirectoryDialog(FString& OutSelectedDirectory) const;
        bool OpenDialogueCSVDirectoryDialog(FString& OutSelectedDirectory) const;
        bool ConvertDialogueGraphsInDirectoryToCSV(const FString& SelectedDirectory, const FString& CultureCode) const;
        bool ConvertCSVsInDirectoryToDialogueLocalizationDataAssets(const FString& SelectedDirectory) const;
        void ShowCSVBatchResultWindow(const FString& ResultLog, int32 SuccessCount, int32 FailCount, const FString& OutputRootDirectory) const;
        void ShowDialogueLocalizationBatchResultWindow(const FString& ResultLog, int32 SuccessCount, int32 FailCount, const FString& OutputRootDirectory) const;

private:
        TSharedPtr<FSlateStyleSet> StyleSet = nullptr;
        TSharedPtr<FDialoguePinFactory> PinFactory = nullptr;
        FString DialogueCultureOverride;
        bool bHasDialogueCultureOverride = false;
};
