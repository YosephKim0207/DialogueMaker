#pragma once

#include "CoreMinimal.h"
#include "DialogueSampleStateProvider.h"
#include "DialogueEffectContract.h"
#include "DialogueSampleEffectProvider.generated.h"

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

	// 전체 검사 성공 전에는 실행 중 상태를 변경하지 않는다.
	bool Validate(FName& OutError) const;
};

// 메모리 샘플이다. 캡처 자료는 게임의 대화 체크포인트와 같은 저장 단위에 넣는다.
UCLASS(BlueprintType, Blueprintable)
class DIALOGUEKIT_API UDialogueSampleEffectProvider : public UDialogueSampleStateProvider, public IDialogueEffectProvider
{
	GENERATED_BODY()
public:
	// 같은 ID의 성공/실패는 최종 결과다. 게임이 재시도를 허용하면 새 ExecutionId를 발급한다.
	virtual FDialogueEffectResult ApplyDialogueEffects_Implementation(const FDialogueEffectBatch& Batch) override;

	// 효과 성공 직후 다음 변경 전에 호출한다. 반환 자료는 이후 상태 변경과 독립적이다.
	UFUNCTION(BlueprintCallable, Category = "DialogueKit|Sample")
	FDialogueSampleEffectSnapshot CaptureEffectSnapshot() const;

	// 실패 시 기존 상태를 유지한다. 성공해도 효과를 재실행하지 않는다.
	UFUNCTION(BlueprintCallable, Category = "DialogueKit|Sample")
	bool RestoreEffectSnapshot(const FDialogueSampleEffectSnapshot& Snapshot, FName& OutError);

private:
	UPROPERTY()
	TMap<FGuid, FDialogueEffectBatch> Requests;

	UPROPERTY()
	TMap<FGuid, FDialogueEffectResult> Results;
};
