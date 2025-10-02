#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "DialogueConditionEvalCriteria.generated.h"

USTRUCT()
struct DIALOGUEMAKER_API FPlayerCondition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "PlayerCondition")
	int32 PlayerLevel = 0;

	UPROPERTY(EditAnywhere, Category = "PlayerCondition")
	FGameplayTagContainer PlayerOwnedTags;
};

USTRUCT(BlueprintType)
struct DIALOGUEMAKER_API FDialogueConditionEvalCriteria
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "PlayerCondition")
	int32 RequiredLevel = 0;	

	UPROPERTY(EditAnywhere, Category = "PlayerCondition")
	FGameplayTagQuery RequiredTagQuery;
};
