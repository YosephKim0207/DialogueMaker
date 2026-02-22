// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PortraitItemWidget.generated.h"

class UImage;

/**
 *
 */
UCLASS()
class DIALOGUEKIT_API UPortraitItemWidget : public UUserWidget
{
	GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "DialogueKit|PortraitItemWidget|Layout")
    void SetBaseTranslation(const FVector2D& TargetPosition);

    UFUNCTION(BlueprintCallable, Category = "DialogueKit|PortraitItemWidget|Layout")
    void SetEmoteImageTranslation(const FVector2D& TargetPosition);

    UFUNCTION(BlueprintCallable, Category = "DialogueKit|PortraitItemWidget|Layout")
    void ResetTranslation();

protected:
	virtual void NativeOnInitialized() override;
	
protected:
    UPROPERTY(BlueprintReadWrite, Category = "DialogueKit|PortraitItemWidget|Widget", meta = (BindWidget))
    class UImage* PortraitImage = nullptr;

    UPROPERTY(BlueprintReadWrite, Category = "DialogueKit|PortraitItemWidget|Widget", meta = (BindWidget))
    UImage* EmoteImage = nullptr;

private:
	UPROPERTY()
	FVector2D BasePortraitImageTranslation = FVector2D::ZeroVector;

	UPROPERTY()
	FVector2D BaseEmoteImageTranslation = FVector2D::ZeroVector;
};
