// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "DialogueSettings.generated.h"

/**
 * 
 */
UCLASS(Config = Game, DefaultConfig, DisplayName = "Dialogue Settings")
class DIALOGUEMAKER_API UDialogueSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UPROPERTY(Config, EditAnywhere, Category = "UI")
	TSoftClassPtr<UUserWidget> DialogueWidgetClass;
};
