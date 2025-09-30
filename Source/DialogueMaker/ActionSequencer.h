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
	
	UFUNCTION(BlueprintCallable, meta = (ExpandBoolAsExecs = "ReturnValue"))
	bool TryGetOffScreenPosition(const UCanvasPanelSlot* CanvasPanelSlot,
	const EPortraitSide PortraitSide, FVector2D& OutPosition) const;
	
	virtual class UWorld* GetWorld() const override;
	
private:
	void AdvanceToNextAction();
	void PlayCurrentMoveAction();
	void TickCurrentMoveAction();
	void SkipSequence(const FPortraitActionData& ActionData);
	FVector2D ResolveTargetTranslation(const FPortraitActionData& ActionData) const;
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

	UPROPERTY()
	FTimerHandle DelayTimerHandle;
	
	UPROPERTY()
	FTimerHandle TickTimerHandle;

	const float SequenceTickInterval = 1.0f / 60.0f;

	UPROPERTY()
	FVector2D CachedOriginalRenderTranslation = FVector2D::ZeroVector;
};