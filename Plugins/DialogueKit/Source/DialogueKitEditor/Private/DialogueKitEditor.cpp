// Copyright Epic Games, Inc. All Rights Reserved.

#include "DialogueKitEditor.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "DesktopPlatformModule.h"
#include "DialogueGraph.h"
#include "DialogueGraphEditorCommands.h"
#include "DialogueLocalizationCSVConverter.h"
#include "DialogueLocalizationUtility.h"
#include "EdGraphUtilities.h"
#include "Framework/Application/SlateApplication.h"
#include "HAL/FileManager.h"
#include "IDesktopPlatform.h"
#include "Misc/MessageDialog.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "SGraphPin.h"
#include "Styling/SlateStyleRegistry.h"
#include "ToolMenus.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/Input/STextComboBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SWindow.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "DialogueKitEditorModule"

class SDialogueGraphPin : public SGraphPin
{
public:
	SLATE_BEGIN_ARGS(SDialogueGraphPin) {};
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UEdGraphPin* InGraphPinObj)
	{
		SGraphPin::Construct(SGraphPin::FArguments(), InGraphPinObj);
	};

	protected:
	virtual FSlateColor GetPinColor() const override
	{
		return FSlateColor(FLinearColor(0.2f, 1.0f, 0.2f));
	};
};

class SDialogueStartGraphPin : public SGraphPin
{
public:
	SLATE_BEGIN_ARGS(SDialogueStartGraphPin) {};
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UEdGraphPin* InGraphPinObj)
	{
		SGraphPin::Construct(SGraphPin::FArguments(), InGraphPinObj);
	};

protected:
	virtual FSlateColor GetPinColor() const override
	{
		return FSlateColor(FLinearColor(1.0f, 0.2f, 0.2f));
	};
};


class SDialogueEndGraphPin : public SGraphPin
{
public:
	SLATE_BEGIN_ARGS(SDialogueEndGraphPin) {};
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UEdGraphPin* InGraphPinObj)
	{
		SGraphPin::Construct(SGraphPin::FArguments(), InGraphPinObj);
	};

protected:
	virtual FSlateColor GetPinColor() const override
	{
		return FSlateColor(FLinearColor(0.2f, 0.2f, 1.0f));
	};
};

struct FDialoguePinFactory : FGraphPanelPinFactory
{
	virtual ~FDialoguePinFactory() {};
	virtual TSharedPtr<SGraphPin> CreatePin(UEdGraphPin* Pin) const override
	{
		if (FName(TEXT("DialoguePin")) == Pin->PinType.PinSubCategory)
		{
			return SNew(SDialogueGraphPin, Pin);
		}
		else if (FName(TEXT("StartPin")) == Pin->PinType.PinSubCategory)
		{
			return SNew(SDialogueStartGraphPin, Pin);
		}
		else if (FName(TEXT("EndPin")) == Pin->PinType.PinSubCategory)
		{
			return SNew(SDialogueEndGraphPin, Pin);
		}

		return nullptr;
	};
};

IMPLEMENT_MODULE(FDialogueKitEditorModule, DialogueKitEditor);

// 모듈 시작 시 스타일/커맨드 등록과 함께 상단 메뉴 확장을 등록한다.
void FDialogueKitEditorModule::StartupModule()
{
	StyleSet = MakeShareable(new FSlateStyleSet(TEXT("DialogueKitEditorStyle")));
	// 아이콘이 들어 있는 'AssetIcon' 폴더 Root 지정
	const FString IconRoot = FPaths::Combine(FPaths::ProjectContentDir(), TEXT("Resources/Texture/AssetIcon"));
	StyleSet->SetContentRoot(IconRoot);
	
	FSlateImageBrush* ThumbnailBrush = new FSlateImageBrush(
		StyleSet->RootToContentDir(TEXT("DialogueGraph_128"), TEXT(".png")), FVector2D(128.0f, 128.0f));
	FSlateImageBrush* IconBrush = new FSlateImageBrush(
	StyleSet->RootToContentDir(TEXT("DialogueGraph_128"), TEXT(".png")), FVector2D(128.0f, 128.0f));
	FSlateImageBrush* NodeAddIcon = new FSlateImageBrush(
	StyleSet->RootToContentDir(TEXT("DialogueGraph_128"), TEXT(".png")), FVector2D(128.0f, 128.0f));
	FSlateImageBrush* NodeDeletePinIcon = new FSlateImageBrush(
	StyleSet->RootToContentDir(TEXT("DialogueGraph_128"), TEXT(".png")), FVector2D(128.0f, 128.0f));
	FSlateImageBrush* NodeDeleteNodeIcon = new FSlateImageBrush(
		StyleSet->RootToContentDir(TEXT("DialogueGraph_128"), TEXT(".png")), FVector2D(128.0f, 128.0f));
		
	StyleSet->Set(TEXT("ClassThumbnail.DialogueGraph"),	ThumbnailBrush);
	StyleSet->Set(TEXT("ClassIcon.DialogueGraph"), IconBrush);	
	StyleSet->Set(TEXT("DialogueKitEditor.NodeAddIcon"), NodeAddIcon);
	StyleSet->Set(TEXT("DialogueKitEditor.NodeDeletePinIcon"), NodeDeletePinIcon);
	StyleSet->Set(TEXT("DialogueKitEditor.NodeDeleteNodeIcon"), NodeDeleteNodeIcon);
	
	FSlateStyleRegistry::RegisterSlateStyle(*StyleSet);   // 전역 등록

	PinFactory = MakeShareable(new FDialoguePinFactory);
	FEdGraphUtilities::RegisterVisualPinFactory(PinFactory);
	FDialogueGraphEditorCommands::Register();

	if (UToolMenus::IsToolMenuUIEnabled())
	{
		UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FDialogueKitEditorModule::RegisterMainMenu));
	}
}

// 모듈 종료 시 등록한 메뉴/스타일/커맨드를 정리한다.
void FDialogueKitEditorModule::ShutdownModule()
{
	if (UToolMenus::IsToolMenuUIEnabled())
	{
		UToolMenus::UnRegisterStartupCallback(this);
		UToolMenus::UnregisterOwner(this);
	}

	if (StyleSet.IsValid())
	{
		FSlateStyleRegistry::UnRegisterSlateStyle(*StyleSet);
	}

	if (PinFactory.IsValid())
	{
		FEdGraphUtilities::UnregisterVisualPinFactory(PinFactory);
	}

	FDialogueGraphEditorCommands::Unregister();
}

// 레벨 에디터 상단 메인 메뉴에 Dialogue 하위 메뉴를 등록한다.
void FDialogueKitEditorModule::RegisterMainMenu()
{
	FToolMenuOwnerScoped OwnerScoped(this);
	UToolMenu* MainMenu = UToolMenus::Get()->ExtendMenu(TEXT("LevelEditor.MainMenu"));
	if (MainMenu == nullptr)
	{
		return;
	}

	FToolMenuSection& MainSection = MainMenu->FindOrAddSection(TEXT("DialogueKitMenuSection"));
	MainSection.AddSubMenu(
		TEXT("DialogueKitMenu"),
		LOCTEXT("DialogueMenu_Label", "Dialogue"),
		LOCTEXT("DialogueMenu_Tooltip", "Dialogue 관련 도구"),
		FNewToolMenuDelegate::CreateRaw(this, &FDialogueKitEditorModule::BuildDialogueMainMenu),
		false,
		FSlateIcon());
}

// Dialogue 메뉴 하위에 Make CSV 버튼을 생성한다.
void FDialogueKitEditorModule::BuildDialogueMainMenu(UToolMenu* InMenu)
{
	if (InMenu == nullptr)
	{
		return;
	}

	FToolMenuSection& Section = InMenu->FindOrAddSection(TEXT("DialogueKitCSVSection"));
	Section.AddMenuEntry(
		TEXT("DialogueKitMakeCSV"),
		LOCTEXT("DialogueMakeCSV_Label", "Make CSV"),
		LOCTEXT("DialogueMakeCSV_Tooltip", "선택한 디렉토리의 DialogueGraph 에셋을 일괄 CSV로 변환"),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateRaw(this, &FDialogueKitEditorModule::OnMakeCSVMenuClicked)));

	Section.AddMenuEntry(
		TEXT("DialogueKitMakeLocalizationDataAsset"),
		LOCTEXT("DialogueMakeLocalizationDataAsset_Label", "Make Dialogue Localization DataAsset"),
		LOCTEXT("DialogueMakeLocalizationDataAsset_Tooltip", "선택한 디렉토리의 CSV 파일을 DialogueLocalizationDataAsset으로 일괄 변환"),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateRaw(this, &FDialogueKitEditorModule::OnMakeDialogueLocalizationDataAssetMenuClicked)));
}

// Make CSV 버튼 클릭 시 언어 선택과 디렉토리 선택을 거쳐 일괄 변환을 실행한다.
void FDialogueKitEditorModule::OnMakeCSVMenuClicked()
{
	ELanguage SelectedLanguage = ELanguage::English;
	FString CultureCode;
	if (!PromptLanguageForCSVExport(SelectedLanguage, CultureCode))
	{
		return;
	}

	FString SelectedDirectory;
	if (!OpenDialogueGraphDirectoryDialog(SelectedDirectory))
	{
		return;
	}

	ConvertDialogueGraphsInDirectoryToCSV(SelectedDirectory, CultureCode);
}

// Make Dialogue Localization DataAsset 버튼 클릭 시 CSV 루트 디렉토리를 선택해 일괄 변환을 실행한다.
void FDialogueKitEditorModule::OnMakeDialogueLocalizationDataAssetMenuClicked()
{
	FString SelectedDirectory;
	if (!OpenDialogueCSVDirectoryDialog(SelectedDirectory))
	{
		return;
	}

	ConvertCSVsInDirectoryToDialogueLocalizationDataAssets(SelectedDirectory);
}

// ELanguage 목록을 팝업 콤보박스로 노출하고 선택된 CultureCode를 반환한다.
bool FDialogueKitEditorModule::PromptLanguageForCSVExport(ELanguage& OutLanguage, FString& OutCultureCode) const
{
	struct FLanguageOption
	{
		ELanguage Language = ELanguage::English;
		FString CultureCode;
		FString DisplayText;
	};

	const UEnum* LanguageEnum = StaticEnum<ELanguage>();
	if (LanguageEnum == nullptr)
	{
		return false;
	}

	TArray<FLanguageOption> LanguageOptions;
	for (int32 EnumIndex = 0; EnumIndex < LanguageEnum->NumEnums(); ++EnumIndex)
	{
		if (LanguageEnum->HasMetaData(TEXT("Hidden"), EnumIndex))
		{
			continue;
		}

		const int64 EnumValue = LanguageEnum->GetValueByIndex(EnumIndex);
		if (EnumValue == INDEX_NONE)
		{
			continue;
		}

		const FString EnumName = LanguageEnum->GetNameStringByIndex(EnumIndex);
		if (EnumName.EndsWith(TEXT("_MAX")))
		{
			continue;
		}

		const ELanguage Language = static_cast<ELanguage>(EnumValue);
		const FString CultureCode = FDialogueLocalizationUtility::ToCultureCode(Language);
		if (CultureCode.IsEmpty())
		{
			continue;
		}

		FLanguageOption Option;
		Option.Language = Language;
		Option.CultureCode = CultureCode;
		Option.DisplayText = FString::Printf(TEXT("%s (%s)"), *LanguageEnum->GetDisplayNameTextByValue(EnumValue).ToString(), *CultureCode);
		LanguageOptions.Add(MoveTemp(Option));
	}

	if (LanguageOptions.Num() == 0)
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("DialogueMakeCSV_NoLanguage", "사용 가능한 언어 목록이 없습니다."));
		return false;
	}

	TArray<TSharedPtr<FString>> ComboOptions;
	ComboOptions.Reserve(LanguageOptions.Num());
	for (const FLanguageOption& Option : LanguageOptions)
	{
		ComboOptions.Add(MakeShared<FString>(Option.DisplayText));
	}

	bool bConfirmed = false;
	TSharedPtr<FString> SelectedItem = ComboOptions[0];
	TWeakPtr<SWindow> PickerWindowWeak;

	TSharedRef<SWindow> PickerWindow = SNew(SWindow)
		.Title(LOCTEXT("DialogueMakeCSV_LanguageWindowTitle", "CSV 저장 언어 선택"))
		.ClientSize(FVector2D(420.0f, 130.0f))
		.SupportsMinimize(false)
		.SupportsMaximize(false);
	PickerWindowWeak = PickerWindow;

	PickerWindow->SetContent(
		SNew(SBorder)
		.Padding(12.0f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(STextBlock)
				.Text(LOCTEXT("DialogueMakeCSV_LanguagePrompt", "CSV를 저장할 언어(CultureCode)를 선택하세요."))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 8.0f, 0.0f, 0.0f)
			[
				SNew(STextComboBox)
				.OptionsSource(&ComboOptions)
				.InitiallySelectedItem(SelectedItem)
				.OnSelectionChanged_Lambda([&SelectedItem](TSharedPtr<FString> NewSelection, ESelectInfo::Type)
				{
					if (NewSelection.IsValid())
					{
						SelectedItem = NewSelection;
					}
				})
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Right)
			.Padding(0.0f, 12.0f, 0.0f, 0.0f)
			[
				SNew(SUniformGridPanel)
				.SlotPadding(FMargin(6.0f, 0.0f))
				+ SUniformGridPanel::Slot(0, 0)
				[
					SNew(SButton)
					.Text(LOCTEXT("DialogueMakeCSV_OK", "확인"))
					.OnClicked_Lambda([&bConfirmed, PickerWindowWeak]()
					{
						bConfirmed = true;
						if (const TSharedPtr<SWindow> Window = PickerWindowWeak.Pin())
						{
							Window->RequestDestroyWindow();
						}
						return FReply::Handled();
					})
				]
				+ SUniformGridPanel::Slot(1, 0)
				[
					SNew(SButton)
					.Text(LOCTEXT("DialogueMakeCSV_Cancel", "취소"))
					.OnClicked_Lambda([PickerWindowWeak]()
					{
						if (const TSharedPtr<SWindow> Window = PickerWindowWeak.Pin())
						{
							Window->RequestDestroyWindow();
						}
						return FReply::Handled();
					})
				]
			]
		]);

	FSlateApplication::Get().AddModalWindow(PickerWindow, FSlateApplication::Get().GetActiveTopLevelWindow(), false);
	if (!bConfirmed || !SelectedItem.IsValid())
	{
		return false;
	}

	const int32 SelectedIndex = ComboOptions.IndexOfByPredicate([&SelectedItem](const TSharedPtr<FString>& Item)
	{
		return Item == SelectedItem;
	});

	if (!LanguageOptions.IsValidIndex(SelectedIndex))
	{
		return false;
	}

	OutLanguage = LanguageOptions[SelectedIndex].Language;
	OutCultureCode = LanguageOptions[SelectedIndex].CultureCode;
	return true;
}

// 변환 대상이 될 DialogueGraph 루트 디렉토리를 선택한다.
bool FDialogueKitEditorModule::OpenDialogueGraphDirectoryDialog(FString& OutSelectedDirectory) const
{
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (DesktopPlatform == nullptr)
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("DialogueMakeCSV_NoDesktopPlatform", "디렉토리 선택 기능을 초기화할 수 없습니다."));
		return false;
	}

	const FString DefaultRootDirectory = FPaths::ConvertRelativePathToFull(
		FPaths::Combine(FPaths::ProjectPluginsDir(), TEXT("DialogueKit"), TEXT("Content"), TEXT("Sample"), TEXT("DialogueGraph")));

	const bool bSelected = DesktopPlatform->OpenDirectoryDialog(
		FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr),
		TEXT("Select DialogueGraph Directory"),
		DefaultRootDirectory,
		OutSelectedDirectory);

	return bSelected && !OutSelectedDirectory.IsEmpty();
}

// 변환 대상이 될 CSV 루트 디렉토리를 선택한다.
bool FDialogueKitEditorModule::OpenDialogueCSVDirectoryDialog(FString& OutSelectedDirectory) const
{
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (DesktopPlatform == nullptr)
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("DialogueMakeLocalization_NoDesktopPlatform", "디렉토리 선택 기능을 초기화할 수 없습니다."));
		return false;
	}

	const FString DefaultRootDirectory = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("DialogueCSV")));
	const bool bSelected = DesktopPlatform->OpenDirectoryDialog(
		FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr),
		TEXT("Select Dialogue CSV Directory"),
		DefaultRootDirectory,
		OutSelectedDirectory);

	return bSelected && !OutSelectedDirectory.IsEmpty();
}

// 선택 디렉토리 하위의 DialogueGraph 에셋을 재귀 탐색해 지정 CultureCode 디렉토리로 CSV를 저장한다.
bool FDialogueKitEditorModule::ConvertDialogueGraphsInDirectoryToCSV(const FString& SelectedDirectory, const FString& CultureCode) const
{
	FString SelectedDirectoryAbs = FPaths::ConvertRelativePathToFull(SelectedDirectory);
	FPaths::NormalizeDirectoryName(SelectedDirectoryAbs);

	FString RootPackagePath;
	if (!FPackageName::TryConvertFilenameToLongPackageName(SelectedDirectoryAbs, RootPackagePath))
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("DialogueMakeCSV_PathConvertFail", "선택한 디렉토리를 에셋 경로로 변환할 수 없습니다."));
		return false;
	}

	IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();

	FARFilter Filter;
	Filter.ClassPaths.Add(UDialogueGraph::StaticClass()->GetClassPathName());
	Filter.PackagePaths.Add(*RootPackagePath);
	Filter.bRecursivePaths = true;
	Filter.bRecursiveClasses = true;

	TArray<FAssetData> AssetDatas;
	AssetRegistry.GetAssets(Filter, AssetDatas);
	if (AssetDatas.Num() == 0)
	{
		FMessageDialog::Open(
			EAppMsgType::Ok,
			FText::Format(LOCTEXT("DialogueMakeCSV_NoAssets", "선택한 경로에서 DialogueGraph 에셋을 찾지 못했습니다.\n{0}"), FText::FromString(RootPackagePath)));
		return false;
	}

	const FString OutputRootDirectory = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("DialogueCSV"), CultureCode);
	IFileManager::Get().MakeDirectory(*OutputRootDirectory, true);

	int32 SuccessCount = 0;
	int32 FailCount = 0;
	FString ResultLog;
	for (const FAssetData& AssetData : AssetDatas)
	{
		const FString AssetObjectPath = AssetData.GetSoftObjectPath().ToString();
		UDialogueGraph* DialogueGraph = Cast<UDialogueGraph>(AssetData.GetAsset());
		if (DialogueGraph == nullptr)
		{
			ResultLog += FString::Printf(TEXT("[FAIL] %s - 에셋 로드 실패%s"), *AssetObjectPath, LINE_TERMINATOR);
			++FailCount;
			continue;
		}

		const FString AssetPackagePath = AssetData.PackagePath.ToString();
		FString RelativePath = AssetPackagePath;
		if (RelativePath.StartsWith(RootPackagePath))
		{
			RelativePath.RightChopInline(RootPackagePath.Len(), false);
			RelativePath.RemoveFromStart(TEXT("/"));
		}

		FString OutputDirectory = OutputRootDirectory;
		if (!RelativePath.IsEmpty())
		{
			TArray<FString> PackagePathSegments;
			RelativePath.ParseIntoArray(PackagePathSegments, TEXT("/"), true);
			for (const FString& Segment : PackagePathSegments)
			{
				OutputDirectory = FPaths::Combine(OutputDirectory, Segment);
			}
		}

		IFileManager::Get().MakeDirectory(*OutputDirectory, true);

		const FString OutputCSVPath = FPaths::Combine(OutputDirectory, FString::Printf(TEXT("%s.csv"), *AssetData.AssetName.ToString()));
		FString ExportErrorMessage;
		if (FDialogueLocalizationCSVConverter::ExportDialogueGraphAssetToCSV(DialogueGraph, OutputCSVPath, &ExportErrorMessage))
		{
			ResultLog += FString::Printf(TEXT("[OK] %s -> %s%s"), *AssetObjectPath, *OutputCSVPath, LINE_TERMINATOR);
			++SuccessCount;
		}
		else
		{
			ResultLog += FString::Printf(TEXT("[FAIL] %s - %s%s"), *AssetObjectPath, *ExportErrorMessage, LINE_TERMINATOR);
			++FailCount;
		}
	}

	ShowCSVBatchResultWindow(ResultLog, SuccessCount, FailCount, OutputRootDirectory);

	return FailCount == 0;
}

// Make CSV 배치 변환의 상세 결과를 별도 로그 창으로 표시한다.
void FDialogueKitEditorModule::ShowCSVBatchResultWindow(const FString& ResultLog, int32 SuccessCount, int32 FailCount, const FString& OutputRootDirectory) const
{
	const FString Summary = FString::Printf(
		TEXT("Make CSV 완료\n성공: %d\n실패: %d\n저장 경로: %s"),
		SuccessCount,
		FailCount,
		*OutputRootDirectory);

	const FString LogBody = ResultLog.IsEmpty()
		? FString::Printf(TEXT("처리 결과가 없습니다.%s"), LINE_TERMINATOR)
		: ResultLog;

	TWeakPtr<SWindow> ResultWindowWeak;
	TSharedRef<SWindow> ResultWindow = SNew(SWindow)
		.Title(LOCTEXT("DialogueMakeCSV_ResultWindowTitle", "Make CSV 결과"))
		.ClientSize(FVector2D(980.0f, 640.0f))
		.SupportsMinimize(true)
		.SupportsMaximize(true);
	ResultWindowWeak = ResultWindow;

	ResultWindow->SetContent(
		SNew(SBorder)
		.Padding(12.0f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(STextBlock)
				.Text(FText::FromString(Summary))
			]
			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			.Padding(0.0f, 8.0f, 0.0f, 0.0f)
			[
				SNew(SMultiLineEditableTextBox)
				.IsReadOnly(true)
				.AutoWrapText(false)
				.Text(FText::FromString(LogBody))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Right)
			.Padding(0.0f, 8.0f, 0.0f, 0.0f)
			[
				SNew(SButton)
				.Text(LOCTEXT("DialogueMakeCSV_ResultWindowClose", "닫기"))
				.OnClicked_Lambda([ResultWindowWeak]()
				{
					if (const TSharedPtr<SWindow> Window = ResultWindowWeak.Pin())
					{
						Window->RequestDestroyWindow();
					}
					return FReply::Handled();
				})
			]
		]);

	FSlateApplication::Get().AddWindow(ResultWindow);
}

// 선택 디렉토리 하위 CSV 파일을 재귀 탐색해 DialogueLocalizationDataAsset으로 일괄 변환한다.
bool FDialogueKitEditorModule::ConvertCSVsInDirectoryToDialogueLocalizationDataAssets(const FString& SelectedDirectory) const
{
	FString SelectedDirectoryAbs = FPaths::ConvertRelativePathToFull(SelectedDirectory);
	FPaths::NormalizeDirectoryName(SelectedDirectoryAbs);

	TArray<FString> CSVFilePaths;
	IFileManager::Get().FindFilesRecursive(CSVFilePaths, *SelectedDirectoryAbs, TEXT("*.csv"), true, false, false);
	if (CSVFilePaths.Num() == 0)
	{
		FMessageDialog::Open(
			EAppMsgType::Ok,
			FText::Format(
				LOCTEXT("DialogueMakeLocalization_NoCSVFiles", "선택한 경로에서 CSV 파일을 찾지 못했습니다.\n{0}"),
				FText::FromString(SelectedDirectoryAbs)));
		return false;
	}

	const FString OutputRootDirectory = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectContentDir(), TEXT("DialogueLocalization")));
	IFileManager::Get().MakeDirectory(*OutputRootDirectory, true);

	FDialogueLocalizationCSVConvertOptions ConvertOptions;
	ConvertOptions.SourceRootDirectory = SelectedDirectoryAbs;
	ConvertOptions.DestinationRootPackagePath = TEXT("/Game/DialogueLocalization");
	ConvertOptions.bUseUnderscoreCultureFolder = true;
	ConvertOptions.bValidateExpectedPrimaryAssetId = false;

	int32 SuccessCount = 0;
	int32 FailCount = 0;
	FString ResultLog;
	for (const FString& CSVFilePath : CSVFilePaths)
	{
		FString ErrorMessage;
		FString SavedAssetPath;
		if (FDialogueLocalizationCSVConverter::ConvertCSVToDialogueLocalizationDataAsset(CSVFilePath, ConvertOptions, &ErrorMessage, &SavedAssetPath))
		{
			ResultLog += FString::Printf(TEXT("[OK] %s -> %s%s"), *CSVFilePath, *SavedAssetPath, LINE_TERMINATOR);
			++SuccessCount;
		}
		else
		{
			ResultLog += FString::Printf(TEXT("[FAIL] %s - %s%s"), *CSVFilePath, *ErrorMessage, LINE_TERMINATOR);
			++FailCount;
		}
	}

	ShowDialogueLocalizationBatchResultWindow(ResultLog, SuccessCount, FailCount, OutputRootDirectory);
	return FailCount == 0;
}

// Make Dialogue Localization DataAsset 배치 변환 결과를 별도 로그 창으로 표시한다.
void FDialogueKitEditorModule::ShowDialogueLocalizationBatchResultWindow(const FString& ResultLog, int32 SuccessCount, int32 FailCount, const FString& OutputRootDirectory) const
{
	const FString Summary = FString::Printf(
		TEXT("Make Dialogue Localization DataAsset 완료\n성공: %d\n실패: %d\n저장 경로: %s"),
		SuccessCount,
		FailCount,
		*OutputRootDirectory);

	const FString LogBody = ResultLog.IsEmpty()
		? FString::Printf(TEXT("처리 결과가 없습니다.%s"), LINE_TERMINATOR)
		: ResultLog;

	TWeakPtr<SWindow> ResultWindowWeak;
	TSharedRef<SWindow> ResultWindow = SNew(SWindow)
		.Title(LOCTEXT("DialogueMakeLocalization_ResultWindowTitle", "Make Dialogue Localization DataAsset 결과"))
		.ClientSize(FVector2D(1080.0f, 680.0f))
		.SupportsMinimize(true)
		.SupportsMaximize(true);
	ResultWindowWeak = ResultWindow;

	ResultWindow->SetContent(
		SNew(SBorder)
		.Padding(12.0f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(STextBlock)
				.Text(FText::FromString(Summary))
			]
			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			.Padding(0.0f, 8.0f, 0.0f, 0.0f)
			[
				SNew(SMultiLineEditableTextBox)
				.IsReadOnly(true)
				.AutoWrapText(false)
				.Text(FText::FromString(LogBody))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Right)
			.Padding(0.0f, 8.0f, 0.0f, 0.0f)
			[
				SNew(SButton)
				.Text(LOCTEXT("DialogueMakeLocalization_ResultWindowClose", "닫기"))
				.OnClicked_Lambda([ResultWindowWeak]()
				{
					if (const TSharedPtr<SWindow> Window = ResultWindowWeak.Pin())
					{
						Window->RequestDestroyWindow();
					}
					return FReply::Handled();
				})
			]
		]);

	FSlateApplication::Get().AddWindow(ResultWindow);
}

#undef LOCTEXT_NAMESPACE
