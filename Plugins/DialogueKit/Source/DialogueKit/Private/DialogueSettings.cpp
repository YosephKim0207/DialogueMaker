// Fill out your copyright notice in the Description page of Project Settings.


#include "DialogueSettings.h"
#include "Engine/AssetManager.h"

DEFINE_LOG_CATEGORY_STATIC(DialogueSettingsLog, Log, All);

void UDialogueSettings::ApplyKitAssetToAssetManager() const
{
	UE_LOG(DialogueSettingsLog, Display, TEXT("UDialogueSettings::ApplyKitAssetToAssetManager : Enter"));

	UAssetManager& AssetManager = UAssetManager::Get();
	if (!AssetManager.IsInitialized())
	{
		UE_LOG(DialogueSettingsLog, Error, TEXT("UDialogueSettings::ApplyKitAssetToAssetManager : AssetManager is not Initialized"));
		return;
	}

	for (const FDialogueKitScanType& Entry : ScanTypes)
	{
		if (!Entry.PrimaryAssetType.IsNone() && Entry.AssetBaseClass.IsValid())
		{
			UClass* BaseClass = Entry.AssetBaseClass.TryLoadClass<UObject>();
			if (!BaseClass)
			{
				continue;
			}

			TArray<FString> Paths;
			Paths.Reserve(Entry.Directories.Num());
			for (const FDirectoryPath& Dir : Entry.Directories)
			{
				if (!Dir.Path.IsEmpty())
				{
					Paths.Add(Dir.Path);
				}
			}

			if (Paths.Num() > 0)
			{
				const int32 NumFound = AssetManager.ScanPathsForPrimaryAssets(
					FPrimaryAssetType(Entry.PrimaryAssetType),
					Paths,
					BaseClass,
					Entry.bHasBlueprintClasses,
					Entry.bIsEditorOnly,
					true
				);
				
				if (!Entry.Rules.IsDefault())
				{
					TArray<FPrimaryAssetId> Ids;
					AssetManager.GetPrimaryAssetIdList(FPrimaryAssetType(Entry.PrimaryAssetType), Ids);
					for (const FPrimaryAssetId& Id : Ids)
					{
						AssetManager.SetPrimaryAssetRules(Id, Entry.Rules);
					}
				}

				UE_LOG(DialogueSettingsLog, Display, TEXT("UDialogueSettings::ApplyKitAssetToAssetManager : [DialogueKit] Scanned %d primary assets of type %s"),
					NumFound, *Entry.PrimaryAssetType.ToString());
			}
		}
	}
}