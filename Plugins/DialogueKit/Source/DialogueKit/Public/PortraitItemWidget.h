// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PortraitItemWidget.generated.h"

/**
 * 
 */
UCLASS()
class DIALOGUEKIT_API UPortraitItemWidget : public UUserWidget
{
	GENERATED_BODY()

public:
        UFUNCTION(BlueprintCallable, Category = "Dialogue|Portrait Widget")
        void SetBaseTranslation(const FVector2D& TargetPosition);

        UFUNCTION(BlueprintCallable, Category = "Dialogue|Portrait Widget")
        void SetEmoteImageTranslation(const FVector2D& TargetPosition);

        UFUNCTION(BlueprintCallable, Category = "Dialogue|Portrait Widget")
        void ResetTranslation();

protected:
	virtual void NativeOnInitialized() override;
	
protected:
        UPROPERTY(BlueprintReadWrite, Category = "Dialogue|Portrait Widget", meta = (BindWidget))
        class UImage* PortraitImage = nullptr;

        UPROPERTY(BlueprintReadWrite, Category = "Dialogue|Portrait Widget", meta = (BindWidget))
        UImage* EmoteImage = nullptr;

private:
        UPROPERTY(Category = "Dialogue|Portrait Widget")
        FVector2D BasePortraitImageTranslation = FVector2D::ZeroVector;

        UPROPERTY(Category = "Dialogue|Portrait Widget")
        FVector2D BaseEmoteImageTranslation = FVector2D::ZeroVector;
};
