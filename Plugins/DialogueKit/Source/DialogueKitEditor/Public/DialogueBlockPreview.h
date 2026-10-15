#pragma once
#include "CoreMinimal.h"
#include "DialogueSampleStateProvider.h"
#include "DialogueBlockPreview.generated.h"
class UDialogueGraph;
class UEdGraph;
class UDialoguePreviewStateProvider;

USTRUCT()
struct FDialoguePreviewPreset
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, Category="테스트") FName Name = TEXT("기본");
    // 공통 정의 표와 수동 정의를 합친다. 현재 값은 States에서 별도로 지정한다.
    UPROPERTY(EditAnywhere, Category="테스트") TSoftObjectPtr<UDataTable> DefinitionTable;
    UPROPERTY(EditAnywhere, Category="테스트") TArray<FDialogueStateDefinition> Definitions;
    UPROPERTY(EditAnywhere, Category="테스트") TArray<FDialogueSampleState> States;
};

// 복사본만 편집하며 명시적 저장 버튼에서만 로컬 사용자 설정을 기록한다.
UCLASS(Config=EditorPerProjectUserSettings)
class UDialoguePreviewSettings : public UObject
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, Config, Category="테스트", meta=(ClampMin="0")) int32 SelectedPreset = 0;
    UPROPERTY(EditAnywhere, Config, Category="테스트", meta=(TitleProperty="Name")) TArray<FDialoguePreviewPreset> Presets;
};

// 에디터의 단일 Block PIE 조율자다. 게임 실행 API가 아니다.
class FDialogueBlockPreview
{
public:
    static void ShowSettings();
    static bool Start(UDialogueGraph* Asset, UEdGraph* Graph, bool bIgnoreEntry, const FDialoguePreviewPreset* PresetOverride = nullptr);
    static void Stop();
    static class UDialogueBlockRunner* GetRunner();
    static void Shutdown();
    // 자동화와 메뉴가 동일한 미저장 그래프 복사/프리셋 경로를 사용한다.
    static UDialogueGraph* CopyWorkingBlock(UDialogueGraph* Asset, UEdGraph* Graph, FString& Error);
    static UDialoguePreviewStateProvider* MakeProvider(const FDialoguePreviewPreset& Preset, FString& Error);
    static bool CheckEntry(UDialogueGraph* Block, UObject* Provider, FString& Explanation);
private:
    static void OnStarted(bool bSimulating);
    static void Finish();
};
