// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/AssetManagerTypes.h"
#include "Engine/DeveloperSettings.h"
#include "GameplayTagContainer.h"
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
	// 개발자가 직접 만든 공통 인사 Block을 에셋 브라우저에서 지정한다.
	UPROPERTY(Config, EditAnywhere, Category = "DialogueKit|DialogueSettings|Fallback")
	TSoftObjectPtr<class UDialogueGraph> CommonGreetingBlock;

	// 저작 도구가 공유하는 정의다. 게임의 현재 Chapter/Scene 진행 상태와 구분한다.
	UPROPERTY(Config, EditAnywhere, Category = "DialogueKit|Story")
	TSoftObjectPtr<class UDialogueStoryDefinition> StoryDefinition;

	// 게임 원본을 공통 상태 정의로 변환하는 에셋이다. 현재 상태 제공자와 구분한다.
	UPROPERTY(Config, EditAnywhere, Category = "DialogueKit|Authoring", meta = (AllowedClasses = "/Script/DialogueKit.DialogueDefinitionProvider", MustImplement = "/Script/DialogueKit.DialogueDefinitionProvider", ToolTip = "조건 ID 선택 목록의 원본 에셋입니다. IDialogueDefinitionProvider를 구현한 카탈로그를 지정하세요. NPC DataAsset·아이템 DataTable은 이 에셋이 공통 정의로 변환합니다. 게임의 현재 상태 제공자를 자동 교체하지 않습니다."))
	TSoftObjectPtr<UObject> ConditionDefinitionSource;

	// 게임별 저작 명칭이다. 표시명 변경은 저장된 대상 Tag나 실행 일치를 바꾸지 않는다.
	UPROPERTY(Config, EditAnywhere, Category = "DialogueKit|Authoring")
	FText TargetFieldLabel = NSLOCTEXT("DialogueKit", "TargetLabel", "대화 대상");

	UPROPERTY(Config, EditAnywhere, Category = "DialogueKit|Authoring")
	TMap<FGameplayTag, FText> TargetDisplayNames;

    UPROPERTY(Config, EditAnywhere, Category = "DialogueKit|DialogueSettings|UI")
    TSoftClassPtr<UUserWidget> DialogueWidgetClass;

	// 플러그인 고유 카테고리
    UPROPERTY(EditAnywhere, Config, Category = "DialogueKit|DialogueSettings|AssetManager")
    TArray<FDialogueKitScanType> ScanTypes;

};
