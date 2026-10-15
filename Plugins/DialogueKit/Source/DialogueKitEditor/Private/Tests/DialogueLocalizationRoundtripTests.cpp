#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "DialogueGraph.h"
#include "DialogueGraphCodec.h"
#include "DialogueEdGraphSchema.h"
#include "DialogueEdGraphNodeBase.h"
#include "DialogueFlowEdGraphNode.h"
#include "DialogueChoiceEdGraphNode.h"
#include "DialogueEdEndGraphNode.h"
#include "DialogueChoiceNodeInfo.h"
#include "DialogueNodeInfo.h"
#include "DialogueLocalizationCSVConverter.h"
#include "DialogueLocalizationSubsystem.h"
#include "Engine/AssetManager.h"
#include "Engine/GameInstance.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"
#include "Misc/PackageName.h"
#include "HAL/FileManager.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDialogueLocalizationRoundtripTest,
	"DialogueKit.Localization.CSVStableChoiceRoundtrip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// 새 CSV 왕복과 하위 폴더 검색, 핀 재생성, 구형 4열 CSV 호환을 격리 경로에서 검사한다.
bool FDialogueLocalizationRoundtripTest::RunTest(const FString& Parameters)
{
	const FString Id = FGuid::NewGuid().ToString(EGuidFormats::Digits);
	const FString Mount = TEXT("/DialogueLocalizationTest") + Id + TEXT("/");
	const FString Folder = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("AI/Localization") / Id) + TEXT("/");
	IFileManager::Get().MakeDirectory(*Folder, true);
	FPackageName::RegisterMountPoint(Mount, Folder);
	UDialogueGraph* Asset = NewObject<UDialogueGraph>(GetTransientPackage(), FName(*(TEXT("Graph") + Id)));
	Asset->InitializeBlockIdentity();
	UEdGraph* Graph = NewObject<UEdGraph>(Asset);
	Graph->Schema = UDialogueEdGraphSchema::StaticClass();
	Graph->GetSchema()->CreateDefaultNodesForGraph(*Graph);
	const auto AddNode = [Graph](UClass* Class, UEdGraphPin* From)
	{
		FNewNodeAction Action(Class, FText(), FText(), FText(), 0);
		return CastChecked<UDialogueEdGraphNodeBase>(Action.PerformAction(Graph, From, FVector2D::ZeroVector));
	};
	UDialogueEdGraphNodeBase* Line = AddNode(UDialogueFlowEdGraphNode::StaticClass(), Graph->Nodes[0]->Pins[0]);
	UDialogueEdGraphNodeBase* Choice = AddNode(UDialogueChoiceEdGraphNode::StaticClass(), Line->Pins[1]);
	UDialogueChoiceNodeInfo* Info = CastChecked<UDialogueChoiceNodeInfo>(Choice->GetNodeInfo());
	Info->Options[0].ChoiceId = FGuid::NewGuid();
	Info->Options[0].Text = FText::FromString(TEXT("Localized answer"));
	Choice->OnPropertiesChanged();
	AddNode(UDialogueEdEndGraphNode::StaticClass(), Choice->Pins[1]);
	FindFProperty<FTextProperty>(UDialogueNodeInfo::StaticClass(), TEXT("DialogueText"))->SetPropertyValue_InContainer(Line->GetNodeInfo(), FText::FromString(TEXT("Localized line")));
	FString Error;
	if (!TestTrue(TEXT("Export"), FDialogueGraphCodec::ExportGraph(Graph, Asset, Error))) return false;
	const FString CSVRoot = Folder / TEXT("CSV");
	const FString CSVPath = CSVRoot / TEXT("en-US/Nested/Graph.csv");
	if (!TestTrue(TEXT("Export CSV"), FDialogueLocalizationCSVConverter::ExportDialogueGraphAssetToCSV(Asset, CSVPath, &Error))) return false;
	FDialogueLocalizationCSVConvertOptions Options;
	Options.SourceRootDirectory = CSVRoot;
	Options.DestinationRootPackagePath = Mount + TEXT("Translations");
	Options.bValidateExpectedPrimaryAssetId = true;
	Options.ExpectedPrimaryAssetId = Asset->GetPrimaryAssetId();
	FString AssetPath;
	if (!TestTrue(TEXT("Import CSV"), FDialogueLocalizationCSVConverter::ConvertCSVToDialogueLocalizationDataAsset(CSVPath, Options, &Error, &AssetPath))) return false;
	UDialogueLocalizationDataAsset* Translation = LoadObject<UDialogueLocalizationDataAsset>(nullptr, *(AssetPath + TEXT(".") + FPackageName::GetLongPackageAssetName(AssetPath)));
	if (!TestNotNull(TEXT("Saved translation"), Translation)) return false;
	TestTrue(TEXT("Nested folder retained"), AssetPath.Contains(TEXT("Nested")));
	UAssetManager::Get().ScanPathsForPrimaryAssets(TEXT("DialogueLocalization"), {Options.DestinationRootPackagePath}, UDialogueLocalizationDataAsset::StaticClass(), false);
	ON_SCOPE_EXIT
	{
		UAssetManager::Get().RemoveScanPathsForPrimaryAssets(TEXT("DialogueLocalization"), {Options.DestinationRootPackagePath}, UDialogueLocalizationDataAsset::StaticClass(), false);
		// 엔진의 저장 후 지연 검증이 패키지를 찾도록 고유 마운트는 프로세스 수명까지 유지한다.
	};
	UDialogueLocalizationSubsystem* Localization = NewObject<UDialogueLocalizationSubsystem>(NewObject<UGameInstance>());
	TestTrue(TEXT("Language-only accepted"), Localization->SetCurrentCultureCode(TEXT("en")));
	TestEqual(TEXT("Canonical culture"), Localization->GetCurrentCultureCode(), FString(TEXT("en-US")));
	const FText Fallback = FText::FromString(TEXT("Fallback"));
	TestEqual(TEXT("Nested Line translation"), Localization->ResolveDialogueText(Asset->GetPrimaryAssetId(), Line->NodeGuid, Fallback).ToString(), FString(TEXT("Localized line")));
	TestEqual(TEXT("Choice survives new pin"), Localization->ResolveChoiceText(Asset->GetPrimaryAssetId(), Choice->NodeGuid, Info->Options[0].ChoiceId, FGuid::NewGuid(), Fallback).ToString(), FString(TEXT("Localized answer")));
	const FGuid LegacyPin = FGuid::NewGuid();
	const FString LegacyCSV = FString::Printf(TEXT("NodeGuid,PinId,Key,Value\n,,PrimaryAssetId,%s\n%s,%s,ResponseText,Legacy answer\n"), *Asset->GetPrimaryAssetId().ToString(), *Choice->NodeGuid.ToString(), *LegacyPin.ToString());
	FFileHelper::SaveStringToFile(LegacyCSV, *CSVPath);
	TestTrue(TEXT("Old four-column CSV accepted"), FDialogueLocalizationCSVConverter::ConvertCSVToDialogueLocalizationDataAsset(CSVPath, Options, &Error, &AssetPath));
	Localization->ClearLocalizationCache();
	TestEqual(TEXT("Legacy pin fallback"), Localization->ResolveChoiceText(Asset->GetPrimaryAssetId(), Choice->NodeGuid, Info->Options[0].ChoiceId, LegacyPin, Fallback).ToString(), FString(TEXT("Legacy answer")));
	TestEqual(TEXT("Explicit traditional culture retained"), FDialogueLocalizationUtility::NormalizeCultureCode(TEXT("zh-Hant")), FString(TEXT("zh-Hant")));
	TestFalse(TEXT("Whitespace culture rejected"), Localization->SetCurrentCultureCode(TEXT("  ")));
	return true;
}
#endif
