#pragma once

#include "CoreMinimal.h"
#include "DialogueNodeInfoBase.h"
#include "DialogueConditionTypes.h"
#include "Struct/DialogueConditionEvalCriteria.h"
#include "DialogueBranchNodeInfoBase.generated.h"

UCLASS()
class DIALOGUEKIT_API UDialogueBranchNodeInfoBase : public UDialogueNodeInfoBase
{
	GENERATED_BODY()
public:
	// 신규 Block 실행기는 이 공통 조건만 평가한다. 아래 구형 조건은 기존 에셋 읽기용이다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DialogueKit|Branch")
	FDialogueConditionSet Conditions;

	bool ConditionCheck(const FPlayerCondition& PlayerEvalCondition) const;
	
    UPROPERTY(EditAnywhere, Category = "DialogueKit|Legacy")
    FDialogueConditionEvalCriteria DialoguePassCondition;
};
