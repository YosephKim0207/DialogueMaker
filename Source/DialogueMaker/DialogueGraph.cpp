// Fill out your copyright notice in the Description page of Project Settings.


#include "DialogueGraph.h"
#include "GameplayTags.h"
#include "UObject/AssetRegistryTagsContext.h"
#include "UObject/ObjectSaveContext.h"

// FString UDialogueGraph::GetSpeakerName() const
// {
// 	// if (SpeakerName.IsEmpty() || FString::Find(TEXT(" ")))
// 	// {
// 	// 	UE_LOG(LogTemp, Error, TEXT("UDialogueGraph::GetSpeakerName : Please Enter Correct Speaker Name in Graph Editor Property!!!"));
// 	// 	return FString("SpeakerName");
// 	// }
// 	
// 	return SpeakerName;
// }

void UDialogueGraph::SetPreSaveListener(std::function<void()> NewOnPreSaveListener)
{
	OnPreSaveListener = NewOnPreSaveListener;
}

void UDialogueGraph::PreSave(FObjectPreSaveContext SaveContext)
{
	if (OnPreSaveListener != nullptr)
	{
		OnPreSaveListener();
	}

}

FPrimaryAssetId UDialogueGraph::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("Dialogue"), GetFName());
}

void UDialogueGraph::GetAssetRegistryTags(FAssetRegistryTagsContext Context) const
{
	UE_LOG(LogTemp, Display, TEXT("UDialogueGraph::GetAssetRegistryTags : Enter"));

	Super::GetAssetRegistryTags(Context);

	Context.AddTag(FAssetRegistryTag(*FString::Printf(TEXT("%s"), *GameplayTags::Required_All_Tags.GetTag().ToString()),RequiredAllTags.ToString(), FAssetRegistryTag::TT_Hidden));
	Context.AddTag(FAssetRegistryTag(*FString::Printf(TEXT("%s"), *GameplayTags::Required_Any_Tags.GetTag().ToString()), RequiredAnyTags.ToString(), FAssetRegistryTag::TT_Hidden));
	Context.AddTag(FAssetRegistryTag(*FString::Printf(TEXT("%s"), *GameplayTags::Blocked_Any_Tags.GetTag().ToString()), BlockedAnyTags.ToString(), FAssetRegistryTag::TT_Hidden));
}
