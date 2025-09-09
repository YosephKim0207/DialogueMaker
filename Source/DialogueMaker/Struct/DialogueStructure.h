#pragma once

#include "CoreMinimal.h"
#include "DialogueConditionEvalCriteria.h"
#include "Engine/DataTable.h"
#include "DialogueStructure.generated.h"


USTRUCT(BlueprintType)
struct FDialogueChoice
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText ResponseText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FDialogueConditionEvalCriteria SelectableChoiceEvalCriteria;

	FDialogueChoice(){}
	explicit FDialogueChoice(const FText& NewResponseText) : ResponseText(NewResponseText) {}
	FDialogueChoice(const FText& NewResponseText, const FDialogueConditionEvalCriteria& NewSelectableChoiceEvalCriteria)
	: ResponseText(NewResponseText), SelectableChoiceEvalCriteria(NewSelectableChoiceEvalCriteria) {}
	
	bool IsPossibleToShow(const FPlayerCondition& PlayerEvalCondition) const
	{
		// Player의 레벨이 RequiredLevel를 충족하는지 판별
		if (PlayerEvalCondition.PlayerLevel < SelectableChoiceEvalCriteria.RequiredLevel)
		{
			return false;
		}

		// RequiredTagQuery가 비어있는 경우 별다른 조건이 없다고 보고 true 반환, Matches의 경우 IsEmpty면 false를 반환하기 때문에 IsEmpty 상황을 위한 별도 분기 생성
		if (SelectableChoiceEvalCriteria.RequiredTagQuery.IsEmpty())
		{
			UE_LOG(LogTemp, Log, TEXT("FDialogueChoice::IsPossibleToShow %s RequiredTagQuery is Empty"), *ResponseText.ToString());
			return true;
		}

		// Player가 갖고있는 태그들이 DialoguePassCondition의 TagQuery를 충족하는지 판별
		if (SelectableChoiceEvalCriteria.RequiredTagQuery.Matches(PlayerEvalCondition.PlayerOwnedTags) == false)
		{
			return false;
		}
	
		return true;
	}
};

// USTRUCT(BlueprintType)
// struct FDialogueStructure : public FTableRowBase
// {
// 	GENERATED_BODY()
//
// 	UPROPERTY(EditAnywhere, BlueprintReadWrite)
// 	FGuid CurrentDialogueId;
// 	UPROPERTY(EditAnywhere, BlueprintReadWrite)
// 	FText SpeakerName;
// 	UPROPERTY(EditAnywhere, BlueprintReadWrite)
// 	FText DialogueText;
// 	UPROPERTY(EditAnywhere, BlueprintReadWrite)
// 	TArray<FDialogueChoice> Choices;
// 	UPROPERTY(EditAnywhere, BlueprintReadWrite)
// 	FGuid NextDialogueId;
// 	// TODO 대화가 이번에 끝나고 다음 이벤트 때 재개되는 경우 bool로 쓰던지 GameplayTag 활용하던지
// 	// TODO 대화에서 이벤트 발생시 GameplayTag 활용하기
// };