// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "DialogueGraphFactory.generated.h"

/**
 * 
 */
UCLASS()
class DIALOGUEMAKEREDITOR_API UDialogueGraphFactory : public UFactory
{
	GENERATED_BODY()

public:
	UDialogueGraphFactory(const FObjectInitializer& ObjectInitializer);
	virtual UObject* FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
};
