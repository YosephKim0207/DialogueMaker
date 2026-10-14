#pragma once

#include "CoreMinimal.h"
#include "DialogueCheckpointContract.h"
#include "DialogueSampleCheckpointProvider.generated.h"

// 요청 값 복사만 보여 주는 샘플이다. 디스크 저장/FIFO 실행/성공 통지를 수행하지 않는다.
UCLASS(BlueprintType)
class DIALOGUEKIT_API UDialogueSampleCheckpointProvider : public UObject, public IDialogueCheckpointProvider
{
	GENERATED_BODY()
public:
	// 실제 게임은 이 호출에서 게임 상태와 효과 ledger도 같이 복사하여 공통 큐에 제출한다.
	virtual FDialogueCheckpointAcceptance RequestDialogueCheckpoint_Implementation(const FDialogueCheckpointRequest& Request) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DialogueKit|Checkpoint")
	TArray<FDialogueCheckpointRequest> CapturedRequests;
};
