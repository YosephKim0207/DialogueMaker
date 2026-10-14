#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "DialogueLocalizationUtility.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDialogueLanguageMappingTest,
    "DialogueKit.Localization.SupportedLanguageMappings",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// 지원 언어 목록이 중복/자동 MAX 항목 없이 실제 문화권 매핑과 일치하는지 검사한다.
bool FDialogueLanguageMappingTest::RunTest(const FString& Parameters)
{
    TArray<FDialogueLanguageMapping> Mappings;
    FDialogueLocalizationUtility::GetSupportedLanguageMappings(Mappings);
    TestEqual(TEXT("Current supported languages"), Mappings.Num(), 12);
    TSet<FString> Cultures;
    TSet<uint8> Languages;
    for (const FDialogueLanguageMapping& Mapping : Mappings)
    {
        TestFalse(TEXT("Culture is not empty"), Mapping.CultureCode.IsEmpty());
        TestFalse(TEXT("Culture is unique"), Cultures.Contains(Mapping.CultureCode));
        TestFalse(TEXT("Language is unique"), Languages.Contains(static_cast<uint8>(Mapping.Language)));
        TestEqual(TEXT("Mapping matches public conversion"), Mapping.CultureCode,
            FDialogueLocalizationUtility::ToCultureCode(Mapping.Language));
        Cultures.Add(Mapping.CultureCode);
        Languages.Add(static_cast<uint8>(Mapping.Language));
    }
    TestTrue(TEXT("Korean mapping retained"), Cultures.Contains(TEXT("ko-KR")));
    TestTrue(TEXT("English mapping retained"), Cultures.Contains(TEXT("en-US")));
    return true;
}
#endif
