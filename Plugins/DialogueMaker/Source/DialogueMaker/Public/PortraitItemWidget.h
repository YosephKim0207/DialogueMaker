// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PortraitItemWidget.generated.h"

/**
 * 
 */
UCLASS()
class DIALOGUEMAKER_API UPortraitItemWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Portrait")
	void SetBaseTranslation(const FVector2D& TargetPosition);
	
	UFUNCTION(BlueprintCallable, Category = "Portrait")
	void SetEmoteImageTranslation(const FVector2D& TargetPosition);
	
	UFUNCTION(BlueprintCallable, Category = "Portrait")
	void ResetTranslation();

protected:
	virtual void NativeOnInitialized() override;
	
protected:
	UPROPERTY(BlueprintReadWrite, Category = "Portrait", meta = (BindWidget))
	class UImage* PortraitImage = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = "Portrait", meta = (BindWidget))
	UImage* EmoteImage = nullptr;

private:
	UPROPERTY()
	FVector2D BasePortraitImageTranslation = FVector2D::ZeroVector;

	UPROPERTY()
	FVector2D BaseEmoteImageTranslation = FVector2D::ZeroVector;
};
