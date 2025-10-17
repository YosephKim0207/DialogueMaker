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

    UPROPERTY(EditAnywhere, Category = "DialogueKit|FDialogueConditionEvalCriteria|Level")
    int32 RequiredLevel = 0;

    UPROPERTY(EditAnywhere, Category = "DialogueKit|FDialogueConditionEvalCriteria|Tags")
    FGameplayTagQuery RequiredTagQuery;
};
