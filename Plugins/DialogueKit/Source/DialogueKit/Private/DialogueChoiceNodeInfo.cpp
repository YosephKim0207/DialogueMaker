#include "DialogueChoiceNodeInfo.h"

// 추가/복제된 항목만 새 ID로 구분한다.
void UDialogueChoiceNodeInfo::EnsureOptionIds()
{
	TSet<FGuid> Ids;
	for (FDialogueOption& Option : Options)
	{
		if (Option.ChoiceId.IsValid() == false || Ids.Contains(Option.ChoiceId))
		{
			Option.ChoiceId = FGuid::NewGuid();
		}
		Ids.Add(Option.ChoiceId);
	}
}

// 빈 목록은 실행 방어 경로가 처리하며 중복/모호한 구성은 거부한다.
bool UDialogueChoiceNodeInfo::HasValidOptions() const
{
	TSet<FGuid> Ids;
	int32 Defaults = 0;
	for (const FDialogueOption& Option : Options)
	{
		if (Option.ChoiceId.IsValid() == false || Ids.Contains(Option.ChoiceId))
		{
			return false;
		}
		Ids.Add(Option.ChoiceId);
		Defaults += Option.bDefault ? 1 : 0;
	}
	return Defaults <= 1;
}
