#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "DialogueSampleStateProvider.h"
#include "DialogueEffectContract.h"
#include "DialoguePreviewPolicy.generated.h"

// 게임 제공자는 이 값을 초기화 시 확인해 테스트 중 영속 I/O와 외부 효과를 차단한다.
UCLASS()
class DIALOGUEKIT_API UDialoguePreviewPolicy : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintPure, Category="DialogueKit|Preview")
    static bool IsBlockPreviewActive();
    // Subsystem 초기화에서는 전역 상태만 보지 말고 자신의 PIE 월드를 함께 확인한다.
    UFUNCTION(BlueprintPure, Category="DialogueKit|Preview")
    static bool IsPreviewWorld(const UObject* WorldContextObject);
#if WITH_EDITOR
    // Editor의 단일 PIE 조율자만 시작/종료하며 중첩 시작은 거부한다.
    static bool BeginPreview();
    static void EndPreview();
#endif
};

// 테스트의 상태 값은 고정한다. 효과는 정상 흐름만 허용하고 게임 상태/저장에 접근하지 않는다.
UCLASS()
class DIALOGUEKIT_API UDialoguePreviewStateProvider : public UDialogueSampleStateProvider, public IDialogueEffectProvider
{
    GENERATED_BODY()
public:
    virtual FDialogueEffectResult ApplyDialogueEffects_Implementation(const FDialogueEffectBatch& Batch) override;
};
