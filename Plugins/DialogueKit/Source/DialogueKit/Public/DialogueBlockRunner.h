#pragma once

#include "CoreMinimal.h"
#include "DialogueGraph.h"
#include "DialogueNodeInfo.h"
#include "DialogueDiagnostics.h"
#include "DialogueBlockRunner.generated.h"

UENUM(BlueprintType)
enum class EDialogueRunState : uint8
{
	Idle,
	Running,
	WaitingLine,
	WaitingChoice,
	Completed,
	Failed
};

USTRUCT(BlueprintType)
struct DIALOGUEKIT_API FDialogueChoiceView
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly, Category = "DialogueKit|Run")
	FGuid ChoiceId;
	UPROPERTY(BlueprintReadOnly, Category = "DialogueKit|Run")
	FText Text;
};

USTRUCT(BlueprintType)
struct DIALOGUEKIT_API FDialoguePresentation
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "DialogueKit|Run")
	EDialogueRunState State = EDialogueRunState::Idle;

	UPROPERTY(BlueprintReadOnly, Category = "DialogueKit|Run")
	FGuid NodeGuid;

	UPROPERTY(BlueprintReadOnly, Category = "DialogueKit|Run")
	FGuid LineNodeGuid;

	UPROPERTY(BlueprintReadOnly, Category = "DialogueKit|Run")
	TObjectPtr<UDialogueNodeInfo> Line;

	UPROPERTY(BlueprintReadOnly, Category = "DialogueKit|Run")
	EDialogueType DirectNextType = EDialogueType::Unknown;

	UPROPERTY(BlueprintReadOnly, Category = "DialogueKit|Run")
	FName ErrorCode;

	UPROPERTY(BlueprintReadOnly, Category = "DialogueKit|Run")
	FGuid ChoiceNodeGuid;

	UPROPERTY(BlueprintReadOnly, Category = "DialogueKit|Run")
	TArray<FDialogueChoiceView> Choices;
};

// 효과 성공 직후의 원점/다음 위치다. 디스크 저장 완료를 뜻하지 않는다.
USTRUCT(BlueprintType)
struct DIALOGUEKIT_API FDialogueExecutionBoundary
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly, Category = "DialogueKit|Run")
	FDialogueDiagnosticContext Context;
	UPROPERTY(BlueprintReadOnly, Category = "DialogueKit|Run")
	FGuid ChoiceId;
	UPROPERTY(BlueprintReadOnly, Category = "DialogueKit|Run")
	FGuid NextNodeGuid;
	UPROPERTY(BlueprintReadOnly, Category = "DialogueKit|Run")
	bool bHasEffects = false;
	UPROPERTY(BlueprintReadOnly, Category = "DialogueKit|Run")
	bool bSaveRequested = false;
	UPROPERTY(BlueprintReadOnly, Category = "DialogueKit|Run")
	bool bBlockCompleted = false;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDialogueExecutionBoundaryEvent, const FDialogueExecutionBoundary&, Boundary);
DECLARE_MULTICAST_DELEGATE_OneParam(FDialogueExecutionBoundaryNative, const FDialogueExecutionBoundary&);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDialoguePresentationEvent, const FDialoguePresentation&, Presentation);
DECLARE_MULTICAST_DELEGATE_OneParam(FDialoguePresentationNative, const FDialoguePresentation&);

// 로드된 Block 하나만 해석한다. 게임 저장/후보 선정/음향·입력 장치를 소유하지 않는다.
UCLASS(BlueprintType)
class DIALOGUEKIT_API UDialogueBlockRunner : public UObject
{
	GENERATED_BODY()
public:
	// 실행 중 재진입은 거부한다. 게임은 제공자를 UObject/Blueprint 구현으로 교체할 수 있다.
	UFUNCTION(BlueprintCallable, Category = "DialogueKit|Run")
	bool StartBlock(UDialogueGraph* Block, UObject* NewStateProvider, UObject* NewEffectProvider, UDialogueDiagnosticRouter* NewRouter);

	// 콜백 중 재진입까지 포함하여 새 시작 가능 여부를 조회한다.
	UFUNCTION(BlueprintPure, Category = "DialogueKit|Run")
	bool CanStartBlock() const;


	// 현재 발화 이후로 진행한다. 실패는 정상 종료가 아니며 게임이 반환값/상태로 안내한다.
	UFUNCTION(BlueprintCallable, Category = "DialogueKit|Run")
	bool AdvanceLine();

	// 표시 순서가 아닌 고정 ID로 선택한다.
	UFUNCTION(BlueprintCallable, Category = "DialogueKit|Run")
	bool SelectChoice(FGuid ChoiceId);

	// 마지막 표시 상태를 조회한다. 조회로 커서·효과·열람·저장을 변경하지 않는다.
	UFUNCTION(BlueprintPure, Category = "DialogueKit|Run")
	FDialoguePresentation GetPresentation() const { return Presentation; }

	// 직접 연결 한 단계만 확인하며 Branch 조건이나 효과를 실행하지 않는다.
	UFUNCTION(BlueprintPure, Category = "DialogueKit|Run")
	EDialogueType GetDirectNextType() const;

	UPROPERTY(BlueprintAssignable, Category = "DialogueKit|Run")
	FDialoguePresentationEvent OnPresentationChanged;

	FDialoguePresentationNative OnPresentationChangedNative;

	// 게임은 이 동기 경계에서 상태/선택기록/다음 위치를 캐시에 반영한다.
	// bSaveRequested일 때만 불변 복사본을 FIFO에 제출한다.
	// 디스크 완료를 기다리지 않는다. S2 자체는 실제 파일 저장을 실행하지 않는다.
	UPROPERTY(BlueprintAssignable, Category = "DialogueKit|Run")
	FDialogueExecutionBoundaryEvent OnExecutionBoundary;
	FDialogueExecutionBoundaryNative OnExecutionBoundaryNative;

private:
	// 연결의 방향/소유/단일 목적지를 검사하고 실패 시 nullptr를 반환한다.
	UDialogueRuntimeNode* ResolveOutput(const UDialogueRuntimeNode* Node, int32 Index) const;
	// 자동 제어 노드를 통과하고 입력 대기 또는 종료 상태를 게시한다.
	bool Pump(UDialogueRuntimeNode* Node);
	// 선택지 조회는 진행 위치나 효과를 변경하지 않는다.
	bool GetChoiceViews(UDialogueRuntimeNode* Node, TArray<FDialogueChoiceView>& OutViews);
	// 오류는 중단 상태로 게시하고 마지막 게임 저장을 건드리지 않는다.
	bool Fail(FName Code);
	// 정상 완료와 저장 요청 경계를 한 경로로 통일한다.
	bool Complete(bool bSaveRequested);
	// 게임의 외부 로그 전송은 Router 구독자로 연결한다.
	void Report(FName Code, const FString& Handling);
	// 동기 콜백의 재진입은 호출 측 처리 구간에서 차단한다.
	void Publish();

	UPROPERTY()
	TObjectPtr<UDialogueGraph> ActiveBlock;

	UPROPERTY()
	TObjectPtr<UObject> StateProvider;

	UPROPERTY()
	TObjectPtr<UObject> EffectProvider;

	UPROPERTY()
	TObjectPtr<UDialogueDiagnosticRouter> Router;

	UPROPERTY()
	FDialoguePresentation Presentation;

	UPROPERTY()
	TMap<FGuid, UDialogueRuntimeNode*> Nodes;

	// 다음 위치 검사가 끝난 뒤에만 효과를 요청하고 성공 경계를 게시한다.
	bool ApplyEffects(UDialogueRuntimeNode* Origin, UDialogueRuntimeNode* Next, const TArray<struct FDialogueEffect>& Effects, FGuid ChoiceId, bool bSaveRequested);

	bool bProcessing = false;
};
