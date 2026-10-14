// Fill out your copyright notice in the Description page of Project Settings.


#include "DialogueGraphFactory.h"
#include "DialogueGraph.h"

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

	// 신규 저작 에셋에만 안정 ID를 부여한다. 기존 에셋 로드는 이 경로를 거치지 않는다.
	NewAsset->InitializeBlockIdentity();

	return NewAsset;
}
