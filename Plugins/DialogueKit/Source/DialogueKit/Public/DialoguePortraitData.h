// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Enum/PlayerProgress.h"
#include "Enum/Portrait.h"
#include "DialoguePortraitData.generated.h"

USTRUCT(BlueprintType)
struct FPortraitData
{
	GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DialogueKit|FPortraitData|Action")
    EPortraitActionType ActionType = EPortraitActionType::None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DialogueKit|FPortraitData|Emote")
    EEmoteType EmoteType = EEmoteType::None;        // 감정 상황에 맞는 이미지 사용

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DialogueKit|FPortraitData|Placement")
    EPortraitSide SidePosition = EPortraitSide::Center;
};

USTRUCT(BlueprintType)
struct FPortraitActionData
{
	GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DialogueKit|FPortraitActionData|Portrait")
    ESpeakerID ActionTargetSpeakerID;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DialogueKit|FPortraitActionData|Portrait", meta = (ToolTip = "ActionType이 Move인 경우 Action 이후 변화하는 Portarit Image, \nActionType이 Emote인 경우 Portrait에 붙어서 연출되는 Emoji Type"))
    EEmoteType PortraitActionEmoteType = EEmoteType::None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DialogueKit|FPortraitActionData|Action")
    EPortraitActionType ActionType = EPortraitActionType::None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DialogueKit|FPortraitActionData|Tween")
    float Delay = 0.0f;     // Action을 시작하기 전 Delay 시간

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DialogueKit|FPortraitActionData|Tween")
    float Duration = 0.0f;  // Action 시작부터 종료까지의 전체 소요 시간

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DialogueKit|FPortraitActionData|Tween")
    FVector2D FromTranslation = FVector2D::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DialogueKit|FPortraitActionData|Tween")
    EPortraitSide TargetSide = EPortraitSide::Center;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DialogueKit|FPortraitActionData|Tween")
    FVector2D TargetSideOffset = FVector2D::ZeroVector;
};

UCLASS()
class DIALOGUEKIT_API UDialoguePortraitData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UTexture2D* GetPortraitTexture() const;

	static FORCEINLINE FName GetSpeakerIDTag()
	{
		return GET_MEMBER_NAME_CHECKED(UDialoguePortraitData, SpeakerId);
	}

	static FORCEINLINE FName GetEmoteTypeTag()
	{
		return GET_MEMBER_NAME_CHECKED(UDialoguePortraitData, EmoteType);
	}

private:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DialogueKit|DialoguePortraitData|Speaker", AssetRegistrySearchable, meta = (AllowPrivateAccess = "true"))
	ESpeakerID SpeakerId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DialogueKit|DialoguePortraitData|Portrait", meta = (AssetBundles = "Portrait", AllowPrivateAccess = "true"))
	TSoftObjectPtr<UTexture2D> Portrait;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DialogueKit|DialoguePortraitData|Emote", AssetRegistrySearchable, meta = (AllowPrivateAccess = "true"))
	EEmoteType EmoteType = EEmoteType::None;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
};
