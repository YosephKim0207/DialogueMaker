#pragma once
#include "CoreMinimal.h"
#include "DialogueCheckpointSession.h"
#include "DialogueBlockLoader.h"
#include "DialogueRestoreController.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FDialogueRestoreEvent, bool, bSuccess, FName, ErrorCode);

// 게임 합본 복원 이후 exact/명시적 교체를 조율한다. 게임 상태 자체는 게임이 먼저 복원한다.
UCLASS(BlueprintType)
class DIALOGUEKIT_API UDialogueRestoreController : public UObject
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category = "DialogueKit|Restore")
	bool Initialize(UDialogueCheckpointSession* Session, UObject* CorrectionProvider, UDialogueDiagnosticRouter* Router, UDialogueBlockLoader* CandidateLoader = nullptr);
	UFUNCTION(BlueprintCallable, Category = "DialogueKit|Restore")
	bool Restore(const FDialogueCheckpointSnapshot& Snapshot, UDialogueReplacementMap* Map);
	// true는 대기 또는 동기 완료 성공이며 최종 결과는 OnRestored로 전달한다.
	// 게임이 상태를 초기화한 같은 호출에서 사용한다. 이전 늦은 로드는 무효화한다.
	UFUNCTION(BlueprintCallable, Category = "DialogueKit|Restore")
	bool ResetStoryAndSave(bool bNewGame);
	UFUNCTION(BlueprintCallable, Category = "DialogueKit|Restore")
	bool CancelRestore();
	UFUNCTION(BlueprintPure, Category = "DialogueKit|Restore")
	bool IsRestoring() const { return bWaiting; }
	UPROPERTY(BlueprintAssignable, Category = "DialogueKit|Restore") FDialogueRestoreEvent OnRestored;
	// 사용자 UI/게임 반환 흐름을 테스트할 수 있는 관찰 지점이다.
	DECLARE_MULTICAST_DELEGATE_TwoParams(FRestoreNative, bool, FName);
	FRestoreNative OnRestoredNative;
private:
	void Loaded(const FDialogueBlockLoadResult& Result);
	void Finish(bool bSuccess, FName ErrorCode);
	UPROPERTY() TObjectPtr<UDialogueCheckpointSession> Session;
	UPROPERTY() TObjectPtr<UDialogueBlockLoader> Loader;
	UPROPERTY() TObjectPtr<UDialogueBlockLoader> CandidateLoader;
	UPROPERTY() TObjectPtr<UObject> CorrectionProvider;
	UPROPERTY() TObjectPtr<UDialogueDiagnosticRouter> Router;
	UPROPERTY() TObjectPtr<UDialogueGraph> SourceGraph;
	UPROPERTY() FDialogueCheckpointSnapshot Source;
	UPROPERTY() FDialogueReplacementRule Rule;
	bool bReplacement = false;
	bool bLoadingSource = false;
	bool bWaiting = false;
	bool bHandling = false;
	bool bLastSuccess = false;
};
