#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "DialogueConditionEvalCriteria.generated.h"

USTRUCT()
struct DIALOGUEMAKER_API FPlayerCondition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	int32 PlayerLevel = 0;

	UPROPERTY(EditAnywhere)
	FGameplayTagContainer PlayerOwnedTags;
};

USTRUCT(BlueprintType)
struct DIALOGUEMAKER_API FDialogueConditionEvalCriteria
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	int32 RequiredLevel = 0;	

	UPROPERTY(EditAnywhere)
	FGameplayTagQuery RequiredTagQuery;
};
