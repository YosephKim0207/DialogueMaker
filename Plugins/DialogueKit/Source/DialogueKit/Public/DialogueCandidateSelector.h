#pragma once

#include "CoreMinimal.h"
#include "DialogueBlockMetadata.h"
#include "DialogueCandidateSelector.generated.h"

USTRUCT(BlueprintType)
struct DIALOGUEKIT_API FDialogueBlockCandidate
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly, Category = "DialogueKit|Selection")
	FSoftObjectPath AssetPath;
	UPROPERTY(BlueprintReadOnly, Category = "DialogueKit|Selection")
	FDialogueBlockMetadata Metadata;
};

USTRUCT(BlueprintType)
struct DIALOGUEKIT_API FDialogueSelectionResult
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly, Category = "DialogueKit|Selection")
	bool bSelected = false;
	UPROPERTY(BlueprintReadOnly, Category = "DialogueKit|Selection")
	FName ErrorCode;
	UPROPERTY(BlueprintReadOnly, Category = "DialogueKit|Selection")
	FDialogueBlockCandidate Candidate;
};

// 한 대상과의 대화 세션 안에서만 후보 소진 상태를 유지한다. 저장/누적 열람과 독립적이다.
UCLASS(BlueprintType)
class DIALOGUEKIT_API UDialogueCandidateSelector : public UObject
{
	GENERATED_BODY()
public:
	// 같은 대상은 현재 세션을 유지하고 대상이 달라지면 새 세션을 시작한다.
	UFUNCTION(BlueprintCallable, Category = "DialogueKit|Selection")
	bool BeginSession(FGameplayTag Target);
	UFUNCTION(BlueprintCallable, Category = "DialogueKit|Selection")
	void EndSession();
	// 게임이 명시적으로 시작/Refresh할 때만 호출한다. 이 함수는 에셋을 로드하지 않는다.
	UFUNCTION(BlueprintCallable, Category = "DialogueKit|Selection")
	FDialogueSelectionResult Select(const TArray<FDialogueBlockCandidate>& Candidates,
		const UDialogueStoryDefinition* Story, const FDialogueStoryProgress& Progress,
		UObject* Provider, UDialogueDiagnosticRouter* Router);
	// 실제 표시/실행 전환에 성공한 때만 직전 노출을 기록한다. 로드 취소는 기록하지 않는다.
	UFUNCTION(BlueprintCallable, Category = "DialogueKit|Selection")
	bool RecordExposed(FDialogueBlockReference Block);

	// 정상 Block 종료에서만 호출한다. 선택/로딩/중단은 소진으로 기록하지 않는다.
	UFUNCTION(BlueprintCallable, Category = "DialogueKit|Selection")
	bool RecordCompleted(FDialogueBlockReference Block);
	// 재현 가능한 자동화/게임 난수 정책 연결용이다. 기본값은 생성 때 난수 시드다.
	UFUNCTION(BlueprintCallable, Category = "DialogueKit|Selection")
	void SetRandomSeed(int32 Seed);
	UDialogueCandidateSelector();
private:
	UPROPERTY()
	FGameplayTag SessionTarget;
	UPROPERTY()
	FDialogueBlockCandidate Selected;
	TSet<FGuid> CompletedOrdinary;
	TSet<FGuid> CompletedDefaults;
	FGuid PreviousBlock;
	FRandomStream Random;
	bool bHasSelection = false;
};
