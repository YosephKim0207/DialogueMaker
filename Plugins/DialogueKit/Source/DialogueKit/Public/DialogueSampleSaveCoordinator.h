#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "DialogueSampleEffectProvider.h"
#include "DialogueSaveQueue.h"
#include "DialogueSampleSaveCoordinator.generated.h"

// 샘플 게임의 상태와 대화 요청을 한 파일 payload로 묶는다.
UCLASS()
class DIALOGUEKIT_API UDialogueSampleCombinedSave : public USaveGame
{
	GENERATED_BODY()
public:
	UPROPERTY()
	int32 SchemaVersion = 1;
	UPROPERTY()
	int64 Revision = 0;
	UPROPERTY()
	FDialogueCheckpointRequest Request;
	UPROPERTY()
	FDialogueSampleEffectSnapshot Game;
	bool Validate() const;
};

// 두 내부 슬롯 중 온전한 최신 payload만 사용한다. 사용자용 과거 저장 선택 기능은 없다.
UCLASS()
class DIALOGUEKIT_API UDialogueSampleSaveFile : public USaveGame
{
	GENERATED_BODY()
public:
	UPROPERTY()
	TArray<uint8> Payload;
	UPROPERTY()
	uint32 Checksum = 0;
};

// GameInstance 등이 수명 동안 보유하는 샘플 게임 저장 조율자다. 실제 게임 저장과 별도로 병행하지 않는다.
UCLASS(BlueprintType)
class DIALOGUEKIT_API UDialogueSampleSaveCoordinator : public UObject, public IDialogueCheckpointProvider
{
	GENERATED_BODY()
public:
	// 전용 이름만 사용한다. 같은 이름의 다른 조율자/쓰기와 병행하면 안 된다.
	UFUNCTION(BlueprintCallable, Category = "DialogueKit|Sample")
	bool Initialize(const FString& NewSlotPrefix, UDialogueSampleEffectProvider* NewProvider, FName& OutError);

	// 대기 자료가 없을 때만 슬롯 소유권을 반납한다. 정상 종료는 먼저 큐를 비워야 한다.
	UFUNCTION(BlueprintCallable, Category = "DialogueKit|Sample")
	bool Shutdown();

	virtual bool CanCaptureDialogueCheckpoint_Implementation() const override;
	virtual FDialogueCheckpointAcceptance CaptureDialogueCheckpoint_Implementation(const FDialogueCheckpointSnapshot& Snapshot) override;

	// 같은 호출 안에서 샘플 상태를 복사한다. 반환 성공은 FIFO 접수 성공이다.
	virtual FDialogueCheckpointAcceptance RequestDialogueCheckpoint_Implementation(const FDialogueCheckpointRequest& Request) override;

	UFUNCTION(BlueprintPure, Category = "DialogueKit|Sample")
	UDialogueSaveQueue* GetSaveQueue() const { return Queue; }

	// 쓰기 대기 중에는 복원하지 않는다. 성공하면 게임 상태와 저장된 대화 자료를 함께 반환한다.
	UFUNCTION(BlueprintCallable, Category = "DialogueKit|Sample")
	bool RestoreLatest(FDialogueCheckpointSnapshot& OutSnapshot, FName& OutError);

	// 읽기 전용 검사다. 손상된 새 슬롯 대신 이전 온전한 슬롯을 선택한다.
	static UDialogueSampleCombinedSave* LoadLatest(const FString& SlotPrefix);

private:
	void WriteSave(const FDialogueQueuedSave& Save);
	UPROPERTY()
	TObjectPtr<UDialogueSampleEffectProvider> Provider;
	UPROPERTY()
	TObjectPtr<UDialogueSaveQueue> Queue;
	FString SlotPrefix;
	int64 Revision = 0;
	UPROPERTY()
	TObjectPtr<UDialogueSampleCombinedSave> PendingCapture;
};
