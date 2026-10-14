#pragma once

#include "CoreMinimal.h"
#include "DialogueSampleStateProvider.h"
#include "DialogueEffectContract.h"
#include "DialogueSampleEffectProvider.generated.h"

// 메모리 예제이며 디스크 저장/게임 저장 복원은 구현하지 않는다.
UCLASS(BlueprintType, Blueprintable)
class DIALOGUEKIT_API UDialogueSampleEffectProvider : public UDialogueSampleStateProvider, public IDialogueEffectProvider
{
	GENERATED_BODY()
public:
	// 같은 ID의 성공/실패는 최종 결과다. 게임이 재시도를 허용하면 새 ExecutionId를 발급한다.
	virtual FDialogueEffectResult ApplyDialogueEffects_Implementation(const FDialogueEffectBatch& Batch) override;

private:
	UPROPERTY()
	TMap<FGuid, FDialogueEffectBatch> Requests;

	UPROPERTY()
	TMap<FGuid, FDialogueEffectResult> Results;
};
