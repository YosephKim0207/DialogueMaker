// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <functional>

#include "CoreMinimal.h"
#include "DialogueRuntimeGraph.h"
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

        UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Initial Portrait")
        ESpeakerID Speaker;

        UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Initial Portrait")
        EEmoteType EmoteType;

        UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialogue|Initial Portrait")
        EPortraitSide PortraitSide;
};

UCLASS(BlueprintType)
class DIALOGUEKIT_API UDialogueGraph : public UPrimaryDataAsset
{
	GENERATED_BODY()

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
        UPROPERTY(VisibleAnywhere, Category = "Dialogue|Graph")
        TObjectPtr<UDialogueRuntimeGraph> Graph;

	UPROPERTY(EditAnywhere, AssetRegistrySearchable, Category = "Filter - Default")
	ESpeakerID SpeakerID;

	UPROPERTY(EditAnywhere, AssetRegistrySearchable, Category = "Filter - Default")
	EChapterID ChapterID;
	
	UPROPERTY(EditAnywhere, Category = "Filter - Tags", meta = (ToolTip = "해당 Tag들이 모두 있어야 노출 조건 충족"))	FGameplayTagContainer RequiredAllTags;

	UPROPERTY(EditAnywhere, Category = "Filter - Tags", meta = (ToolTip = "해당 Tag들 중 하나라도 있으면 노출 조건 충족"))
	FGameplayTagContainer RequiredAnyTags;

	UPROPERTY(EditAnywhere, Category = "Filter - Tags", meta = (ToolTip = "해당 Tag들이 하나라도 있으면 미노출"))	FGameplayTagContainer BlockedAnyTags;
	
	UPROPERTY(EditAnywhere, AssetRegistrySearchable, Category = "Filter - Order")
	EDialogueGraphType DialogueGraphType;
	
	UPROPERTY(EditAnywhere, AssetRegistrySearchable, Category = "Filter - Order")
	int32 DialoguePriorityWeight;

	UPROPERTY(EditAnywhere, AssetRegistrySearchable, Category = "Initial Portrait")
	TArray<FPortraitInitData> InitPortraits;
};

