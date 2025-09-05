// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Enum/PlayerProgress.h"
#include "PlayerProgressSubsystem.generated.h"

/**
 * 
 */
UCLASS()
class DIALOGUEMAKER_API UPlayerProgressSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	static UPlayerProgressSubsystem* Get(const UObject* WorldContextObject);


};