// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "QuestBase.h"
#include "QuestSubsystem.generated.h"

/**
 * 
 */
UCLASS()
class DIALOGUEMAKER_API UQuestSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	static UQuestSubsystem* Get(const UObject* WorldContextObject);

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	
	void StartQuest(const UQuestBase* Quest) const;
	void SetQuestClear(const UQuestBase* Quest) const;
	bool IsClearedQuest(const UQuestBase* Quest) const;
	void AdvanceQuestStep(UQuestBase* Quest) const;
	
private:
	void AddNewOngoingQuest(const UQuestBase* Quest) const;

	bool LoadProgress();
	void SaveProgress() const;
	
private:
	const FString QuestProgressSaveSlot = TEXT("QuestProgressSaveSlot");
	const int32 SaveIndex = 0;
	
	UPROPERTY()
	class UQuestProgressSaveData* SaveData;
};
