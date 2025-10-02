#pragma once

#include "CoreMinimal.h"
#include "DialogueNodeInfoBase.h"
#include "Struct/DialogueConditionEvalCriteria.h"
#include "DialogueBranchNodeInfoBase.generated.h"

UCLASS()
class DIALOGUEMAKER_API UDialogueBranchNodeInfoBase : public UDialogueNodeInfoBase
{
	GENERATED_BODY()
public:
	bool ConditionCheck(const FPlayerCondition& PlayerEvalCondition) const;
	
	UPROPERTY(EditAnywhere)
	FDialogueConditionEvalCriteria DialoguePassCondition;
};
