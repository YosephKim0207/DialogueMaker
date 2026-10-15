#pragma once

#include "CoreMinimal.h"
#include "DialogueConditionTypes.h"
#include "UObject/Interface.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "DialogueEffectContract.generated.h"

UENUM(BlueprintType)
enum class EDialogueEffectOperation : uint8
{
	Add,
	Set,
	Remove
};

UENUM(BlueprintType)
enum class EDialogueEffectStatus : uint8
{
	Failed,
	Applied,
	AlreadyApplied
};

USTRUCT(BlueprintType)
struct DIALOGUEKIT_API FDialogueEffect
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DialogueKit|Effects")
	FDialogueStateKey Key;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DialogueKit|Effects")
	EDialogueEffectOperation Operation = EDialogueEffectOperation::Add;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DialogueKit|Effects")
	double Value = 0.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DialogueKit|Effects")
	int64 Count = 0;

	// 종류별 허용 연산과 활성 값만 검사한다.
	bool IsValid() const;
	// 비활성 필드를 제외한 효과의 의미를 비교한다.
	bool operator==(const FDialogueEffect& Other) const;
};

USTRUCT(BlueprintType)
struct DIALOGUEKIT_API FDialogueEffectBatch
{
	GENERATED_BODY()

	// ExecutionId는 이 Context 안의 값 하나만 사용한다. 정적 ChoiceId와 다르다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DialogueKit|Effects")
	FDialogueDiagnosticContext Context;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DialogueKit|Effects")
	TArray<FDialogueEffect> Effects;

	// StateChange는 빈 값이며 Choice에서 발생한 효과는 원래 선택지 ID를 보존한다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DialogueKit|Effects")
	FGuid ChoiceId;

	// 실행 ID와 비어 있지 않은 유효 효과 묶음을 요구한다.
	bool IsValid() const;
	// 추적용 SaveRequestId는 제외하고 발생 위치와 효과 순서를 비교한다.
	bool HasSamePayload(const FDialogueEffectBatch& Other) const;
};

USTRUCT(BlueprintType)
struct DIALOGUEKIT_API FDialogueEffectResult
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DialogueKit|Effects")
	EDialogueEffectStatus Status = EDialogueEffectStatus::Failed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DialogueKit|Effects")
	FName ErrorCode = TEXT("ProviderNotImplemented");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DialogueKit|Effects")
	FString Message;

	// 성공은 최초 적용 또는 이미 적용한 같은 요청의 재전달이다.
	bool IsSuccess() const { return Status == EDialogueEffectStatus::Applied || Status == EDialogueEffectStatus::AlreadyApplied; }
};

UINTERFACE(BlueprintType)
class DIALOGUEKIT_API UDialogueEffectProvider : public UInterface
{
	GENERATED_BODY()
};

class DIALOGUEKIT_API IDialogueEffectProvider
{
	GENERATED_BODY()
public:
	// 게임은 전량 성공/실패 무변경을 보장하고 같은 실행 ID는 기존 결과를 반환한다.
	// 성공 상태와 실행 기록을 같은 저장 스냅샷에 포함한다. 인터페이스만으로 원자성이 생기지 않는다.
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "DialogueKit|Effects")
	FDialogueEffectResult ApplyDialogueEffects(const FDialogueEffectBatch& Batch);
	virtual FDialogueEffectResult ApplyDialogueEffects_Implementation(const FDialogueEffectBatch& Batch);
};

UCLASS()
class DIALOGUEKIT_API UDialogueEffectLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	// 실패는 효과 무변경 계약이다. UI/재시도/후속 흐름은 반환값으로 게임이 결정한다.
	// 성공 직후 게임 상태·선택/실행 기록·후속 재개 위치를 함께 캡처하여 공통 FIFO 저장에 요청해야 한다.
	UFUNCTION(BlueprintCallable, Category = "DialogueKit|Effects")
	static FDialogueEffectResult ApplyEffectBatch(const FDialogueEffectBatch& Batch, UObject* Provider, UDialogueDiagnosticRouter* Router);
};
