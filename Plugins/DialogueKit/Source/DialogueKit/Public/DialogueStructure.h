#pragma once

#include "CoreMinimal.h"
#include "Struct/DialogueConditionEvalCriteria.h"
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
		// Player???堧波??RequiredLevel毳?於╈”?橂姅歆� ?愲硠
		if (PlayerEvalCondition.PlayerLevel < SelectableChoiceEvalCriteria.RequiredLevel)
		{
			return false;
		}

		// RequiredTagQuery臧� 牍勳柎?堧姅 瓴届毎 氤勲嫟毳?臁瓣贝???嗠嫟瓿?氤搓碃 true 氚橅櫂, Matches??瓴届毎 IsEmpty氅?false毳?氚橅櫂?橁赴 ?岆??IsEmpty ?來櫓???勴暅 氤勲弰 攵勱赴 ?濎劚
		if (SelectableChoiceEvalCriteria.RequiredTagQuery.IsEmpty())
		{
			UE_LOG(LogTemp, Log, TEXT("FDialogueChoice::IsPossibleToShow %s RequiredTagQuery is Empty"), *ResponseText.ToString());
			return true;
		}

		// Player臧� 臧栮碃?堧姅 ?滉犯?れ澊 DialoguePassCondition??TagQuery毳?於╈”?橂姅歆� ?愲硠
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
// 	// TODO ?�?旉? ?措矆???濍倶瓿??れ潓 ?措菠?????皽?橂姅 瓴届毎 bool搿??半崢歆� GameplayTag ?滌毄?橂崢歆�
// 	// TODO ?�?旍棎???措菠??氚滌儩??GameplayTag ?滌毄?橁赴
// };
