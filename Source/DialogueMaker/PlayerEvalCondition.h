#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "PlayerEvalCondition.generated.h"

USTRUCT()
struct DIALOGUEMAKER_API FPlayerEvalCondition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	int32 PlayerLevel = 0;

	UPROPERTY(EditAnywhere)
	FGameplayTagContainer PlayerOwnedTags;
};
