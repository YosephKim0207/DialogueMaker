#pragma once

#include "CoreMinimal.h"
#include "DialogueBlockRunner.h"
#include "DialogueCheckpointContract.h"
#include "DialogueCheckpointState.generated.h"

// 현재 표시 위치, 저장 재개점, 누적 열람 캐시를 분리한다. 디스크 저장은 게임 조율자 책임이다.
UCLASS(BlueprintType)
class DIALOGUEKIT_API UDialogueCheckpointState : public UObject
{
	GENERATED_BODY()
public:
	UDialogueCheckpointState();
	UFUNCTION(BlueprintCallable, Category = "DialogueKit|Checkpoint")
	bool BeginBlock(FDialogueBlockReference Block, FGameplayTag TargetTag, FGuid BlockRunId);
	// 표시 중 Line은 임시 수집만 한다. 저장/정상 종료 때 누적 이력으로 병합한다.
	UFUNCTION(BlueprintCallable, Category = "DialogueKit|Checkpoint")
	bool VisitLine(FGuid NodeGuid, int32 ReadVersion);
	// 실제 다음 흐름 진입 전에 호출하며 같은 실행의 통과 ID를 중복 없이 수집한다.
	UFUNCTION(BlueprintCallable, Category = "DialogueKit|Checkpoint")
	bool PassLine(FGuid NodeGuid);
	// 효과 경계 직후 호출한다. 반환 snapshot은 이후 진행과 독립적이다.
	UFUNCTION(BlueprintCallable, Category = "DialogueKit|Checkpoint")
	bool ApplyBoundary(const FDialogueExecutionBoundary& Boundary, FDialogueCheckpointSnapshot& OutSnapshot);
	// 게임의 일반 저장에도 마지막 체크포인트(없으면 Block 시작)를 사용한다.
	UFUNCTION(BlueprintCallable, Category = "DialogueKit|Checkpoint")
	FDialogueCheckpointSnapshot CaptureGameSave();
	UFUNCTION(BlueprintCallable, Category = "DialogueKit|Checkpoint")
	bool RestoreSnapshot(const FDialogueCheckpointSnapshot& Snapshot, FName& OutError);
	// 교체 준비 자료는 아직 디스크 성공이 아니므로 임시 선택 정리를 유예한다.
	bool InstallReplacement(const FDialogueCheckpointSnapshot& Snapshot, FName& OutError);
	// 게임이 실제 저장 성공한 snapshot만 전달한다. 후속 선택 자료는 보존한다.
	UFUNCTION(BlueprintCallable, Category = "DialogueKit|Checkpoint")
	void AcknowledgeSavedSnapshot(const FDialogueCheckpointSnapshot& Snapshot);
	// 사망/스토리 초기화는 열람을 유지한다. 완전한 새 게임만 bNewGame=true다.
	UFUNCTION(BlueprintCallable, Category = "DialogueKit|Checkpoint")
	void ResetStory(bool bNewGame);
	UFUNCTION(BlueprintPure, Category = "DialogueKit|Checkpoint")
	bool HasReadLine(FGuid BlockId, FGuid NodeGuid, int32 ReadVersion) const;
	UFUNCTION(BlueprintPure, Category = "DialogueKit|Checkpoint")
	FDialogueResumePosition GetResumeAnchor() const { return Cached.Resume; }
	UFUNCTION(BlueprintPure, Category = "DialogueKit|Checkpoint")
	FGuid GetCurrentLine() const { return CurrentLine; }
	UFUNCTION(BlueprintPure, Category = "DialogueKit|Checkpoint")
	TArray<FDialogueBlockReadRecord> GetReadHistory() const { return Cached.FullReadHistory; }
private:
	void MergePendingReads(bool bCompleted);
	FDialogueCheckpointSnapshot Capture(const FDialogueCheckpointTrigger& Trigger);
	UPROPERTY()
	FDialogueCheckpointSnapshot Cached;
	UPROPERTY()
	TArray<FDialogueLineReadRecord> PendingReads;
	FGuid CurrentLine;
};
