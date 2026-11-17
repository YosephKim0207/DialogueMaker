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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DialogueKit|Condition", meta = (ToolTip = "상태 종류입니다. Tag는 태그 유무, Value는 레벨·호감도 등의 수치, Count는 아이템 보유량 등의 정수 수량입니다. 선택한 종류의 키와 값만 사용합니다."))
	EDialogueStateKind Kind = EDialogueStateKind::Tag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DialogueKit|Condition", meta = (ToolTip = "상태를 조회하거나 변경할 대상의 게임 정의 ID입니다(예: Player, NPC_Merchant). Block 대화 대상 태그와 다릅니다. None은 게임 제공자가 전역 상태로 처리할 수 있습니다."))
	FName TargetId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DialogueKit|Condition", meta = (ToolTip = "Value/Count 항목의 게임 정의 ID입니다(예: Level, HealthPotion). 표시 이름이 아니라 게임 제공자가 처리하는 정확한 키를 입력합니다. Tag 종류에서는 사용하지 않습니다."))
	FName ItemId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DialogueKit|Condition", meta = (ToolTip = "Tag 종류에서 조회하거나 변경할 GameplayTag입니다. 프로젝트에 등록된 태그를 선택합니다. Value/Count 종류에서는 사용하지 않습니다."))
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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DialogueKit|Condition", meta = (ToolTip = "게임 상태를 식별하는 종류·대상·항목 키입니다. 게임 제공자의 정의 및 조회/변경 코드와 일치해야 합니다."))
	FDialogueStateKey Key;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DialogueKit|Condition", meta = (ToolTip = "제작자에게 보여줄 항목 이름입니다. 저장 및 상태 조회에 사용하는 Key 식별자와 구분됩니다."))
	FText DisplayName;
};

USTRUCT(BlueprintType)
struct DIALOGUEKIT_API FDialogueCondition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DialogueKit|Condition", meta = (ToolTip = "게임 상태를 식별하는 종류·대상·항목 키입니다. 게임 제공자의 정의 및 조회/변경 코드와 일치해야 합니다."))
	FDialogueStateKey Key;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DialogueKit|Condition", meta = (ToolTip = "현재 값과 기준값을 비교합니다. Equal(같음), NotEqual(다름), Less(미만), LessOrEqual(이하), Greater(초과), GreaterOrEqual(이상). Tag는 Equal/NotEqual만 지원합니다."))
	EDialogueComparison Comparison = EDialogueComparison::Equal;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DialogueKit|Condition", meta = (ToolTip = "Kind가 Tag일 때 비교할 기준입니다. 체크는 True, 해제는 False입니다. Equal과 체크를 함께 쓰면 태그 보유를 요구합니다. 다른 종류에서는 무시합니다."))
	bool bTagOperand = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DialogueKit|Condition", meta = (ToolTip = "Kind가 Value일 때 비교할 수치 기준값입니다. 예: GreaterOrEqual과 10이면 현재 값이 10 이상이어야 합니다. 다른 종류에서는 무시합니다."))
	double ValueOperand = 0.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DialogueKit|Condition", meta = (ToolTip = "Kind가 Count일 때 비교할 정수 기준값입니다. 예: GreaterOrEqual과 3이면 보유 수량 3개 이상입니다. 다른 종류에서는 무시합니다."))
	// 비교 기준은 음수도 허용한다. 현재 보유 수량은 0 이상이며 음수 기준과도 그대로 비교한다.
	int64 CountOperand = 0;
};

USTRUCT(BlueprintType)
struct DIALOGUEKIT_API FDialogueConditionSet
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DialogueKit|Condition", meta = (ToolTip = "여러 조건을 합치는 방식입니다. All은 모두 만족(AND), Any는 하나 이상 만족(OR)입니다. 빈 목록은 두 방식 모두 통과합니다."))
	EDialogueConditionCombine Combine = EDialogueConditionCombine::All;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DialogueKit|Condition", meta = (ToolTip = "평가할 조건 목록입니다. 빈 목록은 통과하며 Branch에서는 추가로 진단을 남깁니다. 조회 오류 항목은 진단 후 True로 처리하고 나머지 조건과 종합합니다."))
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
