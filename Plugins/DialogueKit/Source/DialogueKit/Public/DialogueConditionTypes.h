#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "DialogueDiagnostics.h"
#include "DialogueConditionTypes.generated.h"

UENUM(BlueprintType)
enum class EDialogueStateKind : uint8
{
	Tag,
	Value,
	Count
};

UENUM(BlueprintType)
enum class EDialogueComparison : uint8
{
	Equal,
	NotEqual,
	Less,
	LessOrEqual,
	Greater,
	GreaterOrEqual
};

UENUM(BlueprintType)
enum class EDialogueConditionCombine : uint8
{
	All,
	Any
};

// 대화 진입 대상과 별개인 게임 상태 대상/항목 키다. None 대상은 게임의 전역 상태로 사용 가능하다.
USTRUCT(BlueprintType)
struct DIALOGUEKIT_API FDialogueStateKey
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DialogueKit|Condition")
	EDialogueStateKind Kind = EDialogueStateKind::Tag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DialogueKit|Condition")
	FName TargetId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DialogueKit|Condition")
	FName ItemId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DialogueKit|Condition")
	FGameplayTag Tag;

	// 종류별 실제 키가 있어야 하며 알 수 없는 종류는 거부한다.
	bool IsValid() const;
	// 비활성 필드는 동일성에 영향을 주지 않는다.
	bool operator==(const FDialogueStateKey& Other) const;
	friend uint32 GetTypeHash(const FDialogueStateKey& Key);
};

// 게임 DataTable을 직접 사용하거나 게임별 표를 이 행 구조로 변환할 수 있다.
USTRUCT(BlueprintType)
struct DIALOGUEKIT_API FDialogueStateDefinition : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DialogueKit|Condition")
	FDialogueStateKey Key;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DialogueKit|Condition")
	FText DisplayName;
};

USTRUCT(BlueprintType)
struct DIALOGUEKIT_API FDialogueCondition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DialogueKit|Condition")
	FDialogueStateKey Key;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DialogueKit|Condition")
	EDialogueComparison Comparison = EDialogueComparison::Equal;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DialogueKit|Condition")
	bool bTagOperand = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DialogueKit|Condition")
	double ValueOperand = 0.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DialogueKit|Condition")
	// 비교 기준은 음수도 허용한다. 현재 보유 수량은 0 이상이며 음수 기준과도 그대로 비교한다.
	int64 CountOperand = 0;
};

USTRUCT(BlueprintType)
struct DIALOGUEKIT_API FDialogueConditionSet
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DialogueKit|Condition")
	EDialogueConditionCombine Combine = EDialogueConditionCombine::All;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DialogueKit|Condition")
	TArray<FDialogueCondition> Conditions;
};

USTRUCT(BlueprintType)
struct DIALOGUEKIT_API FDialogueConditionResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "DialogueKit|Condition")
	bool bPassed = true;

	UPROPERTY(BlueprintReadOnly, Category = "DialogueKit|Condition")
	bool bHadErrors = false;

	UPROPERTY(BlueprintReadOnly, Category = "DialogueKit|Condition")
	TArray<FDialogueDiagnostic> Diagnostics;
};
