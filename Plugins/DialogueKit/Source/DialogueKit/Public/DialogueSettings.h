// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/AssetManagerTypes.h"
#include "Engine/DeveloperSettings.h"
#include "DialogueSettings.generated.h"

USTRUCT(BlueprintType)
struct FDialogueKitScanType
{
	GENERATED_BODY()

	// "Dialogue" / "Quest" / "DialoguePortraitData" 등
    UPROPERTY(EditAnywhere, Config, Category = "DialogueKit|FDialogueKitScanType|AssetManager")
    FName PrimaryAssetType = NAME_None;

	// "/Script/DialogueKit.DialogueGraph" 등
    UPROPERTY(EditAnywhere, Config, Category = "DialogueKit|FDialogueKitScanType|AssetManager")
    FSoftClassPath AssetBaseClass;

    UPROPERTY(EditAnywhere, Config, Category = "DialogueKit|FDialogueKitScanType|AssetManager")
    bool bHasBlueprintClasses = false;

    UPROPERTY(EditAnywhere, Config, Category = "DialogueKit|FDialogueKitScanType|AssetManager")
    bool bIsEditorOnly = false;

    UPROPERTY(EditAnywhere, Config, Category = "DialogueKit|FDialogueKitScanType|AssetManager")
    TArray<FDirectoryPath> Directories;

    UPROPERTY(EditAnywhere, Config, Category = "DialogueKit|FDialogueKitScanType|AssetManager")
    FPrimaryAssetRules Rules;
};

UCLASS(Config = DialogueKit, DefaultConfig, DisplayName = "DialogueKit Settings")
class DIALOGUEKIT_API UDialogueSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "DialogueKit|DialogueSettings|AssetManager")
    void ApplyKitAssetToAssetManager() const;

public:
    UPROPERTY(Config, EditAnywhere, Category = "DialogueKit|DialogueSettings|UI")
    TSoftClassPtr<UUserWidget> DialogueWidgetClass;

	// 플러그인 고유 카테고리
    UPROPERTY(EditAnywhere, Config, Category = "DialogueKit|DialogueSettings|AssetManager")
    TArray<FDialogueKitScanType> ScanTypes;

};
