// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Enum/Portrait.h"
#include "Subsystems/Subsystem.h"
#include "Widgets/Layout/Anchors.h"
#include "PortraitSubsystem.generated.h"

UCLASS()
class DIALOGUEKIT_API UPortraitSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	static UPortraitSubsystem* Get(const UObject* WorldContextObject);

    UFUNCTION(BlueprintCallable, Category = "DialogueKit|PortraitSubsystem|Portrait")
    FAnchors GetPortraitAnchors(EPortraitSide PortraitSide) const;

    UFUNCTION(BlueprintCallable, Category = "DialogueKit|PortraitSubsystem|Portrait")
    FVector2D GetPortraitAlignment(EPortraitSide PortraitSide) const;

private:
	const FAnchors LeftAnchors = FAnchors(0.1f,   0.5f, 0.1f,   0.5f);
	const FAnchors CenterAnchors = FAnchors(0.5f,  0.5f, 0.5f,  0.5f);
	const FAnchors RightAnchors  = FAnchors(0.9f,   0.5f, 0.9f,   0.5f);
	const FVector2D LeftAlignment = FVector2D(0.0f, 0.5f);
	const FVector2D CenterAlignment = FVector2D(0.5f, 0.5f);
	const FVector2D RightAlignment = FVector2D(1.0f, 0.5f);
};
