// Fill out your copyright notice in the Description page of Project Settings.


#include "DialogueGraph.h"
#include "GameplayTags.h"
#include "UObject/AssetRegistryTagsContext.h"
#include "UObject/ObjectSaveContext.h"

DEFINE_LOG_CATEGORY_STATIC(LogDialogueGraph, Log, All);

// FString UDialogueGraph::GetSpeakerName() const
// {
// 	// if (SpeakerName.IsEmpty() || FString::Find(TEXT(" ")))
// 	// {
// 	// 	UE_LOG(LogDialogueGraph, Error, TEXT("UDialogueGraph::GetSpeakerName : Please Enter Correct Speaker Name in Graph Editor Property!!!"));
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
	Super::PreSave(SaveContext);
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
	UE_LOG(LogDialogueGraph, Display, TEXT("UDialogueGraph::GetAssetRegistryTags : Enter"));

	Super::GetAssetRegistryTags(Context);

	Context.AddTag(FAssetRegistryTag(*FString::Printf(TEXT("%s"), *GameplayTags::Required_All_Tags.GetTag().ToString()),RequiredAllTags.ToString(), FAssetRegistryTag::TT_Hidden));
	Context.AddTag(FAssetRegistryTag(*FString::Printf(TEXT("%s"), *GameplayTags::Required_Any_Tags.GetTag().ToString()), RequiredAnyTags.ToString(), FAssetRegistryTag::TT_Hidden));
	Context.AddTag(FAssetRegistryTag(*FString::Printf(TEXT("%s"), *GameplayTags::Blocked_Any_Tags.GetTag().ToString()), BlockedAnyTags.ToString(), FAssetRegistryTag::TT_Hidden));
}

// C++ 생성 도구와 Factory에서만 명시적으로 호출하여 기존 에셋을 조용히 변경하지 않는다.
bool UDialogueGraph::InitializeBlockIdentity()
{
    if (IsTemplate() || BlockId.IsValid())
    {
        return false;
    }
    BlockId = FGuid::NewGuid();
    ContentVersion = 1;
    return true;
}

// 조회는 ID 생성이나 상태 변경을 수행하지 않는다.
FDialogueBlockReference UDialogueGraph::GetBlockReference() const
{
    FDialogueBlockReference Reference;
    Reference.BlockId = BlockId;
    Reference.ContentVersion = ContentVersion;
    return Reference;
}

// 콘텐츠 브라우저의 일반 복제본은 새 Block이며 실행용 복제는 원본 정체성을 유지한다.
void UDialogueGraph::PostDuplicate(EDuplicateMode::Type DuplicateMode)
{
    Super::PostDuplicate(DuplicateMode);
    if (DuplicateMode == EDuplicateMode::Normal && !IsTemplate())
    {
        BlockId = FGuid::NewGuid();
        ContentVersion = 1;
    }
}
