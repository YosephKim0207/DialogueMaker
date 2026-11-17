#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "DialogueConditionEvalCriteria.generated.h"

USTRUCT()
struct DIALOGUEKIT_API FPlayerCondition
{
	GENERATED_BODY()

    UPROPERTY(EditAnywhere, Category = "DialogueKit|FPlayerCondition|Player")
    int32 PlayerLevel = 0;

    UPROPERTY(EditAnywhere, Category = "DialogueKit|FPlayerCondition|Tags")
    FGameplayTagContainer PlayerOwnedTags;
};

USTRUCT(BlueprintType)
struct DIALOGUEKIT_API FDialogueConditionEvalCriteria
{
	GENERATED_BODY()

    UPROPERTY(EditAnywhere, Category = "DialogueKit|FDialogueConditionEvalCriteria|Level", meta = (ToolTip = "구형 조건의 최소 플레이어 레벨입니다. 신규 Conditions에서는 Value 종류와 게임이 정의한 레벨 ID를 사용합니다."))
    int32 RequiredLevel = 0;

    UPROPERTY(EditAnywhere, Category = "DialogueKit|FDialogueConditionEvalCriteria|Tags", meta = (ToolTip = "구형 조건의 GameplayTag 쿼리입니다. 신규 Branch 실행기는 이 자료를 평가하지 않고 공통 Conditions를 사용합니다."))
    FGameplayTagQuery RequiredTagQuery;
};
