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
		// Player???ˆë²¨??RequiredLevelë¥?ì¶©ì¡±?˜ëŠ”ì§€ ?ë³„
		if (PlayerEvalCondition.PlayerLevel < SelectableChoiceEvalCriteria.RequiredLevel)
		{
			return false;
		}

		// RequiredTagQueryê°€ ë¹„ì–´?ˆëŠ” ê²½ìš° ë³„ë‹¤ë¥?ì¡°ê±´???†ë‹¤ê³?ë³´ê³  true ë°˜í™˜, Matches??ê²½ìš° IsEmptyë©?falseë¥?ë°˜í™˜?˜ê¸° ?Œë¬¸??IsEmpty ?í™©???„í•œ ë³„ë„ ë¶„ê¸° ?ì„±
		if (SelectableChoiceEvalCriteria.RequiredTagQuery.IsEmpty())
		{
			UE_LOG(LogTemp, Log, TEXT("FDialogueChoice::IsPossibleToShow %s RequiredTagQuery is Empty"), *ResponseText.ToString());
			return true;
		}

		// Playerê°€ ê°–ê³ ?ˆëŠ” ?œê·¸?¤ì´ DialoguePassCondition??TagQueryë¥?ì¶©ì¡±?˜ëŠ”ì§€ ?ë³„
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
// 	// TODO ?€?”ê? ?´ë²ˆ???ë‚˜ê³??¤ìŒ ?´ë²¤?????¬ê°œ?˜ëŠ” ê²½ìš° boolë¡??°ë˜ì§€ GameplayTag ?œìš©?˜ë˜ì§€
// 	// TODO ?€?”ì—???´ë²¤??ë°œìƒ??GameplayTag ?œìš©?˜ê¸°
// };
