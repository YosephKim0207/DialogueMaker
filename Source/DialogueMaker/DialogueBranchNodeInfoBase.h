#pragma once

#include "CoreMinimal.h"
#include "DialogueNodeInfoBase.h"
#include "GameplayTagContainer.h"
#include "PlayerEvalCondition.h"
#include "DialogueBranchNodeInfoBase.generated.h"

USTRUCT(BlueprintType)
struct FDialogueCondition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	int32 RequiredLevel = 0;	

	UPROPERTY(EditAnywhere)
	FGameplayTagQuery RequiredTagQuery;
};

UCLASS()
class DIALOGUEMAKER_API UDialogueBranchNodeInfoBase : public UDialogueNodeInfoBase
{
	GENERATED_BODY()
public:
	bool ConditionCheck(const FPlayerEvalCondition& PlayerEvalCondition) const;
	
	UPROPERTY(EditAnywhere)
	FDialogueCondition DialoguePassCondition;
};
