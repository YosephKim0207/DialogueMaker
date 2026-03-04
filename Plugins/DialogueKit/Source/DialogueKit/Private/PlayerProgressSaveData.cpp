// Fill out your copyright notice in the Description page of Project Settings.


#include "PlayerProgressSaveData.h"

const FGameplayTagContainer& UPlayerProgressSaveData::GetAllTags() const
{
	return PlayersTagContainer; 
}

bool UPlayerProgressSaveData::HasTag(const FGameplayTag& Tag) const
{
	return PlayersTagContainer.HasTag(Tag);
}

void UPlayerProgressSaveData::AddTag(const FGameplayTag& NewGameplayTag)
{
	PlayersTagContainer.AddTag(NewGameplayTag);
}

void UPlayerProgressSaveData::AddTags(const FGameplayTagContainer& NewGameplayTagContainer)
{
	for (const FGameplayTag& Tag : NewGameplayTagContainer)
	{
		PlayersTagContainer.AddTag(Tag);
	}
}

void UPlayerProgressSaveData::RemoveTag(const FGameplayTag& RemoveTargetGameplayTag)
{
	PlayersTagContainer.RemoveTag(RemoveTargetGameplayTag);
}

void UPlayerProgressSaveData::RemoveTags(const FGameplayTagContainer& RemoveTargetGameplayTagContainer)
{
	for (const FGameplayTag& Tag : RemoveTargetGameplayTagContainer)
	{
		PlayersTagContainer.RemoveTag(Tag);
	}
}

int32 UPlayerProgressSaveData::GetPlayerLevel() const
{
	return PlayerLevel;
}

void UPlayerProgressSaveData::SetPlayerLevel(int32 NewPlayerLevel)
{
	PlayerLevel = NewPlayerLevel;
}

EChapterID UPlayerProgressSaveData::GetCurrentChapter() const
{
	return CurrentChapter;
}
