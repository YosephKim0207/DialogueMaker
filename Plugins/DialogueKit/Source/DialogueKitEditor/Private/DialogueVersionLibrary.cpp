#include "DialogueVersionLibrary.h"
#include "DialogueBlockLoader.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"

// 원본과 기존 목적지를 변경하지 않고 별도 패키지에 버전을 복제한다. 저장은 에디터 사용자 책임이다.
UDialogueGraph* UDialogueVersionLibrary::CreateContentVersion(UDialogueGraph* Source, const FString& PackageName, FName AssetName, int32 NewVersion, FName& OutError)
{
	OutError = TEXT("InvalidVersionDestination");
	if (IsValid(Source) == false || Source->GetBlockReference().IsValid() == false || NewVersion <= Source->GetBlockReference().ContentVersion ||
		FPackageName::IsValidLongPackageName(PackageName) == false || AssetName.IsNone() || FPackageName::GetLongPackageAssetName(PackageName) != AssetName.ToString() ||
		FPackageName::DoesPackageExist(PackageName) || FindPackage(nullptr, *PackageName)) return nullptr;
	FDialogueBlockReference Reference = Source->GetBlockReference();
	Reference.ContentVersion = NewVersion;
	FSoftObjectPath Existing;
	FName LookupError;
	if (UDialogueBlockLoader::FindExactReference(Reference, Existing, LookupError) || LookupError == TEXT("AmbiguousRestoreReference"))
	{
		OutError = TEXT("ContentVersionAlreadyExists");
		return nullptr;
	}
	UPackage* Package = CreatePackage(*PackageName);
	UDialogueGraph* Created = DuplicateObject<UDialogueGraph>(Source, Package, AssetName);
	if (Created == nullptr || Created->InitializeContentVersion(Source, NewVersion) == false) return nullptr;
	Created->SetFlags(RF_Public | RF_Standalone | RF_Transactional);
	Created->Availability = EDialogueBlockAvailability::Active;
	FAssetRegistryModule::AssetCreated(Created);
	Created->MarkPackageDirty();
	OutError = NAME_None;
	return Created;
}
