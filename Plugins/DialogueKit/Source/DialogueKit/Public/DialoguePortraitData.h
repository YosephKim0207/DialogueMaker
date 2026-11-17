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

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DialogueKit|FPortraitData|Action", meta = (ToolTip = "현재 기본 실행 코드에서 사용하지 않는 초상화 동작 필드입니다. 표정과 위치는 Emote Type 및 Side Position으로 지정합니다."))
    EPortraitActionType ActionType = EPortraitActionType::None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DialogueKit|FPortraitData|Emote", meta = (ToolTip = "화자의 표정입니다. 화자 ID와 함께 초상화 자료를 찾는 데 사용합니다."))
    EEmoteType EmoteType = EEmoteType::None;        // 감정 상황에 맞는 이미지 사용

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DialogueKit|FPortraitData|Placement", meta = (ToolTip = "화자 초상화의 기준 화면 위치입니다. Left/Center/Right 중 선택합니다."))
    EPortraitSide SidePosition = EPortraitSide::Center;
};

USTRUCT(BlueprintType)
struct FPortraitActionData
{
	GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DialogueKit|FPortraitActionData|Portrait", meta = (ToolTip = "구형 초상화 액션을 적용할 화자입니다. 현재 대사를 말하는 화자와 별도로 지정할 수 있습니다."))
    ESpeakerID ActionTargetSpeakerID = ESpeakerID::TestNPC;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DialogueKit|FPortraitActionData|Portrait", meta = (ToolTip = "구형 초상화 액션의 표정 지정용 필드입니다. 현재 실행 코드에서 읽지 않으므로 값을 바꿔도 이동 후 표정이나 감정 연출이 적용되지 않습니다."))
    EEmoteType PortraitActionEmoteType = EEmoteType::None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DialogueKit|FPortraitActionData|Action", meta = (ToolTip = "구형 초상화 액션 종류입니다. 현재 Move만 이동을 실행하며 Emote/None은 연출을 실행하지 않습니다. 신규 Block 기본 화면은 이 배열을 자동 실행하지 않습니다."))
    EPortraitActionType ActionType = EPortraitActionType::None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DialogueKit|FPortraitActionData|Tween", meta = (ToolTip = "구형 Move 액션을 시작하기 전 대기 시간입니다. 단위는 초이며 다른 액션 종류에서는 사용하지 않습니다."))
    float Delay = 0.0f;     // Action을 시작하기 전 Delay 시간

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DialogueKit|FPortraitActionData|Tween", meta = (ToolTip = "구형 Move 액션의 이동 시간입니다. 단위는 초이며 다른 액션 종류에서는 사용하지 않습니다."))
    float Duration = 0.0f;  // Action 시작부터 종료까지의 전체 소요 시간

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DialogueKit|FPortraitActionData|Tween", meta = (ToolTip = "구형 Move 액션의 시작 위치입니다. X/Y가 모두 0에 가까우면 현재 초상화 위치에서 시작합니다."))
    FVector2D FromTranslation = FVector2D::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DialogueKit|FPortraitActionData|Tween", meta = (ToolTip = "구형 Move 액션이 도착할 화면 기준 위치입니다. 다른 액션 종류에서는 사용하지 않습니다."))
    EPortraitSide TargetSide = EPortraitSide::Center;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DialogueKit|FPortraitActionData|Tween", meta = (ToolTip = "구형 Move 액션의 도착 기준 위치에 더하는 X/Y 오프셋입니다. 다른 액션 종류에서는 사용하지 않습니다."))
    FVector2D TargetSideOffset = FVector2D::ZeroVector;
};

UCLASS()
class DIALOGUEKIT_API UDialoguePortraitData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UTexture2D* GetPortraitTexture() const;
	// 현재 표시할 초상화만 명시적으로 로드한다.
	UTexture2D* LoadPortraitTexture() const;

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
