#include "DialoguePreviewPolicy.h"
#include "Engine/World.h"

// 편집기 월드와 별도 실행 중인 게임의 저장에는 영향을 주지 않는다.
bool UDialoguePreviewPolicy::IsPreviewWorld(const UObject* WorldContextObject)
{
    const UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
    return IsBlockPreviewActive() && World && World->WorldType == EWorldType::PIE;
}

#if WITH_EDITOR
static bool bDialoguePreviewActive = false;
#endif

// 배포 빌드에서는 미리보기 우회가 활성화되지 않는다.
bool UDialoguePreviewPolicy::IsBlockPreviewActive()
{
#if WITH_EDITOR
    return bDialoguePreviewActive;
#else
    return false;
#endif
}
#if WITH_EDITOR
// 다른 PIE 요청이 같은 격리 세션을 덮어쓰지 못하게 한다.
bool UDialoguePreviewPolicy::BeginPreview()
{
    check(IsInGameThread());
    if (bDialoguePreviewActive) return false;
    bDialoguePreviewActive = true;
    return true;
}
// PIE 종료/실패/취소의 공통 정리다.
void UDialoguePreviewPolicy::EndPreview()
{
    check(IsInGameThread());
    bDialoguePreviewActive = false;
}
#endif

// 실제 보상/소모는 하지 않으며 유효 효과가 있는 흐름을 계속 확인한다.
FDialogueEffectResult UDialoguePreviewStateProvider::ApplyDialogueEffects_Implementation(const FDialogueEffectBatch& Batch)
{
    FDialogueEffectResult Result;
    if (Batch.IsValid())
    {
        Result.Status = EDialogueEffectStatus::Applied;
        Result.ErrorCode = NAME_None;
        Result.Message = TEXT("Block 미리보기: 실제 효과와 저장은 실행하지 않았습니다.");
    }
    else
    {
        Result.ErrorCode = TEXT("InvalidPreviewEffect");
    }
    return Result;
}
