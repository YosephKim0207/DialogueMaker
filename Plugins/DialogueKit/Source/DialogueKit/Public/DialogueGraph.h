// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <functional>

#include "CoreMinimal.h"
#include "DialogueRuntimeGraph.h"
#include "DialogueBlockIdentity.h"
#include "GameplayTagContainer.h"
#include "Enum/PlayerProgress.h"
#include "Enum/Portrait.h"
#include "DialogueGraph.generated.h"

UENUM(BlueprintType)
enum class EDialogueGraphType : uint8
{
	Quest = 0 UMETA(DisplayName = "Quest"),
	Story = 10 UMETA(DisplayName = "Story"),
	DailyChat = 100 UMETA(DisplayName = "DailyChat"),
};

USTRUCT(BlueprintType)
struct FPortraitInitData
{
	GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DialogueKit|FPortraitInitData|Speaker")
    ESpeakerID Speaker;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DialogueKit|FPortraitInitData|Emote")
    EEmoteType EmoteType;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DialogueKit|FPortraitInitData|Portrait")
    EPortraitSide PortraitSide;
};

UCLASS(BlueprintType)
class DIALOGUEKIT_API UDialogueGraph : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
    // 새 에셋 생성 시 누락된 ID만 초기화한다. 로드/저장에서는 자동 호출하지 않는다.
    // 이미 ID가 있거나 템플릿 객체이면 false이며 기존 ID는 변경하지 않는다.
    bool InitializeBlockIdentity();

    // 이름과 경로에 독립적인 Block 식별자를 조회한다.
    UFUNCTION(BlueprintPure, Category = "DialogueKit|Identity")
    FGuid GetBlockId() const { return BlockId; }

    // 저장 형식/열람 버전과 구분되는 실행 콘텐츠 버전을 조회한다.
    UFUNCTION(BlueprintPure, Category = "DialogueKit|Identity")
    int32 GetContentVersion() const { return ContentVersion; }

    // 정확한 콘텐츠를 지칭하는 ID/버전 쌍을 조회한다.
    UFUNCTION(BlueprintPure, Category = "DialogueKit|Identity")
    FDialogueBlockReference GetBlockReference() const;

    // 일반 저작 복제만 새 Block으로 취급하고 World/PIE 복제는 동일 콘텐츠로 유지한다.
    virtual void PostDuplicate(EDuplicateMode::Type DuplicateMode) override;

private:
    UPROPERTY(VisibleAnywhere, Category = "DialogueKit|Identity")
    FGuid BlockId;

    UPROPERTY(VisibleAnywhere, Category = "DialogueKit|Identity")
    int32 ContentVersion = 1;

public:
	// FString GetSpeakerName() const;
	void SetPreSaveListener(std::function<void()> NewOnPreSaveListener);

	virtual void PreSave(FObjectPreSaveContext SaveContext) override;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	virtual void GetAssetRegistryTags(FAssetRegistryTagsContext Context) const override;

	static FORCEINLINE FName GetSpeakerIDTag()
	{
		return GET_MEMBER_NAME_CHECKED(UDialogueGraph, SpeakerID);
	}

	static FORCEINLINE FName GetChapterIDTag()
	{
		return GET_MEMBER_NAME_CHECKED(UDialogueGraph, ChapterID);
	}
	
public:
	std::function<void()> OnPreSaveListener;

public:	// Properties
	// UPROPERTY(EditAnywhere)
	// FString SpeakerName = FString("Enter Dialogue Name Here");
        UPROPERTY(VisibleAnywhere, Category = "DialogueKit|DialogueGraph|Runtime")
        TObjectPtr<UDialogueRuntimeGraph> Graph;

        UPROPERTY(EditAnywhere, AssetRegistrySearchable, Category = "DialogueKit|DialogueGraph|Filter - Default")
        ESpeakerID SpeakerID;

        UPROPERTY(EditAnywhere, AssetRegistrySearchable, Category = "DialogueKit|DialogueGraph|Filter - Default")
        EChapterID ChapterID;
	
	UPROPERTY(EditAnywhere, Category = "DialogueKit|DialogueGraph|Filter - Tags", meta = (ToolTip = "해당 Tag들이 모두 있어야 노출 조건 충족"))
	FGameplayTagContainer RequiredAllTags;

	UPROPERTY(EditAnywhere, Category = "DialogueKit|DialogueGraph|Filter - Tags", meta = (ToolTip = "해당 Tag들 중 하나라도 있으면 노출 조건 충족"))
	FGameplayTagContainer RequiredAnyTags;

	UPROPERTY(EditAnywhere, Category = "DialogueKit|DialogueGraph|Filter - Tags", meta = (ToolTip = "해당 Tag들이 하나라도 있으면 미노출"))
	FGameplayTagContainer BlockedAnyTags;
	
        UPROPERTY(EditAnywhere, AssetRegistrySearchable, Category = "DialogueKit|DialogueGraph|Filter - Order")
        EDialogueGraphType DialogueGraphType;

        UPROPERTY(EditAnywhere, AssetRegistrySearchable, Category = "DialogueKit|DialogueGraph|Filter - Order")
        int32 DialoguePriorityWeight;

        UPROPERTY(EditAnywhere, AssetRegistrySearchable, Category = "DialogueKit|DialogueGraph|Initial Portrait")
        TArray<FPortraitInitData> InitPortraits;
};

