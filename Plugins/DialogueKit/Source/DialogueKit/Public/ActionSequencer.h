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
class DIALOGUEKIT_API UActionSequencer : public UObject
{
	GENERATED_BODY()

public :
        UFUNCTION(BlueprintCallable, Category = "Dialogue|Action Sequencer")
        void Initialize(UObject* NewInWorldContextObject);

        UFUNCTION(BlueprintCallable, Category = "Dialogue|Action Sequencer")
        void AddPortraitWidget(const ESpeakerID PortraitOwner, UWidget* Widget);

        UFUNCTION(BlueprintCallable, Category = "Dialogue|Action Sequencer")
        void StartSequence(const TArray<FPortraitActionData>& ActionDatas, const bool bIsSkip = false);

        UFUNCTION(BlueprintCallable, Category = "Dialogue|Action Sequencer")
        void StopSequence();

        UFUNCTION(BlueprintCallable, Category = "Dialogue|Action Sequencer")
        bool TryGetOffScreenPosition(const UCanvasPanelSlot* CanvasPanelSlot,
        const EPortraitSide PortraitSide, FVector2D& OutPosition);
	
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
        UPROPERTY(Category = "Dialogue|Action Sequencer")
        TWeakObjectPtr<UObject> InWorldContextObject;

        UPROPERTY(Category = "Dialogue|Action Sequencer")
        TArray<FPortraitActionData> CachedActionDatas;

        UPROPERTY(Category = "Dialogue|Action Sequencer")
        TWeakObjectPtr<UPortraitItemWidget> CurrentActionTargetWidget;

        UPROPERTY(Category = "Dialogue|Action Sequencer")
        TMap<ESpeakerID, TWeakObjectPtr<UPortraitItemWidget>> CachedWidgetMap;

	int32 CurrentActionIndex = INDEX_NONE;
	float CurrentActionElapsedTime = 0.0f;

        UPROPERTY(Category = "Dialogue|Action Sequencer")
        FTimerHandle DelayTimerHandle;

        UPROPERTY(Category = "Dialogue|Action Sequencer")
        FTimerHandle TickTimerHandle;

	const float SequenceTickInterval = 1.0f / 60.0f;

        UPROPERTY(Category = "Dialogue|Action Sequencer")
        FVector2D CachedOriginalRenderTranslation = FVector2D::ZeroVector;
};
