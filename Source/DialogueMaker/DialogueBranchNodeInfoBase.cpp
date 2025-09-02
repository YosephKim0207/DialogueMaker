#include "DialogueBranchNodeInfoBase.h"

#include "DialogueSubsystem.h"

DEFINE_LOG_CATEGORY_STATIC(DialogueBranchNodeInfoBase, Log, All);

bool UDialogueBranchNodeInfoBase::ConditionCheck(const FPlayerEvalCondition& PlayerEvalCondition) const
{
	if (DialoguePassCondition.RequiredTagQuery.IsEmpty())
	{
		UE_LOG(DialogueBranchNodeInfoBase, Log, TEXT("UDialogueBranchNodeInfoBase::ConditionCheck RequiredTagQuery is Empty"));
	}

	// Player의 레벨이 RequiredLevel를 충족하는지 판별
	if (PlayerEvalCondition.PlayerLevel < DialoguePassCondition.RequiredLevel)
	{
		return false;
	}

	// Player가 갖고있는 태그들이 DialoguePassCondition의 TagQuery를 충족하는지 판별
	if (DialoguePassCondition.RequiredTagQuery.Matches(PlayerEvalCondition.PlayerOwnedTags) == false)
	{
		return false;
	}
	
	return true;
}
