// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DialoguePortraitData.h"
#include "PortraitItemWidget.h"
#include "Enum/PlayerProgress.h"
#include "ActionSequencer.generated.h"

/**
 * 
 */

UCLASS(BlueprintType)
class DIALOGUEMAKER_API UActionSequencer : public UObject
{
	GENERATED_BODY()

public :
	UFUNCTION(BlueprintCallable)
	void Initialize(UObject* NewInWorldContextObject);
	
	UFUNCTION(BlueprintCallable)
	void AddPortraitWidget(const ESpeakerID PortraitOwner, UWidget* Widget);
	
	UFUNCTION(BlueprintCallable)
	void StartSequence(const TArray<FPortraitActionData>& ActionDatas, const bool bIsSkip = false);
	
	UFUNCTION(BlueprintCallable)
	void StopSequence();

	virtual class UWorld* GetWorld() const override;
	
private:
	void AdvanceToNextAction();
	void PlayCurrentMoveAction();
	void TickCurrentMoveAction();
	void SkipSequence(const FPortraitActionData& ActionData);
	void ResetState();
	void ClearAllTimer();
	
private:
	UPROPERTY()
	TWeakObjectPtr<UObject> InWorldContextObject;
	
	UPROPERTY()
	TArray<FPortraitActionData> CachedActionDatas;
	
	UPROPERTY()
	TWeakObjectPtr<UPortraitItemWidget> CurrentActionTargetWidget;

	UPROPERTY()
	TMap<ESpeakerID, TWeakObjectPtr<UPortraitItemWidget>> CachedWidgetMap;

	int32 CurrentActionIndex = INDEX_NONE;
	float CurrentActionElapsedTime = 0.0f;
	
	FTimerHandle DelayTimerHandle;
	FTimerHandle TickTimerHandle;

	const float SequenceTickInterval = 1.0f / 60.0f;
};