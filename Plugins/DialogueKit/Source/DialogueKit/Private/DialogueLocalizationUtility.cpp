#include "DialogueLocalizationUtility.h"

// 언어 enum 값을 Unreal 문화권 코드 문자열로 변환한다.
FString FDialogueLocalizationUtility::ToCultureCode(ELanguage Language)
{
	switch (Language)
	{
	case ELanguage::English:
		return TEXT("en-US");
	case ELanguage::Japanese:
		return TEXT("ja-JP");
	case ELanguage::ChineseSimplified:
		return TEXT("zh-Hans");
	case ELanguage::German:
		return TEXT("de-DE");
	case ELanguage::Spanish:
		return TEXT("es-ES");
	case ELanguage::BrazilianPortuguese:
		return TEXT("pt-BR");
	case ELanguage::Russian:
		return TEXT("ru-RU");
	case ELanguage::Korean:
		return TEXT("ko-KR");
	case ELanguage::French:
		return TEXT("fr-FR");
	case ELanguage::Italian:
		return TEXT("it-IT");
	case ELanguage::Turkish:
		return TEXT("tr-TR");
	case ELanguage::Polish:
		return TEXT("pl-PL");
	default:
		return TEXT("en-US");
	}
}

// ELanguage 기준의 지원 언어/문화권 매핑 목록을 반환한다.
void FDialogueLocalizationUtility::GetSupportedLanguageMappings(TArray<FDialogueLanguageMapping>& OutMappings)
{
	OutMappings.Reset();

	const UEnum* LanguageEnum = StaticEnum<ELanguage>();
	if (LanguageEnum == nullptr)
	{
		return;
	}

	for (int32 EnumIndex = 0; EnumIndex < LanguageEnum->NumEnums(); ++EnumIndex)
	{
		// Hidden 언어 skip
		if (LanguageEnum->HasMetaData(TEXT("Hidden"), EnumIndex))
		{
			continue;
		}

		const int64 EnumValue = LanguageEnum->GetValueByIndex(EnumIndex);
		
#pragma region 방어코드
		if (EnumValue == INDEX_NONE)
		{
			continue;
		}

		// 
		const FString EnumName = LanguageEnum->GetNameStringByIndex(EnumIndex);
		if (EnumName.EndsWith(TEXT("_MAX")))
		{
			continue;
		}
#pragma endregion 

		const ELanguage Language = static_cast<ELanguage>(EnumValue);
		const FString CultureCode = ToCultureCode(Language);
		if (CultureCode.IsEmpty())
		{
			continue;
		}

		FDialogueLanguageMapping Mapping;
		Mapping.Language = Language;
		Mapping.CultureCode = CultureCode;
		OutMappings.Add(MoveTemp(Mapping));
	}
}

// 에디터에서 사용할 대사 언어 override 설정의 ini 섹션명을 반환한다.
const TCHAR* FDialogueLocalizationUtility::GetDialogueLanguageConfigSection()
{
	return TEXT("/Script/DialogueKitEditor.DialogueKitEditorPreviewSettings");
}

// 에디터에서 사용할 대사 언어 override 설정의 ini 키명을 반환한다.
const TCHAR* FDialogueLocalizationUtility::GetDialogueLanguageConfigKey()
{
	return TEXT("DialogueCultureOverride");
}

