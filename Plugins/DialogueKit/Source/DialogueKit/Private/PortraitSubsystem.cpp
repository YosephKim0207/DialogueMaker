// Fill out your copyright notice in the Description page of Project Settings.


#include "PortraitSubsystem.h"

#include "Engine/Engine.h"
#include "Engine/World.h"

DEFINE_LOG_CATEGORY_STATIC(PortraitSubsystemLog, Log, All);

UPortraitSubsystem* UPortraitSubsystem::Get(const UObject* WorldContextObject)
{
	if (WorldContextObject == nullptr)
	{
		return nullptr;
	}

	UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	if (World == nullptr)
	{
		UE_LOG(PortraitSubsystemLog, Warning, TEXT("UPortraitSubsystem::Get : World is nullptr"));
	
		return nullptr;
	}
	
	if (UGameInstance* GameInstance = World->GetGameInstance())
	{
		return GameInstance->GetSubsystem<UPortraitSubsystem>();
	}
				
	UE_LOG(PortraitSubsystemLog, Warning, TEXT("UPortraitSubsystem::Get : GameInstance is nullptr"));

	return nullptr;
}

FAnchors UPortraitSubsystem::GetPortraitAnchors(EPortraitSide PortraitSide) const
{

	switch (PortraitSide)
	{
		case EPortraitSide::Center :
			return CenterAnchors;
		case EPortraitSide::Left :
			return LeftAnchors;
		case EPortraitSide::Right :
			return RightAnchors;
		default:
			
			UE_LOG(PortraitSubsystemLog, Warning, TEXT("UPortraitSubsystem::GetPortraitAnchors : PortraitSide is invalid"));
			return CenterAnchors;
	}
}

FVector2D UPortraitSubsystem::GetPortraitAlignment(EPortraitSide PortraitSide) const
{
	switch (PortraitSide)
	{
		case EPortraitSide::Center :
			return CenterAlignment;
		case EPortraitSide::Left :
			return LeftAlignment;
		case EPortraitSide::Right :
			return RightAlignment;
		default:
			
			UE_LOG(PortraitSubsystemLog, Warning, TEXT("UPortraitSubsystem::GetPortraitAnchors : PortraitSide is invalid"));
		return CenterAlignment;
	}
}
