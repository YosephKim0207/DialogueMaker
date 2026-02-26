#pragma once

#include "CoreMinimal.h"
#include "UObject/PrimaryAssetId.h"

class UDialogueGraph;

struct FDialogueLocalizationCSVConvertOptions
{
	// CSV 루트 디렉토리(상대 경로 계산용). 비어있으면 CSV 상위 폴더만 사용한다.
	FString SourceRootDirectory;

	// 생성될 DataAsset의 루트 패키지 경로.
	FString DestinationRootPackagePath = TEXT("/Game/DialogueLocalization");

	// true면 CultureCode 폴더명을 en-US 대신 en_US 형식으로 저장한다.
	bool bUseUnderscoreCultureFolder = true;

	// true면 CSV의 PrimaryAssetId가 ExpectedPrimaryAssetId와 일치해야 한다.
	bool bValidateExpectedPrimaryAssetId = false;

	FPrimaryAssetId ExpectedPrimaryAssetId;
};

class DIALOGUEKITEDITOR_API FDialogueLocalizationCSVConverter
{
public:
	// DialogueGraph 에셋을 CSV 파일로 저장한다.
	static bool ExportDialogueGraphAssetToCSV(const UDialogueGraph* InDialogueGraph, const FString& CSVFilePath, FString* OutErrorMessage = nullptr);

	// DialogueGraph의 번역 대상 데이터를 CSV 문자열로 구성한다.
	static FString BuildDialogueGraphCSVFromAsset(const UDialogueGraph* InDialogueGraph);

	// CSV 파일을 DialogueLocalization DataAsset으로 변환해 저장한다.
	static bool ConvertCSVToDialogueLocalizationDataAsset(
		const FString& CSVFilePath,
		const FDialogueLocalizationCSVConvertOptions& Options,
		FString* OutErrorMessage = nullptr,
		FString* OutSavedAssetPath = nullptr);
};
