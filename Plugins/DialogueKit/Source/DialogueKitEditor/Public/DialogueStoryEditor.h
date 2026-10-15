#pragma once
#include "CoreMinimal.h"
#include "AssetRegistry/AssetData.h"
#include "DialogueBlockMetadata.h"
class UDialogueStoryDefinition;
class UDialogueGraph;

struct FDialogueStoryBlock
{
	FAssetData Asset;
	FDialogueBlockMetadata Metadata;
	bool bMetadataReadable = false;
};
class FDialogueStoryEditor
{
public:
	static void Open();
	static TArray<FDialogueStoryBlock> CollectBlocks();
	static bool IsUnassigned(const FDialogueStoryScope& Scope, const UDialogueStoryDefinition* Definition);
	static bool RemoveChapter(UDialogueStoryDefinition* Definition, FGuid Id);
	static bool RemoveScene(UDialogueStoryDefinition* Definition, FGuid Id);
	static bool MoveScene(UDialogueStoryDefinition* Definition, FGuid Scene, FGuid Chapter);
	static bool MoveBlocks(UDialogueStoryDefinition* Definition, const TArray<FAssetData>& Assets, const FDialogueStoryScope& Scope);
	// UI가 정확한 선택 집합의 미출시 여부와 두 확인을 받은 뒤만 호출한다.
	static bool DeleteUnassigned(UDialogueStoryDefinition* Definition, const TArray<FAssetData>& Assets, FText& Result);
};
