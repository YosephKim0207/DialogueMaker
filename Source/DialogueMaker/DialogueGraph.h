// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <functional>

#include "CoreMinimal.h"
#include "DialogueRuntimeGraph.h"
#include "GameplayTagContainer.h"
#include "DialogueGraph.generated.h"

// Use Enum for prevent miss input 
UENUM(BlueprintType)
enum class ENPCID : uint8
{
	// TODO Add Dialogue owned NPC name here
	TestNPC UMETA(DisplayName = "TestNPC"),
	TestNPC2 UMETA(DisplayName = "TestNPC2"),
};

// Use Enum for prevent miss input 
UENUM(BlueprintType)
enum class EChapterID : uint8
{
	// TODO Add Story Chapter here
	Chapter01 UMETA(DisplayName = "Chapter01"),
	Chapter02 UMETA(DisplayName = "Chapter02"),
};

UENUM(BlueprintType)
enum class EDialogueGraphType : uint8
{
	Quest = 0 UMETA(DisplayName = "Quest"),
	Story = 10 UMETA(DisplayName = "Story"),
	DailyChat = 100 UMETA(DisplayName = "DailyChat"),
};

UCLASS(BlueprintType)
class DIALOGUEMAKER_API UDialogueGraph : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// FString GetSpeakerName() const;
	void SetPreSaveListener(std::function<void()> NewOnPreSaveListener);

	virtual void PreSave(FObjectPreSaveContext SaveContext) override;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	virtual void GetAssetRegistryTags(FAssetRegistryTagsContext Context) const override;
public:
	std::function<void()> OnPreSaveListener;

public:	// Properties
	// UPROPERTY(EditAnywhere)
	// FString SpeakerName = FString("Enter Dialogue Name Here");
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UDialogueRuntimeGraph> Graph;

	UPROPERTY(EditAnywhere, AssetRegistrySearchable)
	ENPCID NPCID;

	UPROPERTY(EditAnywhere, AssetRegistrySearchable)
	EChapterID ChapterID;

	UPROPERTY(EditAnywhere, AssetRegistrySearchable)
	EDialogueGraphType DialogueGraphType;
	
	UPROPERTY(EditAnywhere, AssetRegistrySearchable)
	int32 DialoguePriorityWeight;
	
	UPROPERTY(EditAnywhere, Category = "Tags", meta = (ToolTip = "해당 Tag들이 모두 있어야 노출 조건 충족"))
	FGameplayTagContainer RequiredAllTags;

	UPROPERTY(EditAnywhere, Category = "Tags", meta = (ToolTip = "해당 Tag들 중 하나라도 있으면 노출 조건 충족"))
	FGameplayTagContainer RequiredAnyTags;

	UPROPERTY(EditAnywhere, Category = "Tags", meta = (ToolTip = "해당 Tag들이 하나라도 있으면 미노출"))
	FGameplayTagContainer BlockedAnyTags;
	

	
};

