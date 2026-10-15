#pragma once

#include "CoreMinimal.h"
#include "DialogueCandidateSelector.h"
#include "DialogueGraph.h"
#include "DialogueBlockLoader.generated.h"

USTRUCT(BlueprintType)
struct DIALOGUEKIT_API FDialogueBlockLoadResult
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly, Category = "DialogueKit|Loading")
	FGuid RequestId;
	UPROPERTY(BlueprintReadOnly, Category = "DialogueKit|Loading")
	bool bLoaded = false;
	UPROPERTY(BlueprintReadOnly, Category = "DialogueKit|Loading")
	FName ErrorCode;
	UPROPERTY(BlueprintReadOnly, Category = "DialogueKit|Loading")
	TObjectPtr<UDialogueGraph> Block;
};
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDialogueBlockLoadEvent, const FDialogueBlockLoadResult&, Result);

// 최신 요청의 선택된 Block 하나만 로드한다. 게임이 시작/Refresh 시점을 정한다.
UCLASS(BlueprintType)
class DIALOGUEKIT_API UDialogueBlockLoader : public UObject
{
	GENERATED_BODY()
public:
	UDialogueBlockLoader();
	// 연타 방지는 게임이 입력/이동 정책에서 추가한다. 이 클래스는 늦은 이전 완료를 폐기한다.
	UFUNCTION(BlueprintCallable, Category = "DialogueKit|Loading")
	FGuid RequestFromRegistry(FGameplayTag Target, UDialogueStoryDefinition* Story, FDialogueStoryProgress Progress, UObject* Provider, UDialogueDiagnosticRouter* Router);
	// 저장 복원 전용. 현재 태그/조건/우선순위를 재검사하지 않는다.
	UFUNCTION(BlueprintCallable, Category = "DialogueKit|Loading")
	FGuid RequestExactReference(FDialogueBlockReference Reference, UDialogueDiagnosticRouter* Router);
	// 게임 카탈로그 또는 명시적 교체에서 이미 찾은 경로도 로드 후 ID/버전을 검증한다.
	UFUNCTION(BlueprintCallable, Category = "DialogueKit|Loading")
	FGuid RequestExactPath(TSoftObjectPtr<UDialogueGraph> Block, FDialogueBlockReference Reference, UDialogueDiagnosticRouter* Router);
	static bool FindExactReference(FDialogueBlockReference Reference, FSoftObjectPath& OutPath, FName& OutError);

	// 게임의 별도 카탈로그/테스트에서 메타데이터 후보를 공급하는 확장점이다.
	FGuid RequestCandidates(const TArray<FDialogueBlockCandidate>& Candidates, FGameplayTag Target,
		UDialogueStoryDefinition* Story, const FDialogueStoryProgress& Progress, UObject* Provider, UDialogueDiagnosticRouter* Router);
	UFUNCTION(BlueprintCallable, Category = "DialogueKit|Loading")
	void CancelPending();
	UFUNCTION(BlueprintCallable, Category = "DialogueKit|Loading")
	void EndSession();
	UFUNCTION(BlueprintCallable, Category = "DialogueKit|Loading")
	bool RecordBlockCompleted(FGuid RequestId, FDialogueBlockReference Block);
	// 실제 실행/표시에 성공한 때 호출한다. 로드 완료만으로 노출을 기록하지 않는다.
	UFUNCTION(BlueprintCallable, Category = "DialogueKit|Loading")
	bool RecordBlockExposed(FGuid RequestId, FDialogueBlockReference Block);
	UFUNCTION(BlueprintPure, Category = "DialogueKit|Loading")
	bool IsLoading() const { return bPending; }
	UFUNCTION(BlueprintPure, Category = "DialogueKit|Loading")
	bool IsCurrentRequest(FGuid RequestId) const { return RequestId == Generation; }
	UFUNCTION(BlueprintPure, Category = "DialogueKit|Loading")
	FDialogueBlockLoadResult GetLastResult() const { return LastResult; }
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DialogueKit|Loading")
	TSoftObjectPtr<UDialogueGraph> CommonGreetingBlock;
	UPROPERTY(BlueprintAssignable, Category = "DialogueKit|Loading")
	FDialogueBlockLoadEvent OnLoadCompleted;
	DECLARE_MULTICAST_DELEGATE_OneParam(FLoadNative, const FDialogueBlockLoadResult&);
	FLoadNative OnLoadCompletedNative;
	// 커스텀 스트리밍은 이 단일 스트리밍 callback을 연결하고 CompleteRequest로 결과를 반환한다.
	DECLARE_DELEGATE_RetVal_TwoParams(bool, FLoadOverride, FGuid, const FSoftObjectPath&);
	FLoadOverride LoadOverride;
	bool CompleteRequest(FGuid RequestId, UDialogueGraph* Block);
	virtual void BeginDestroy() override;
private:
	void InvalidateCurrentRequest();
	FGuid BeginRequest(UDialogueDiagnosticRouter* Router);
	void SelectAndLoad(FGuid RequestId, const TArray<FDialogueBlockCandidate>& Candidates, FGameplayTag Target,
		UDialogueStoryDefinition* Story, const FDialogueStoryProgress& Progress, UObject* Provider);
	void StartLoad(FGuid RequestId, const FSoftObjectPath& Path);
	void FailRequest(FGuid RequestId, FName ErrorCode);
	void Publish();
	UPROPERTY()
	TObjectPtr<UDialogueCandidateSelector> Selector;
	UPROPERTY()
	TObjectPtr<UDialogueDiagnosticRouter> DiagnosticRouter;
	UPROPERTY()
	FDialogueBlockLoadResult LastResult;
	UPROPERTY()
	FDialogueBlockCandidate Selected;
	TSharedPtr<struct FStreamableHandle> Handle;
	FGuid Generation;
	FSoftObjectPath ExpectedPath;
	bool bPending = false;
	bool bGreeting = false;
	bool bRestoring = false;
};
