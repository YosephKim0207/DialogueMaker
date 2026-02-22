// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DialogueGraph.h"
#include "GameFramework/Actor.h"
#include "TEST.generated.h"

UCLASS()
class DIALOGUEKIT_API ATEST : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ATEST();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	public:
    UPROPERTY(EditAnywhere, Category = "DialogueKit|ATEST|Dialogue")
    TSoftObjectPtr<UDialogueGraph> TestDialogueGraph;

};
