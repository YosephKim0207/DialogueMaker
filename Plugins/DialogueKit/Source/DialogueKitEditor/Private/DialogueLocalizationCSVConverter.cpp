#include "DialogueLocalizationCSVConverter.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "DialogueGraph.h"
#include "DialogueLocalizationDataAsset.h"
#include "DialogueLocalizationUtility.h"
#include "DialogueNodeInfo.h"
#include "DialogueRuntimeGraph.h"
#include "HAL/FileManager.h"
#include "Internationalization/Culture.h"
#include "Internationalization/Internationalization.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "UObject/SavePackage.h"

namespace
{
// 오류 메시지를 설정하고 실패를 반환한다.
bool Fail(FString* OutErrorMessage, const FString& ErrorMessage)
{
	if (OutErrorMessage != nullptr)
	{
		*OutErrorMessage = ErrorMessage;
	}
	return false;
}

// CSV 셀 값에 포함된 특수 문자를 이스케이프한다.
FString CSVEscape(const FString& Input)
{
	FString Escaped = Input;
	Escaped.ReplaceInline(TEXT("\""), TEXT("\"\""));

	if (Escaped.Contains(TEXT(",")) || Escaped.Contains(TEXT("\"")) || Escaped.Contains(TEXT("\n")) || Escaped.Contains(TEXT("\r")))
	{
		return FString::Printf(TEXT("\"%s\""), *Escaped);
	}

	return Escaped;
}

// CSV 한 줄을 결과 문자열에 추가한다.
void AppendCSVRow(FString& OutCSV, const TArray<FString>& Columns)
{
	TArray<FString> EscapedColumns;
	EscapedColumns.Reserve(Columns.Num());
	for (const FString& Column : Columns)
	{
		EscapedColumns.Add(CSVEscape(Column));
	}

	OutCSV += FString::Join(EscapedColumns, TEXT(","));
	OutCSV += LINE_TERMINATOR;
}

// Guid를 CSV 저장용 문자열로 변환한다.
FString GuidToString(const FGuid& Guid)
{
	return Guid.IsValid() ? Guid.ToString(EGuidFormats::DigitsWithHyphensLower) : TEXT("");
}

// 문자열을 CSV 헤더/키 비교에 맞게 정규화한다.
FString NormalizeCSVToken(const FString& Input)
{
	return Input.TrimStartAndEnd().ToLower();
}

// Row에서 안전하게 값을 가져온다.
FString GetCSVValueAt(const TArray<FString>& Row, const int32 Index)
{
	return Row.IsValidIndex(Index) ? Row[Index] : TEXT("");
}

// 문자열을 CSV 행/열 구조로 파싱한다.
bool ParseCSVRows(const FString& CSVContent, TArray<TArray<FString>>& OutRows)
{
	OutRows.Reset();

	TArray<FString> CurrentRow;
	FString CurrentCell;
	bool bInQuotes = false;

	for (int32 Index = 0; Index < CSVContent.Len(); ++Index)
	{
		const TCHAR Character = CSVContent[Index];
		if (Character == TEXT('"'))
		{
			if (bInQuotes && (Index + 1) < CSVContent.Len() && CSVContent[Index + 1] == TEXT('"'))
			{
				CurrentCell.AppendChar(TEXT('"'));
				++Index;
			}
			else
			{
				bInQuotes = !bInQuotes;
			}
			continue;
		}

		if (!bInQuotes && Character == TEXT(','))
		{
			CurrentRow.Add(CurrentCell);
			CurrentCell.Reset();
			continue;
		}

		if (!bInQuotes && (Character == TEXT('\n') || Character == TEXT('\r')))
		{
			CurrentRow.Add(CurrentCell);
			CurrentCell.Reset();
			OutRows.Add(CurrentRow);
			CurrentRow.Reset();

			if (Character == TEXT('\r') && (Index + 1) < CSVContent.Len() && CSVContent[Index + 1] == TEXT('\n'))
			{
				++Index;
			}
			continue;
		}

		CurrentCell.AppendChar(Character);
	}

	if (!CurrentCell.IsEmpty() || CurrentRow.Num() > 0)
	{
		CurrentRow.Add(CurrentCell);
		OutRows.Add(CurrentRow);
	}

	// UTF-8 BOM 제거
	if (OutRows.Num() > 0 && OutRows[0].Num() > 0)
	{
		OutRows[0][0].RemoveFromStart(TEXT("\xFEFF"));
	}

	return OutRows.Num() > 0;
}

// "Type:Name" 문자열을 FPrimaryAssetId로 파싱한다.
bool TryParsePrimaryAssetIdString(const FString& InPrimaryAssetId, FPrimaryAssetId& OutPrimaryAssetId)
{
	FString PrimaryAssetTypeString;
	FString PrimaryAssetNameString;
	if (!InPrimaryAssetId.Split(TEXT(":"), &PrimaryAssetTypeString, &PrimaryAssetNameString))
	{
		return false;
	}

	if (PrimaryAssetTypeString.IsEmpty() || PrimaryAssetNameString.IsEmpty())
	{
		return false;
	}

	OutPrimaryAssetId = FPrimaryAssetId(FName(*PrimaryAssetTypeString), FName(*PrimaryAssetNameString));
	return true;
}

// 패키지/에셋 이름으로 사용할 문자열을 정규화한다.
FString SanitizeForObjectName(const FString& Input)
{
	FString Result;
	Result.Reserve(Input.Len());
	for (const TCHAR Character : Input)
	{
		if (FChar::IsAlnum(Character) || Character == TEXT('_'))
		{
			Result.AppendChar(Character);
		}
		else
		{
			Result.AppendChar(TEXT('_'));
		}
	}

	return Result.IsEmpty() ? TEXT("Unknown") : Result;
}

// 문화권 문자열 비교를 위해 형식을 통일한다.
FString NormalizeCultureToken(const FString& Input)
{
	FString Result = Input.TrimStartAndEnd().ToLower();
	Result.ReplaceInline(TEXT("_"), TEXT("-"));
	return Result;
}

// ELanguage 기준으로 지원 문화권 코드 맵을 구성한다.
TMap<FString, FString> BuildSupportedCultureCodeMap()
{
	TMap<FString, FString> CultureMap;

	TArray<FDialogueLanguageMapping> LanguageMappings;
	FDialogueLocalizationUtility::GetSupportedLanguageMappings(LanguageMappings);
	for (const FDialogueLanguageMapping& Mapping : LanguageMappings)
	{
		CultureMap.Add(NormalizeCultureToken(Mapping.CultureCode), Mapping.CultureCode);
	}

	return CultureMap;
}

// 입력 경로를 슬래시 기반 세그먼트 배열로 분해한다.
void ParsePathSegments(const FString& InputPath, TArray<FString>& OutSegments)
{
	OutSegments.Reset();
	FString NormalizedPath = InputPath;
	FPaths::NormalizeFilename(NormalizedPath);
	TArray<FString> RawSegments;
	NormalizedPath.ParseIntoArray(RawSegments, TEXT("/"), true);
	for (const FString& Segment : RawSegments)
	{
		if (Segment == TEXT(".") || Segment == TEXT(".."))
		{
			continue;
		}

		OutSegments.Add(Segment);
	}
}

// 세그먼트 배열을 패키지 상대 경로 문자열로 결합한다.
FString BuildSanitizedRelativePath(const TArray<FString>& Segments, const int32 StartIndex)
{
	TArray<FString> SanitizedSegments;
	for (int32 SegmentIndex = StartIndex; SegmentIndex < Segments.Num(); ++SegmentIndex)
	{
		SanitizedSegments.Add(SanitizeForObjectName(Segments[SegmentIndex]));
	}

	return FString::Join(SanitizedSegments, TEXT("/"));
}

// CSV 파일 경로에서 문화권 코드와(필요 시) 하위 상대 경로를 추론한다.
FString ResolveCultureAndRelativePath(const FString& CSVFilePath, const FString& SourceRootDirectory, FString& OutRelativePath)
{
	OutRelativePath.Empty();

	const TMap<FString, FString> SupportedCultureMap = BuildSupportedCultureCodeMap();

	FString FallbackCulture = TEXT("en-US");
	if (const FCulturePtr CurrentCulture = FInternationalization::Get().GetCurrentCulture())
	{
		const FString CurrentCultureCode = CurrentCulture->GetName();
		const FString NormalizedCurrentCode = NormalizeCultureToken(CurrentCultureCode);
		if (const FString* FoundCulture = SupportedCultureMap.Find(NormalizedCurrentCode))
		{
			FallbackCulture = *FoundCulture;
		}
		else if (!CurrentCultureCode.IsEmpty())
		{
			FallbackCulture = CurrentCultureCode;
		}
	}

	const FString CSVDirectory = FPaths::GetPath(FPaths::ConvertRelativePathToFull(CSVFilePath));
	TArray<FString> RelativeSegments;
	bool bHasRelativeSegments = false;
	FString SourceRootCleanName;

	if (!SourceRootDirectory.IsEmpty())
	{
		FString SourceRootAbs = FPaths::ConvertRelativePathToFull(SourceRootDirectory);
		FPaths::NormalizeDirectoryName(SourceRootAbs);
		SourceRootCleanName = FPaths::GetCleanFilename(SourceRootAbs);

		FString RelativeDirectory = CSVDirectory;
		if (FPaths::MakePathRelativeTo(RelativeDirectory, *SourceRootAbs))
		{
			ParsePathSegments(RelativeDirectory, RelativeSegments);
			bHasRelativeSegments = RelativeSegments.Num() > 0;
		}
	}

	int32 CultureSegmentIndex = INDEX_NONE;
	FString CultureCode = FallbackCulture;
	for (int32 SegmentIndex = 0; SegmentIndex < RelativeSegments.Num(); ++SegmentIndex)
	{
		const FString NormalizedSegment = NormalizeCultureToken(RelativeSegments[SegmentIndex]);
		if (const FString* FoundCulture = SupportedCultureMap.Find(NormalizedSegment))
		{
			CultureCode = *FoundCulture;
			CultureSegmentIndex = SegmentIndex;
			break;
		}
	}

	if (CultureSegmentIndex == INDEX_NONE && !SourceRootDirectory.IsEmpty())
	{
		const FString NormalizedRootName = NormalizeCultureToken(SourceRootCleanName);
		if (const FString* FoundCulture = SupportedCultureMap.Find(NormalizedRootName))
		{
			CultureCode = *FoundCulture;
		}
	}

	if (CultureSegmentIndex != INDEX_NONE)
	{
		OutRelativePath = BuildSanitizedRelativePath(RelativeSegments, CultureSegmentIndex + 1);
	}
	else if (bHasRelativeSegments)
	{
		OutRelativePath = BuildSanitizedRelativePath(RelativeSegments, 0);
	}

	return CultureCode;
}
}

// DialogueGraph 에셋을 CSV 파일로 저장한다.
bool FDialogueLocalizationCSVConverter::ExportDialogueGraphAssetToCSV(const UDialogueGraph* InDialogueGraph, const FString& CSVFilePath, FString* OutErrorMessage)
{
	const FString CSVContent = BuildDialogueGraphCSVFromAsset(InDialogueGraph);
	if (CSVContent.IsEmpty())
	{
		return Fail(OutErrorMessage, TEXT("CSV 문자열 생성에 실패했습니다."));
	}

	const FString DirectoryPath = FPaths::GetPath(CSVFilePath);
	if (!DirectoryPath.IsEmpty())
	{
		IFileManager::Get().MakeDirectory(*DirectoryPath, true);
	}

	if (!FFileHelper::SaveStringToFile(CSVContent, *CSVFilePath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
	{
		return Fail(OutErrorMessage, FString::Printf(TEXT("CSV 파일 저장에 실패했습니다.\n%s"), *CSVFilePath));
	}

	return true;
}

// DialogueGraph의 번역 대상 데이터를 CSV 문자열로 구성한다.
FString FDialogueLocalizationCSVConverter::BuildDialogueGraphCSVFromAsset(const UDialogueGraph* InDialogueGraph)
{
	if (InDialogueGraph == nullptr)
	{
		return TEXT("");
	}

	FString OutCSV;
	AppendCSVRow(OutCSV, {TEXT("NodeGuid"), TEXT("PinId"), TEXT("Key"), TEXT("Value")});

	const auto AddRecord = [&OutCSV](const FString& NodeGuid, const FString& PinId, const FString& Key, const FString& Value)
	{
		AppendCSVRow(OutCSV, {NodeGuid, PinId, Key, Value});
	};

	AddRecord(TEXT(""), TEXT(""), TEXT("PrimaryAssetId"), InDialogueGraph->GetPrimaryAssetId().ToString());

	if (InDialogueGraph->Graph == nullptr)
	{
		return OutCSV;
	}

	for (const UDialogueRuntimeNode* RuntimeNode : InDialogueGraph->Graph->Nodes)
	{
		if (RuntimeNode == nullptr)
		{
			continue;
		}

		const UDialogueNodeInfo* DialogueNodeInfo = Cast<UDialogueNodeInfo>(RuntimeNode->NodeInfo);
		if (DialogueNodeInfo == nullptr)
		{
			continue;
		}

		const FString NodeGuid = GuidToString(RuntimeNode->NodeGuid);
		AddRecord(NodeGuid, TEXT(""), TEXT("DialogueText"), DialogueNodeInfo->GetDialogueText().ToString());

		const TArray<FDialogueChoice>& Choices = DialogueNodeInfo->GetDialogueChoices();
		for (int32 ChoiceIndex = 0; ChoiceIndex < Choices.Num(); ++ChoiceIndex)
		{
			const FDialogueChoice& Choice = Choices[ChoiceIndex];
			const UDialogueRuntimePin* ChoicePin = RuntimeNode->OutputPins.IsValidIndex(ChoiceIndex) ? RuntimeNode->OutputPins[ChoiceIndex] : nullptr;
			const FString PinId = ChoicePin ? GuidToString(ChoicePin->PinId) : TEXT("");
			AddRecord(NodeGuid, PinId, TEXT("ResponseText"), Choice.ResponseText.ToString());
		}
	}

	return OutCSV;
}

// CSV 파일을 DialogueLocalization DataAsset으로 변환해 저장한다.
bool FDialogueLocalizationCSVConverter::ConvertCSVToDialogueLocalizationDataAsset(
	const FString& CSVFilePath,
	const FDialogueLocalizationCSVConvertOptions& Options,
	FString* OutErrorMessage,
	FString* OutSavedAssetPath)
{
	FString CSVContent;
	if (!FFileHelper::LoadFileToString(CSVContent, *CSVFilePath))
	{
		return Fail(OutErrorMessage, FString::Printf(TEXT("CSV 파일 읽기에 실패했습니다.\n%s"), *CSVFilePath));
	}

	TArray<TArray<FString>> Rows;
	if (!ParseCSVRows(CSVContent, Rows) || Rows.Num() < 2)
	{
		return Fail(OutErrorMessage, FString::Printf(TEXT("CSV 형식이 올바르지 않습니다.\n%s"), *CSVFilePath));
	}

	const TArray<FString>& Header = Rows[0];
	int32 NodeGuidColumn = INDEX_NONE;
	int32 PinIdColumn = INDEX_NONE;
	int32 KeyColumn = INDEX_NONE;
	int32 ValueColumn = INDEX_NONE;

	for (int32 HeaderIndex = 0; HeaderIndex < Header.Num(); ++HeaderIndex)
	{
		const FString NormalizedHeader = NormalizeCSVToken(Header[HeaderIndex]);
		if (NormalizedHeader == TEXT("nodeguid"))
		{
			NodeGuidColumn = HeaderIndex;
		}
		else if (NormalizedHeader == TEXT("pinid"))
		{
			PinIdColumn = HeaderIndex;
		}
		else if (NormalizedHeader == TEXT("key"))
		{
			KeyColumn = HeaderIndex;
		}
		else if (NormalizedHeader == TEXT("value"))
		{
			ValueColumn = HeaderIndex;
		}
	}

	if (NodeGuidColumn == INDEX_NONE || PinIdColumn == INDEX_NONE || KeyColumn == INDEX_NONE || ValueColumn == INDEX_NONE)
	{
		return Fail(OutErrorMessage, FString::Printf(TEXT("CSV 헤더가 올바르지 않습니다. (NodeGuid, PinId, Key, Value 필요)\n%s"), *CSVFilePath));
	}

	FPrimaryAssetId TargetDialoguePrimaryAssetId;
	bool bPrimaryAssetIdFound = false;
	TArray<FDialogueLocalizationEntry> Entries;
	Entries.Reserve(Rows.Num() - 1);

	for (int32 RowIndex = 1; RowIndex < Rows.Num(); ++RowIndex)
	{
		const TArray<FString>& Row = Rows[RowIndex];
		const FString Key = NormalizeCSVToken(GetCSVValueAt(Row, KeyColumn));
		const FString Value = GetCSVValueAt(Row, ValueColumn);

		if (Key == TEXT("primaryassetid"))
		{
			FPrimaryAssetId ParsedPrimaryAssetId;
			if (!TryParsePrimaryAssetIdString(Value.TrimStartAndEnd(), ParsedPrimaryAssetId))
			{
				return Fail(OutErrorMessage, FString::Printf(TEXT("PrimaryAssetId 파싱 실패\nCSV: %s\n값: %s"), *CSVFilePath, *Value));
			}

			TargetDialoguePrimaryAssetId = ParsedPrimaryAssetId;
			bPrimaryAssetIdFound = true;
			continue;
		}

		if (Key != TEXT("dialoguetext") && Key != TEXT("responsetext"))
		{
			continue;
		}

		FGuid ParsedNodeGuid;
		if (!FGuid::Parse(GetCSVValueAt(Row, NodeGuidColumn).TrimStartAndEnd(), ParsedNodeGuid))
		{
			return Fail(OutErrorMessage, FString::Printf(TEXT("NodeGuid 파싱 실패\nCSV: %s\n값: %s"), *CSVFilePath, *GetCSVValueAt(Row, NodeGuidColumn)));
		}

		FDialogueLocalizationEntry NewEntry;
		NewEntry.NodeGuid = ParsedNodeGuid;
		NewEntry.Key = (Key == TEXT("dialoguetext")) ? FName(TEXT("DialogueText")) : FName(TEXT("ResponseText"));
		NewEntry.Value = FText::FromString(Value);

		if (Key == TEXT("responsetext"))
		{
			if (!FGuid::Parse(GetCSVValueAt(Row, PinIdColumn).TrimStartAndEnd(), NewEntry.PinId))
			{
				return Fail(OutErrorMessage, FString::Printf(TEXT("PinId 파싱 실패\nCSV: %s\n값: %s"), *CSVFilePath, *GetCSVValueAt(Row, PinIdColumn)));
			}
		}

		Entries.Add(MoveTemp(NewEntry));
	}

	if (!bPrimaryAssetIdFound)
	{
		return Fail(OutErrorMessage, FString::Printf(TEXT("PrimaryAssetId 항목이 없습니다.\n%s"), *CSVFilePath));
	}

	if (Options.bValidateExpectedPrimaryAssetId && TargetDialoguePrimaryAssetId != Options.ExpectedPrimaryAssetId)
	{
		return Fail(
			OutErrorMessage,
			FString::Printf(
				TEXT("CSV의 PrimaryAssetId가 현재 에셋과 일치하지 않습니다.\nCSV: %s\nCSV Id: %s\nExpected Id: %s"),
				*CSVFilePath,
				*TargetDialoguePrimaryAssetId.ToString(),
				*Options.ExpectedPrimaryAssetId.ToString()));
	}

	FString RelativePathUnderCulture;
	const FString CultureCode = ResolveCultureAndRelativePath(CSVFilePath, Options.SourceRootDirectory, RelativePathUnderCulture);
	const FString CultureFolderRaw = Options.bUseUnderscoreCultureFolder ? CultureCode.Replace(TEXT("-"), TEXT("_")) : CultureCode;
	const FString CultureFolder = SanitizeForObjectName(CultureFolderRaw);

	FString DestinationRoot = Options.DestinationRootPackagePath.IsEmpty() ? TEXT("/Game/DialogueLocalization") : Options.DestinationRootPackagePath;
	DestinationRoot.RemoveFromEnd(TEXT("/"));

	FString LocalizationFolder = FString::Printf(TEXT("%s/%s"), *DestinationRoot, *CultureFolder);
	if (!RelativePathUnderCulture.IsEmpty())
	{
		LocalizationFolder = LocalizationFolder + TEXT("/") + RelativePathUnderCulture;
	}

	const FString DialogueGraphAssetName = SanitizeForObjectName(TargetDialoguePrimaryAssetId.PrimaryAssetName.ToString());
	const FString LocalizationAssetName = FString::Printf(TEXT("DL_%s_%s"), *DialogueGraphAssetName, *CultureFolder);
	const FString LocalizationPackageName = LocalizationFolder + TEXT("/") + LocalizationAssetName;

	UPackage* Package = CreatePackage(*LocalizationPackageName);
	if (Package == nullptr)
	{
		return Fail(OutErrorMessage, FString::Printf(TEXT("패키지 생성 실패: %s"), *LocalizationPackageName));
	}

	UDialogueLocalizationDataAsset* LocalizationAsset = LoadObject<UDialogueLocalizationDataAsset>(nullptr, *(LocalizationPackageName + TEXT(".") + LocalizationAssetName));
	bool bCreated = false;
	if (LocalizationAsset == nullptr)
	{
		LocalizationAsset = NewObject<UDialogueLocalizationDataAsset>(Package, *LocalizationAssetName, RF_Public | RF_Standalone | RF_Transactional);
		if (LocalizationAsset == nullptr)
		{
			return Fail(OutErrorMessage, FString::Printf(TEXT("DataAsset 생성 실패: %s"), *LocalizationPackageName));
		}
		bCreated = true;
	}

	LocalizationAsset->Modify();
	LocalizationAsset->TargetDialoguePrimaryAssetId = TargetDialoguePrimaryAssetId;
	LocalizationAsset->CultureName = CultureCode;
	LocalizationAsset->Entries = MoveTemp(Entries);
	LocalizationAsset->MarkPackageDirty();
	Package->MarkPackageDirty();

	if (bCreated)
	{
		FAssetRegistryModule::AssetCreated(LocalizationAsset);
	}

	const FString PackageFilePath = FPackageName::LongPackageNameToFilename(LocalizationPackageName, FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	SaveArgs.SaveFlags = SAVE_NoError;
	if (!UPackage::SavePackage(Package, LocalizationAsset, *PackageFilePath, SaveArgs))
	{
		return Fail(OutErrorMessage, FString::Printf(TEXT("패키지 저장 실패: %s"), *PackageFilePath));
	}

	if (OutSavedAssetPath != nullptr)
	{
		*OutSavedAssetPath = LocalizationPackageName;
	}

	return true;
}
