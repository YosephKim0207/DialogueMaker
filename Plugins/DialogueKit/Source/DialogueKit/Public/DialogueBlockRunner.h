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
struct DIALOGUEKIT_API FDialoguePresentation
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "DialogueKit|Run")
	EDialogueRunState State = EDialogueRunState::Idle;

	UPROPERTY(BlueprintReadOnly, Category = "DialogueKit|Run")
	FGuid NodeGuid;

	UPROPERTY(BlueprintReadOnly, Category = "DialogueKit|Run")
	TObjectPtr<UDialogueNodeInfo> Line;

	UPROPERTY(BlueprintReadOnly, Category = "DialogueKit|Run")
	EDialogueType DirectNextType = EDialogueType::Unknown;

	UPROPERTY(BlueprintReadOnly, Category = "DialogueKit|Run")
	FName ErrorCode;
};

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

	// 현재 발화 이후로 진행한다. 실패는 정상 종료가 아니며 게임이 반환값/상태로 안내한다.
	UFUNCTION(BlueprintCallable, Category = "DialogueKit|Run")
	bool AdvanceLine();

	// 마지막 표시 상태를 조회한다. 조회로 커서·효과·열람·저장을 변경하지 않는다.
	UFUNCTION(BlueprintPure, Category = "DialogueKit|Run")
	FDialoguePresentation GetPresentation() const { return Presentation; }

	// 직접 연결 한 단계만 확인하며 Branch 조건이나 효과를 실행하지 않는다.
	UFUNCTION(BlueprintPure, Category = "DialogueKit|Run")
	EDialogueType GetDirectNextType() const;

	UPROPERTY(BlueprintAssignable, Category = "DialogueKit|Run")
	FDialoguePresentationEvent OnPresentationChanged;

	FDialoguePresentationNative OnPresentationChangedNative;

private:
	// 연결의 방향/소유/단일 목적지를 검사하고 실패 시 nullptr를 반환한다.
	UDialogueRuntimeNode* ResolveOutput(const UDialogueRuntimeNode* Node, int32 Index) const;
	// 자동 제어 노드를 통과하고 입력 대기 또는 종료 상태를 게시한다.
	bool Pump(UDialogueRuntimeNode* Node);
	// 오류는 중단 상태로 게시하고 마지막 게임 저장을 건드리지 않는다.
	bool Fail(FName Code);
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

	bool bProcessing = false;
};
