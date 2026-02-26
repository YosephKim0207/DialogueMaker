#pragma once

#include "CoreMinimal.h"
#include "DialogueLocalizationUtility.generated.h"

UENUM(BlueprintType)
enum class ELanguage : uint8
{
	English UMETA(DisplayName = "English"),
	ChineseSimplified UMETA(DisplayName = "Chinese (Simplified)"),
	German UMETA(DisplayName = "German"),
	Spanish UMETA(DisplayName = "Spanish"),
	BrazilianPortuguese UMETA(DisplayName = "Brazilian Portuguese"),
	Russian UMETA(DisplayName = "Russian"),
	Korean UMETA(DisplayName = "Korean"),
	French UMETA(DisplayName = "French"),
	Italian UMETA(DisplayName = "Italian"),
	Turkish UMETA(DisplayName = "Turkish"),
	Polish UMETA(DisplayName = "Polish"),
	Japanese UMETA(DisplayName = "Japanese")
};

struct FDialogueLanguageMapping
{
	ELanguage Language = ELanguage::English;
	FString CultureCode;
};

class DIALOGUEKIT_API FDialogueLocalizationUtility
{
public:
	// 언어 enum 값을 Unreal 문화권 코드 문자열로 변환한다.
	static FString ToCultureCode(ELanguage Language);

	// ELanguage 기준의 지원 언어/문화권 매핑 목록을 반환한다.
	static void GetSupportedLanguageMappings(TArray<FDialogueLanguageMapping>& OutMappings);

	// 에디터에서 사용할 대사 언어 override 설정의 ini 섹션명을 반환한다.
	static const TCHAR* GetDialogueLanguageConfigSection();

	// 에디터에서 사용할 대사 언어 override 설정의 ini 키명을 반환한다.
	static const TCHAR* GetDialogueLanguageConfigKey();
};

