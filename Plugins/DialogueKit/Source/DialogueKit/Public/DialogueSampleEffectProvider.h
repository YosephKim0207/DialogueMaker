#pragma once

#include "CoreMinimal.h"
#include "DialogueSampleStateProvider.h"
#include "DialogueEffectContract.h"
#include "DialogueReplacement.h"
#include "DialogueSampleEffectProvider.generated.h"

// 성공한 보정의 멱등성과 출처를 같은 게임 snapshot에 보존한다.
USTRUCT(BlueprintType)
struct DIALOGUEKIT_API FDialogueSampleCorrectionReceipt
{
	GENERATED_BODY()
	UPROPERTY(SaveGame) FGuid CorrectionId;
	UPROPERTY(SaveGame) FString RuleFingerprint;
	UPROPERTY(SaveGame) FDialogueBlockReference Source;
	UPROPERTY(SaveGame) FGuid BlockRunId;
	UPROPERTY(SaveGame) TArray<FGuid> Executions;
};

// 게임 상태와 실행 원장을 함께 복사하는 샘플 저장 자료다. 실제 게임의 저장 포맷은 게임이 소유한다.
USTRUCT(BlueprintType)
struct DIALOGUEKIT_API FDialogueSampleEffectSnapshot
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "DialogueKit|Sample")
	int32 SchemaVersion = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "DialogueKit|Sample")
	TArray<FDialogueStateDefinition> Definitions;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "DialogueKit|Sample")
	TArray<FDialogueSampleState> States;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "DialogueKit|Sample")
	TMap<FGuid, FDialogueEffectBatch> Requests;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "DialogueKit|Sample")
	TMap<FGuid, FDialogueEffectResult> Results;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "DialogueKit|Sample")
	TArray<FDialogueSampleCorrectionReceipt> Corrections;

	// 전체 검사 성공 전에는 실행 중 상태를 변경하지 않는다.
	bool Validate(FName& OutError) const;
};

// 메모리 샘플이다. 캡처 자료는 게임의 대화 체크포인트와 같은 저장 단위에 넣는다.
UCLASS(BlueprintType, Blueprintable)
class DIALOGUEKIT_API UDialogueSampleEffectProvider : public UDialogueSampleStateProvider, public IDialogueEffectProvider, public IDialogueCorrectionProvider
{
	GENERATED_BODY()
public:
	// 같은 ID의 성공/실패는 최종 결과다. 게임이 재시도를 허용하면 새 ExecutionId를 발급한다.
	virtual FDialogueEffectResult ApplyDialogueEffects_Implementation(const FDialogueEffectBatch& Batch) override;

	// 초기값 0/단일 작성자/Count Add 전용 출처 검증 예제다. 범용 게임 롤백이 아니다.
	virtual FDialogueEffectResult CorrectDialogueState_Implementation(const FDialogueCorrectionRequest& Request) override;

	// 효과 성공 직후 다음 변경 전에 호출한다. 반환 자료는 이후 상태 변경과 독립적이다.
	UFUNCTION(BlueprintCallable, Category = "DialogueKit|Sample")
	FDialogueSampleEffectSnapshot CaptureEffectSnapshot() const;

	// 실패 시 기존 상태를 유지한다. 성공해도 효과를 재실행하지 않는다.
	UFUNCTION(BlueprintCallable, Category = "DialogueKit|Sample")
	bool RestoreEffectSnapshot(const FDialogueSampleEffectSnapshot& Snapshot, FName& OutError);

private:
	UPROPERTY()
	TArray<FDialogueSampleCorrectionReceipt> Corrections;
	UPROPERTY()
	TMap<FGuid, FDialogueEffectBatch> Requests;

	UPROPERTY()
	TMap<FGuid, FDialogueEffectResult> Results;
};
