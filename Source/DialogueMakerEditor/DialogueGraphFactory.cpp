// Fill out your copyright notice in the Description page of Project Settings.


#include "DialogueGraphFactory.h"
#include "DialogueMaker/DialogueGraph.h"

UDialogueGraphFactory::UDialogueGraphFactory(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	SupportedClass = UDialogueGraph::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
}

UObject* UDialogueGraphFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags,
                                                 UObject* Context, FFeedbackContext* Warn)
{
	UDialogueGraph* NewAsset = NewObject<UDialogueGraph>(InParent, InName, Flags);

	return NewAsset;
}
