#pragma once

#include "CoreMinimal.h"
#include "DialogueCheckpointState.h"
#include "DialogueSaveQueue.h"
#include "DialogueReplacement.h"
#include "DialogueCheckpointSession.generated.h"

// 게임 저장 제공자와 실행기를 연결한다. 실제 디스크 쓰기와 게임 상태 소유권은 제공자에 있다.
UCLASS(BlueprintType)
class DIALOGUEKIT_API UDialogueCheckpointSession : public UObject
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category = "DialogueKit|Session")
	bool Initialize(UObject* SaveProvider, UObject* StateProvider, UObject* EffectProvider, UDialogueDiagnosticRouter* Router, UDialogueBlockRunner* ExistingRunner = nullptr);
	UFUNCTION(BlueprintCallable, Category = "DialogueKit|Session")
	bool StartBlock(UDialogueGraph* Block, FGameplayTag Target);
	// 게임 상태를 같은 합본에서 먼저 복원한 뒤 호출한다. 후보 필터는 적용하지 않는다.
	UFUNCTION(BlueprintCallable, Category = "DialogueKit|Session")
	bool ResumeBlock(UDialogueGraph* Block, const FDialogueCheckpointSnapshot& Snapshot);
	// Source는 게임과 함께 복원한 저장이다. 선택적 롤백은 보존 원본 Graph도 전달한다.
	UFUNCTION(BlueprintCallable, Category = "DialogueKit|Session")
	bool ReplaceLoadedBlock(UDialogueGraph* Target, const FDialogueCheckpointSnapshot& Source, const FDialogueReplacementRule& Rule, UDialogueGraph* SourceGraph, UObject* CorrectionProvider);
	// 활성 대화가 없는 저장은 이전 Block을 다시 로드/실행하지 않는다.
	UFUNCTION(BlueprintCallable, Category = "DialogueKit|Session")
	bool RestoreInactive(const FDialogueCheckpointSnapshot& Snapshot);
	UFUNCTION(BlueprintCallable, Category = "DialogueKit|Session")
	bool SaveCurrentState();
	// 접수 실패만 재시도한다. 디스크 실패는 게임 FIFO RetryFailedWrite를 사용한다.
	UFUNCTION(BlueprintCallable, Category = "DialogueKit|Session")
	bool RetryCapture();
	// 대화 취소는 활성 이력을 버리지 않는다. 사망/새게임은 별도 ResetStory를 사용한다.
	UFUNCTION(BlueprintCallable, Category = "DialogueKit|Session")
	bool ResetStory(bool bNewGame);
	// 저장 불가이면 초기화하지 않는다. 접수 실패이면 초기화 snapshot을 재시도한다.
	UFUNCTION(BlueprintCallable, Category = "DialogueKit|Session")
	bool ResetStoryAndSave(bool bNewGame);
	UFUNCTION(BlueprintPure, Category = "DialogueKit|Session")
	UDialogueBlockRunner* GetRunner() const { return Runner; }
	UFUNCTION(BlueprintPure, Category = "DialogueKit|Session")
	UDialogueGraph* GetActiveBlock() const { return ActiveBlock; }
	UFUNCTION(BlueprintPure, Category = "DialogueKit|Session")
	UDialogueCheckpointState* GetCheckpointState() const { return State; }
	UFUNCTION(BlueprintPure, Category = "DialogueKit|Session")
	bool HasPendingCapture() const { return bPendingCapture; }
	UFUNCTION(BlueprintPure, Category = "DialogueKit|Session")
	FName GetLastError() const { return LastError; }
	// 실제 성공 snapshot만 전달한다. 샘플 조율자의 큐를 사용하면 AttachSaveQueue로 자동 연결한다.
	UFUNCTION(BlueprintCallable, Category = "DialogueKit|Session")
	void AcknowledgeSaved(const FDialogueCheckpointSnapshot& Snapshot);
	UFUNCTION(BlueprintCallable, Category = "DialogueKit|Session")
	bool AttachSaveQueue(UDialogueSaveQueue* Queue);
private:
	bool Ready() const;
	bool Capture(const FDialogueCheckpointSnapshot& Snapshot);
	bool HandleBoundary(const FDialogueExecutionBoundary& Boundary);
	bool Visit(FGuid Line, int32 Version);
	bool Pass(FGuid Line);
	void Saved(const FDialogueQueuedSave& Save);
	bool CanOperate() const;
	UPROPERTY() TObjectPtr<UDialogueBlockRunner> Runner;
	UPROPERTY() TObjectPtr<UDialogueCheckpointState> State;
	UPROPERTY() TObjectPtr<UObject> SaveProvider;
	UPROPERTY() TObjectPtr<UObject> StateProvider;
	UPROPERTY() TObjectPtr<UObject> EffectProvider;
	UPROPERTY() TObjectPtr<UDialogueDiagnosticRouter> Router;
	UPROPERTY() TObjectPtr<UDialogueGraph> ActiveBlock;
	UPROPERTY() TObjectPtr<UDialogueSaveQueue> SaveQueue;
	UPROPERTY() FDialogueCheckpointSnapshot Pending;
	FName LastError;
	bool bBusy = false;
	bool bPendingCapture = false;
	bool bResumeAfterCapture = false;
};
