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

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EPortraitActionType ActionType = EPortraitActionType::None;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EEmoteType EmoteType = EEmoteType::None;	// 감정 상황에 맞는 이미지 사용

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EPortraitSide SidePosition = EPortraitSide::Center;
};

USTRUCT(BlueprintType)
struct FPortraitActionData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Portrait")
	ESpeakerID ActionTargetSpeakerID;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Portrait", meta = (ToolTip = "ActionType이 Move인 경우 Action 이후 변화하는 Portarit Image, \nActionType이 Emote인 경우 Portrait에 붙어서 연출되는 Emoji Type"))
	EEmoteType PortraitActionEmoteType = EEmoteType::None;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action")
	EPortraitActionType ActionType = EPortraitActionType::None;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tween")
	float Delay = 0.0f;	// Action을 시작하기 전 Delay 시간

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tween")
	float Duration = 0.0f;	// Action 시작부터 종료까지의 전체 소요 시간

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tween")
	FVector2D FromTranslation = FVector2D::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tween")
	FVector2D ToTranslation = FVector2D::ZeroVector;
};

UCLASS()
class DIALOGUEMAKER_API UDialoguePortraitData : public UPrimaryDataAsset
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
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Speaker", AssetRegistrySearchable, meta = (AllowPrivateAccess = "true"))
	ESpeakerID SpeakerId;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Portrait", meta = (AssetBundles = "Portrait", AllowPrivateAccess = "true"))
	TSoftObjectPtr<UTexture2D> Portrait;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Emote", AssetRegistrySearchable, meta = (AllowPrivateAccess = "true"))
	EEmoteType EmoteType = EEmoteType::None;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
};
