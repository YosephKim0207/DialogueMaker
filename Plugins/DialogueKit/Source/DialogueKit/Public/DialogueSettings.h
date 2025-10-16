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
	UPROPERTY(EditAnywhere, Config, Category="AssetManager")
	FName PrimaryAssetType = NAME_None;

	// "/Script/DialogueKit.DialogueGraph" 등
	UPROPERTY(EditAnywhere, Config, Category="AssetManager")
	FSoftClassPath AssetBaseClass;

	UPROPERTY(EditAnywhere, Config, Category="AssetManager")
	bool bHasBlueprintClasses = false;

	UPROPERTY(EditAnywhere, Config, Category="AssetManager")
	bool bIsEditorOnly = false;

	UPROPERTY(EditAnywhere, Config, Category="AssetManager")
	TArray<FDirectoryPath> Directories;

	UPROPERTY(EditAnywhere, Config, Category="AssetManager")
	FPrimaryAssetRules Rules; 
};

UCLASS(Config = DialogueKit, DefaultConfig, DisplayName = "DialogueKit Settings")
class DIALOGUEKIT_API UDialogueSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	// ini 기반 스캔 적용
	UFUNCTION(BlueprintCallable, Category="DialogueKit|AssetManager")
	void ApplyKitAssetToAssetManager(const bool bForceSyncScan = true) const;
	
public:
	UPROPERTY(Config, EditAnywhere, Category = "UI")
	TSoftClassPtr<UUserWidget> DialogueWidgetClass;

	// 플러그인 고유 카테고리
	UPROPERTY(EditAnywhere, Config, Category="AssetManager")
	TArray<FDialogueKitScanType> ScanTypes;

};
