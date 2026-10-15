#pragma once

#include "CoreMinimal.h"
#include "DialogueCheckpointContract.h"
#include "DialogueSaveQueue.generated.h"

// 게임이 같은 시점에 캡처한 전체 자료다. 큐는 게임 payload의 내부 형식을 해석하지 않는다.
USTRUCT(BlueprintType)
struct DIALOGUEKIT_API FDialogueQueuedSave
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly, Category = "DialogueKit|Save")
	FDialogueCheckpointRequest Request;
	UPROPERTY(BlueprintReadOnly, Category = "DialogueKit|Save")
	TArray<uint8> GamePayload;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDialogueSaveWriteEvent, const FDialogueQueuedSave&, Save);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDialogueSaveCompleteEvent, const FDialogueCheckpointCompletion&, Completion);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FDialogueSaveStateEvent, bool, bWriting, bool, bFailed);

// 게임 공통 저장 조율자가 소유하는 FIFO 도구다. 독립 대화 저장 파일을 만들지 않는다.
UCLASS(BlueprintType)
class DIALOGUEKIT_API UDialogueSaveQueue : public UObject
{
	GENERATED_BODY()
public:
	// 모든 요청이 처리된 상태에서만 새 세대를 시작한다. 진행 중 파일 쓰기를 취소하지 않는다.
	UFUNCTION(BlueprintCallable, Category = "DialogueKit|Save")
	bool InitializeQueue(FGuid NewGeneration);

	// 게임 공통 큐의 다음 요청을 만든다. 바로 Enqueue해야 하며 이 함수만으로 접수되지 않는다.
	UFUNCTION(BlueprintCallable, Category = "DialogueKit|Save")
	FDialogueCheckpointRequest MakeRequest(const FDialogueCheckpointSnapshot& Snapshot) const;

	// 최초 캡처 실패 중 후속 게임 저장의 순서 역전을 막는다. 접수 성공 시 해제한다.
	bool ReserveCapture(FGuid CheckpointId);

	// 호출 전에 게임 상태/원장과 Snapshot을 함께 캡처한다. true는 접수이며 디스크 성공이 아니다.
	UFUNCTION(BlueprintCallable, Category = "DialogueKit|Save")
	FDialogueCheckpointAcceptance Enqueue(const FDialogueCheckpointRequest& Request, const TArray<uint8>& GamePayload);

	// RequestId/세대/순서가 현재 쓰기와 정확히 일치할 때만 처리한다. 오래된 완료는 false다.
	UFUNCTION(BlueprintCallable, Category = "DialogueKit|Save")
	bool CompleteWrite(const FDialogueCheckpointCompletion& Completion);

	// 실패 head의 불변 자료와 순서를 유지하고 새 물리 요청 ID로 재시도한다.
	UFUNCTION(BlueprintCallable, Category = "DialogueKit|Save")
	bool RetryFailedWrite();

	UFUNCTION(BlueprintPure, Category = "DialogueKit|Save")
	int32 GetPendingCount() const { return Pending.Num(); }
	UFUNCTION(BlueprintPure, Category = "DialogueKit|Save")
	bool IsWriting() const { return bWriting; }
	UFUNCTION(BlueprintPure, Category = "DialogueKit|Save")
	bool HasFailedWrite() const { return bFailed; }
	UFUNCTION(BlueprintPure, Category = "DialogueKit|Save")
	FGuid GetGeneration() const { return Generation; }
	UFUNCTION(BlueprintPure, Category = "DialogueKit|Save")
	FDialogueQueuedSave GetLastSuccessfulSave() const { return LastSuccessfulSave; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DialogueKit|Save")
	TObjectPtr<UDialogueDiagnosticRouter> DiagnosticRouter;

	// 게임의 비동기 저장 어댑터는 이 이벤트를 받고 CompleteWrite로 결과를 알린다.
	UPROPERTY(BlueprintAssignable, Category = "DialogueKit|Save")
	FDialogueSaveWriteEvent OnWriteRequested;
	UPROPERTY(BlueprintAssignable, Category = "DialogueKit|Save")
	FDialogueSaveCompleteEvent OnWriteCompleted;
	// 저장 이미지와 실패 안내는 게임 UI에서 이 이벤트를 구독한다.
	UPROPERTY(BlueprintAssignable, Category = "DialogueKit|Save")
	FDialogueSaveStateEvent OnSaveStateChanged;

	DECLARE_DELEGATE_OneParam(FWriteNative, const FDialogueQueuedSave&);
	FWriteNative OnWriteRequestedNative;
	DECLARE_MULTICAST_DELEGATE_OneParam(FSavedNative, const FDialogueQueuedSave&);
	FSavedNative OnSavedNative;

private:
	// 동기 완료/접수 재진입도 후속 항목의 배열 참조를 무효화하지 않도록 복사하여 게시한다.
	void Pump();
	UPROPERTY()
	TArray<FDialogueQueuedSave> Pending;
	UPROPERTY()
	FDialogueQueuedSave LastSuccessfulSave;
	FGuid Generation;
	FGuid ReservedCheckpoint;
	int64 LastSequence = 0;
	TSet<FGuid> SeenRequests;
	TSet<FGuid> SeenCheckpoints;
	bool bWriting = false;
	bool bFailed = false;
	bool bPumping = false;
	bool bCompleting = false;
	bool bAnnouncing = false;
};
