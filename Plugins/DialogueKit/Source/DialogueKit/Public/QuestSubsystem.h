// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "QuestBase.h"
#include "QuestSubsystem.generated.h"

/**
 * 
 */
UCLASS()
class DIALOGUEKIT_API UQuestSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	static UQuestSubsystem* Get(const UObject* WorldContextObject);

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	
	void StartQuest(const UQuestBase* Quest) const;
	void SetQuestClear(const UQuestBase* Quest) const;
	bool IsClearedQuest(const UQuestBase* Quest) const;
	void AdvanceQuestStep(UQuestBase* Quest) const;

	void SaveProgress() const;
private:
	// 초기화 당시의 격리 상태를 유지하여 PIE 종료 중에도 파일에 접근하지 않는다.
	bool bPreviewPersistenceBlocked = false;
	void AddNewOngoingQuest(const UQuestBase* Quest) const;

	bool LoadProgress();
	
private:
	const FString QuestProgressSaveSlot = TEXT("QuestProgressSaveSlot");
	const int32 SaveIndex = 0;
	
	UPROPERTY()
	class UQuestProgressSaveData* SaveData;
};
