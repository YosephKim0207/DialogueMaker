#include "DialogueLocalizationDataAsset.h"

FPrimaryAssetId UDialogueLocalizationDataAsset::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("DialogueLocalization"), GetFName());
}
