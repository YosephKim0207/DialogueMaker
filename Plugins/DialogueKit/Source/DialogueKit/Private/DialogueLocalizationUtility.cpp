#include "DialogueLocalizationUtility.h"

// 언어 enum 값을 Unreal 문화권 코드 문자열로 변환한다.
FString FDialogueLocalizationUtility::ToCultureCode(ELanguage Language)
{
	switch (Language)
	{
	case ELanguage::English:
		return TEXT("en-US");
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

