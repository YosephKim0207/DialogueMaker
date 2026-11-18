#include "DialogueCreateDefinitionSamplesCommandlet.h"
#include "Samples/DialogueSampleDefinitions.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "HAL/FileManager.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"

// 렌더링/게임 실행 없이 명시적으로 샘플 패키지만 생성한다.
UDialogueCreateDefinitionSamplesCommandlet::UDialogueCreateDefinitionSamplesCommandlet()
{
	IsClient = false;
	IsServer = false;
	IsEditor = true;
	LogToConsole = true;
}

// 모든 대상의 충돌과 새 자료를 검증한 뒤 신규 파일만 저장한다. 실패한 부분 결과도 삭제하지 않는다.
int32 UDialogueCreateDefinitionSamplesCommandlet::Main(const FString& Params)
{
	// Params에는 엔진의 unattended/NullRHI 옵션도 들어올 수 있다. 별도 덮어쓰기 옵션은 제공하지 않는다.
	const FString Root = TEXT("/DialogueKit/Sample/GameDefinitions/");
	const TArray<FString> Names = {TEXT("DA_SampleNPC_Merchant"), TEXT("DA_SampleNPC_Guard"),
		TEXT("DT_SampleItems"), TEXT("DT_SampleSkills"), TEXT("DA_SampleDefinitionCatalog")};
	TArray<FString> Filenames;
	for (const FString& Name : Names)
	{
		const FString PackageName = Root + Name;
		FString Filename;
		if (FPackageName::TryConvertLongPackageNameToFilename(PackageName, Filename, FPackageName::GetAssetPackageExtension()) == false ||
			FPackageName::DoesPackageExist(PackageName) || FindPackage(nullptr, *PackageName) || IFileManager::Get().FileExists(*Filename))
		{
			UE_LOG(LogTemp, Error, TEXT("대상 경로를 사용할 수 없거나 이미 존재합니다. 아무 파일도 덮어쓰지 않습니다: %s"), *PackageName);
			return 1;
		}
		Filenames.Add(Filename);
	}
	TArray<UObject*> Assets;
	// 아래 ID와 수치는 샘플일 뿐 실제 게임의 기획·저장 규약을 정의하지 않는다.
	auto* Merchant = NewObject<UDialogueSampleNpcDefinition>(CreatePackage(*(Root + Names[0])), *Names[0], RF_Public | RF_Standalone);
	Merchant->Id = TEXT("NPC.Merchant");
	Merchant->DisplayName = NSLOCTEXT("DialogueKitSample", "MerchantName", "샘플 상인");
	Merchant->Description = NSLOCTEXT("DialogueKitSample", "MerchantDescription", "NPC 정의 에셋 예시입니다. 현재 호감도는 게임 상태에서 별도로 조회합니다. Portrait는 게임 이미지로 연결할 선택적 참조입니다.");
	Assets.Add(Merchant);
	auto* Guard = NewObject<UDialogueSampleNpcDefinition>(CreatePackage(*(Root + Names[1])), *Names[1], RF_Public | RF_Standalone);
	Guard->Id = TEXT("NPC.Guard");
	Guard->DisplayName = NSLOCTEXT("DialogueKitSample", "GuardName", "샘플 경비병");
	Guard->Description = NSLOCTEXT("DialogueKitSample", "GuardDescription", "동일한 Affection 항목도 NPC ID에 따라 다른 상태가 됨을 보여 주는 두 번째 인물입니다.");
	Assets.Add(Guard);

	auto* Items = NewObject<UDataTable>(CreatePackage(*(Root + Names[2])), *Names[2], RF_Public | RF_Standalone);
	Items->RowStruct = FDialogueSampleItemRow::StaticStruct();
	auto AddItem = [Items](const TCHAR* RowName, const TCHAR* Id, const FText& Name, int32 MaxStack)
	{
		FDialogueSampleItemRow Row;
		Row.Id = Id;
		Row.DisplayName = Name;
		Row.MaxStack = MaxStack;
		Row.Description = NSLOCTEXT("DialogueKitSample", "ItemDescription", "게임 정의 테이블 예시입니다. Id가 상태 키이며 RowName은 편집용 이름입니다. MaxStack은 현재 보유량이 아닙니다. 사용 효과는 구현하지 않습니다.");
		Items->AddRow(FName(RowName), Row);
	};
	AddItem(TEXT("HealthPotion"), TEXT("Item.HealthPotion"), NSLOCTEXT("DialogueKitSample", "HealthPotion", "샘플 회복 물약"), 99);
	AddItem(TEXT("ManaPotion"), TEXT("Item.ManaPotion"), NSLOCTEXT("DialogueKitSample", "ManaPotion", "샘플 마력 물약"), 99);
	AddItem(TEXT("IronOre"), TEXT("Item.IronOre"), NSLOCTEXT("DialogueKitSample", "IronOre", "샘플 철광석"), 999);
	AddItem(TEXT("QuestLetter"), TEXT("Item.QuestLetter"), NSLOCTEXT("DialogueKitSample", "QuestLetter", "샘플 편지"), 1);
	AddItem(TEXT("IronSword"), TEXT("Item.IronSword"), NSLOCTEXT("DialogueKitSample", "IronSword", "샘플 철검"), 1);
	AddItem(TEXT("Gift"), TEXT("Item.Gift"), NSLOCTEXT("DialogueKitSample", "Gift", "샘플 선물"), 20);
	Assets.Add(Items);

	auto* Skills = NewObject<UDataTable>(CreatePackage(*(Root + Names[3])), *Names[3], RF_Public | RF_Standalone);
	Skills->RowStruct = FDialogueSampleSkillRow::StaticStruct();
	auto AddSkill = [Skills](const TCHAR* RowName, const TCHAR* Id, const FText& Name, int32 MaxLevel, float Cooldown)
	{
		FDialogueSampleSkillRow Row;
		Row.Id = Id;
		Row.DisplayName = Name;
		Row.MaxLevel = MaxLevel;
		Row.CooldownSeconds = Cooldown;
		Row.Description = NSLOCTEXT("DialogueKitSample", "SkillDescription", "스킬 정의와 현재 레벨을 분리하는 표 예시입니다. 숫자는 샘플이며 동작·습득·성장 규칙은 게임이 구현합니다. 최대 레벨을 현재 레벨로 사용하지 않습니다.");
		Skills->AddRow(FName(RowName), Row);
	};
	AddSkill(TEXT("Firebolt"), TEXT("Skill.Firebolt"), NSLOCTEXT("DialogueKitSample", "Firebolt", "샘플 화염탄"), 5, 2.0f);
	AddSkill(TEXT("Icebolt"), TEXT("Skill.Icebolt"), NSLOCTEXT("DialogueKitSample", "Icebolt", "샘플 얼음탄"), 5, 3.0f);
	AddSkill(TEXT("Dash"), TEXT("Skill.Dash"), NSLOCTEXT("DialogueKitSample", "Dash", "샘플 돌진"), 3, 4.0f);
	AddSkill(TEXT("Shield"), TEXT("Skill.Shield"), NSLOCTEXT("DialogueKitSample", "Shield", "샘플 보호막"), 3, 8.0f);
	AddSkill(TEXT("Summon"), TEXT("Skill.Summon"), NSLOCTEXT("DialogueKitSample", "Summon", "샘플 소환"), 5, 10.0f);
	AddSkill(TEXT("Focus"), TEXT("Skill.Focus"), NSLOCTEXT("DialogueKitSample", "Focus", "샘플 집중"), 1, 0.0f);
	Assets.Add(Skills);

	auto* Catalog = NewObject<UDialogueSampleDefinitionCatalog>(CreatePackage(*(Root + Names[4])), *Names[4], RF_Public | RF_Standalone);
	Catalog->Npcs.Add(Merchant);
	Catalog->Npcs.Add(Guard);
	Catalog->Items = Items;
	Catalog->Skills = Skills;
	Assets.Add(Catalog);
	TArray<FDialogueStateDefinition> Definitions;
	FString Error;
	if (Catalog->BuildDefinitions(Definitions, Error) == false || Definitions.Num() != 14)
	{
		UE_LOG(LogTemp, Error, TEXT("생성 전 샘플 검증 실패: %s"), *Error);
		return 1;
	}
	for (int32 Index = 0; Index < Assets.Num(); ++Index)
	{
		if (IFileManager::Get().MakeDirectory(*FPaths::GetPath(Filenames[Index]), true) == false)
		{
			UE_LOG(LogTemp, Error, TEXT("샘플 폴더 생성 실패: %s"), *Filenames[Index]);
			return 1;
		}
		// 사전 검사 후 외부에서 파일이 생성된 경우에도 덮어쓰지 않는다.
		if (IFileManager::Get().FileExists(*Filenames[Index]))
		{
			UE_LOG(LogTemp, Error, TEXT("샘플 저장 직전 대상 충돌: %s"), *Filenames[Index]);
			return 1;
		}
		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
		SaveArgs.SaveFlags = SAVE_NoError;
		if (UPackage::SavePackage(Assets[Index]->GetOutermost(), Assets[Index], *Filenames[Index], SaveArgs) == false)
		{
			UE_LOG(LogTemp, Error, TEXT("샘플 저장 실패. 이미 생성된 파일은 보존합니다: %s"), *Filenames[Index]);
			return 1;
		}
		FAssetRegistryModule::AssetCreated(Assets[Index]);
		UE_LOG(LogTemp, Display, TEXT("샘플 생성 완료: %s"), *Assets[Index]->GetPathName());
	}
	UE_LOG(LogTemp, Display, TEXT("샘플 에셋 5개 및 상태 정의 14개 생성 완료. 기존 게임/설정은 변경하지 않았습니다."));
	return 0;
}
