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
#if WITH_EDITOR
		// 메타데이터는 에디터 전용이다. 게임 빌드에서는 런타임 enum 값으로 목록을 구성한다.
		if (LanguageEnum->HasMetaData(TEXT("Hidden"), EnumIndex))
		{
			continue;
		}
#endif

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


// 언어 전용 코드와 구분자를 정규화하되 번체 등 명시적인 문화권을 임의 변환하지 않는다.
FString FDialogueLocalizationUtility::NormalizeCultureCode(const FString& CultureCode)
{
	const FString Normalized = CultureCode.TrimStartAndEnd().Replace(TEXT("_"), TEXT("-"));
	TArray<FDialogueLanguageMapping> Mappings;
	GetSupportedLanguageMappings(Mappings);
	for (const FDialogueLanguageMapping& Mapping : Mappings)
	{
		if (Normalized.Equals(Mapping.CultureCode, ESearchCase::IgnoreCase))
		{
			return Mapping.CultureCode;
		}
		FString Language;
		FString Region;
		if (Mapping.CultureCode.Split(TEXT("-"), &Language, &Region) && Normalized.Equals(Language, ESearchCase::IgnoreCase))
		{
			return Mapping.CultureCode;
		}
	}
	return Normalized;
}
