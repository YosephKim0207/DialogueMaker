// Fill out your copyright notice in the Description page of Project Settings.


#include "DialogueGraph.h"
#include "UObject/AssetRegistryTagsContext.h"
#include "UObject/ObjectSaveContext.h"
#include "Serialization/CustomVersion.h"

DEFINE_LOG_CATEGORY_STATIC(LogDialogueGraph, Log, All);

namespace
{
	const FGuid UnifiedFilterVersion(0x48B54B16, 0x327A4B9D, 0x94DC6EAA, 0x7CA8EF03);
	FCustomVersionRegistration RegisterUnifiedFilterVersion(UnifiedFilterVersion, 1, TEXT("DialogueUnifiedFilter"));
}

// 구형 에셋은 읽기만 허용하고 새 조건이 사라진 채 후보로 실행되지 않게 한다.
void UDialogueGraph::Serialize(FArchive& Ar)
{
	Ar.UsingCustomVersion(UnifiedFilterVersion);
	Super::Serialize(Ar);
	if (Ar.IsLoading() && Ar.IsPersistent() && Ar.CustomVer(UnifiedFilterVersion) < 1)
	{
		bRetiredFilterAsset = true;
	}
}

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
	Context.AddTag(FAssetRegistryTag(TEXT("DialogueKit.BlockId"), BlockId.ToString(EGuidFormats::Digits), FAssetRegistryTag::TT_Hidden));
	Context.AddTag(FAssetRegistryTag(TEXT("DialogueKit.ContentVersion"), FString::FromInt(ContentVersion), FAssetRegistryTag::TT_Hidden));
	Context.AddTag(FAssetRegistryTag(FDialogueBlockMetadata::RegistryKey(), GetSelectionMetadata().ToRegistryValue(), FAssetRegistryTag::TT_Hidden));
	Context.AddTag(FAssetRegistryTag(FDialogueBlockMetadata::TargetRegistryKey(), DialogueTarget.ToString(), FAssetRegistryTag::TT_Hidden));

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

// Block 원본의 값만 복사하며 ID 생성이나 게임 상태 조회는 하지 않는다.
FDialogueBlockMetadata UDialogueGraph::GetSelectionMetadata() const
{
	FDialogueBlockMetadata Metadata;
	Metadata.bRetiredFilterAsset = !UsesUnifiedFilter();
	Metadata.Block = GetBlockReference();
	Metadata.TargetTag = DialogueTarget;
	Metadata.StoryScope = StoryScope;
	Metadata.EntryConditions = EntryConditions;
	Metadata.DialogueGraphType = DialogueGraphType;
	Metadata.Availability = Availability;
	Metadata.bDefault = bDefaultBlock;
	Metadata.bTopPriority = bTopPriorityBlock;
	Metadata.Priority = DialoguePriorityWeight;
	return Metadata;
}

#if WITH_EDITOR
// 새 버전 복제본에 원본의 안정 ID를 부여하며 일반 복제는 새 ID를 계속 사용한다.
bool UDialogueGraph::InitializeContentVersion(const UDialogueGraph* Source, int32 NewVersion)
{
	if (IsValid(Source) == false || Source == this || Source->GetBlockReference().IsValid() == false || NewVersion <= Source->ContentVersion) return false;
	Modify();
	BlockId = Source->BlockId;
	ContentVersion = NewVersion;
	return true;
}
#endif
