// Fill out your copyright notice in the Description page of Project Settings.


#include "TEST.h"

#include "DialogueSubsystem.h"

// Sets default values
ATEST::ATEST()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void ATEST::BeginPlay()
{
	Super::BeginPlay();
	if (UWorld* World = GetWorld())
	{
		if (UGameInstance* GameInstance = World->GetGameInstance())
		{
			if (UDialogueSubsystem* DialogueSubsystem = GameInstance->GetSubsystem<UDialogueSubsystem>())
			{
				DialogueSubsystem->BeginDialogue(ENPCID::TestNPC);
			}
		}
	}
}

// Called every frame
void ATEST::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

