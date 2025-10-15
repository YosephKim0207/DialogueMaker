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
	EEmoteType EmoteType = EEmoteType::None;	// Í∞êÏ†ï ?ÅÌô©??ÎßûÎäî ?¥Î?ÏßÄ ?¨Ïö©

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EPortraitSide SidePosition = EPortraitSide::Center;
};

USTRUCT(BlueprintType)
struct FPortraitActionData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Portrait")
	ESpeakerID ActionTargetSpeakerID;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Portrait", meta = (ToolTip = "ActionType??Move??Í≤ΩÏö∞ Action ?¥ÌõÑ Î≥Ä?îÌïò??Portarit Image, \nActionType??Emote??Í≤ΩÏö∞ Portrait??Î∂ôÏñ¥???∞Ï∂ú?òÎäî Emoji Type"))
	EEmoteType PortraitActionEmoteType = EEmoteType::None;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Action")
	EPortraitActionType ActionType = EPortraitActionType::None;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tween")
	float Delay = 0.0f;	// Action???úÏûë?òÍ∏∞ ??Delay ?úÍ∞Ñ

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tween")
	float Duration = 0.0f;	// Action ?úÏûëÎ∂Ä??Ï¢ÖÎ£åÍπåÏ????ÑÏ≤¥ ?åÏöî ?úÍ∞Ñ

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tween")
	FVector2D FromTranslation = FVector2D::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tween")
	EPortraitSide TargetSide = EPortraitSide::Center;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tween")
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
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Speaker", AssetRegistrySearchable, meta = (AllowPrivateAccess = "true"))
	ESpeakerID SpeakerId;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Portrait", meta = (AssetBundles = "Portrait", AllowPrivateAccess = "true"))
	TSoftObjectPtr<UTexture2D> Portrait;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Emote", AssetRegistrySearchable, meta = (AllowPrivateAccess = "true"))
	EEmoteType EmoteType = EEmoteType::None;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
};
