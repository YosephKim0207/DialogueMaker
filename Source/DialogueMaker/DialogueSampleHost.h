#pragma once
#include "CoreMinimal.h"
#include "DialogueBlockLoader.h"
#include "DialogueRestoreController.h"
#include "DialogueSampleHost.generated.h"

class UDialogueSubsystem;
class UDialoguePresentationWidget;
class UDialogueSavingWidget;
class APlayerController;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FDialogueSampleHostResult, bool, bSuccess, FName, ErrorCode);

// 샘플 게임 소유 연결 객체다. 실제 게임은 상태·저장 제공자와 수명/입력 정책을 명시적으로 공급한다.
UCLASS(BlueprintType)
class DIALOGUEMAKER_API UDialogueSampleHost : public UObject
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category = "DialogueSample")
	bool Initialize(UDialogueSubsystem* Dialogue, UObject* StateProvider, UObject* EffectProvider,
		UObject* SaveProvider, UDialogueSaveQueue* Queue, UObject* CorrectionProvider, UDialogueDiagnosticRouter* Router);
	// 성공은 요청 접수이며 최종 실행 결과는 OnStarted다. 연타/이동 제한은 게임 입력 계층에서 정한다.
	UFUNCTION(BlueprintCallable, Category = "DialogueSample")
	FGuid RequestDialogue(FGameplayTag Target, UDialogueStoryDefinition* Story, FDialogueStoryProgress Progress);
	// 커스텀 카탈로그도 동일 실행 경계를 사용한다.
	FGuid RequestCandidates(const TArray<FDialogueBlockCandidate>& Candidates, FGameplayTag Target,
		UDialogueStoryDefinition* Story, const FDialogueStoryProgress& Progress);
	// 게임과 대화 snapshot을 같은 저장에서 복원한 뒤 호출한다. 게임 진행도도 게임이 복원해야 한다.
	UFUNCTION(BlueprintCallable, Category = "DialogueSample")
	bool RestoreDialogue(const FDialogueCheckpointSnapshot& Snapshot, UDialogueReplacementMap* Map);
	UFUNCTION(BlueprintCallable, Category = "DialogueSample")
	bool EndConversation();
	// 최초 자동 효과의 접수 실패도 같은 요청 ID로 재시도한다. 디스크 실패는 Queue 재시도다.
	UFUNCTION(BlueprintCallable, Category = "DialogueSample")
	bool RetryCapture();
	// 기존 게임 UI를 자동 변경하지 않는다. 호출자가 로컬 플레이어와 입력 모드를 소유한다.
	UFUNCTION(BlueprintCallable, Category = "DialogueSample")
	bool ShowPresentation(APlayerController* Player);
	UFUNCTION(BlueprintPure, Category = "DialogueSample")
	UDialogueBlockLoader* GetLoader() const { return Loader; }
	UFUNCTION(BlueprintPure, Category = "DialogueSample")
	UDialogueRestoreController* GetRestoreController() const { return RestoreController; }
	UPROPERTY(BlueprintAssignable, Category = "DialogueSample") FDialogueSampleHostResult OnStarted;
private:
	bool CanRequest() const;
	void Loaded(const FDialogueBlockLoadResult& Result);
	void Presented(const FDialoguePresentation& Presentation);
	void RecordCompletion();
	UPROPERTY() TObjectPtr<UDialogueSubsystem> Dialogue;
	UPROPERTY() TObjectPtr<UDialogueBlockLoader> Loader;
	UPROPERTY() TObjectPtr<UDialogueRestoreController> RestoreController;
	UPROPERTY() TObjectPtr<UObject> StateProvider;
	UPROPERTY() TObjectPtr<UDialogueDiagnosticRouter> Router;
	UPROPERTY() TObjectPtr<UDialogueSaveQueue> Queue;
	UPROPERTY() TObjectPtr<UDialoguePresentationWidget> PresentationWidget;
	UPROPERTY() TObjectPtr<UDialogueSavingWidget> SavingWidget;
	FGameplayTag Target;
	FGuid ActiveRequest;
	FDialogueBlockReference ActiveReference;
	bool bPendingStart = false;
	bool bStarting = false;
	bool bHandlingRequest = false;
	bool bCompletionPending = false;
};
