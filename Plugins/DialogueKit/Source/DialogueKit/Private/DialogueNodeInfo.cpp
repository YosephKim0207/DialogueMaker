#include "DialogueNodeInfo.h"

const FText& UDialogueNodeInfo::GetTitle() const
{
	return Title;
}

const FText& UDialogueNodeInfo::GetDialogueText() const
{
	return DialogueText;
}

const TArray<FDialogueChoice>& UDialogueNodeInfo::GetDialogueChoices() const
{
	return DialogueChoices;
}

void UDialogueNodeInfo::AddDialogueChoice(const FDialogueChoice& DialogueChoice)
{
	// 자기 배열의 선택을 복제 입력으로 받아도 재할당 중 참조가 무효화되지 않게 한다.
	FDialogueChoice NewChoice = DialogueChoice;
	DialogueChoices.Add(MoveTemp(NewChoice));
	EnsureChoiceIds();
}

void UDialogueNodeInfo::RemoveDialogueChoiceAt(int32 Index)
{
	DialogueChoices.RemoveAt(Index);
}

bool UDialogueNodeInfo::IsDialogueAlreadyShown() const
{
	return bIsShown;
}

void UDialogueNodeInfo::SetShownCondition(const bool NewCondition)
{
	bIsShown = NewCondition;
}

const FSpeakerEmotePair UDialogueNodeInfo::GetSpeakerEmotePair() const
{
	return FSpeakerEmotePair(SpeakerID, PortraitData.EmoteType);
}

const FPortraitData UDialogueNodeInfo::GetPortraitData() const
{
	return PortraitData;
}

const TArray<FPortraitActionData>& UDialogueNodeInfo::GetPortraitActionDatas() const
{
	return PortraitActionDatas;
}

// 새 선택이나 Details 복제 항목만 새 ID를 얻고 기존 선택의 ID는 유지한다.
void UDialogueNodeInfo::EnsureChoiceIds()
{
	TSet<FGuid> UsedIds;
	for (FDialogueChoice& Choice : DialogueChoices)
	{
		if (Choice.ChoiceId.IsValid() == false || UsedIds.Contains(Choice.ChoiceId))
		{
			Choice.ChoiceId = FGuid::NewGuid();
		}
		UsedIds.Add(Choice.ChoiceId);
	}
}

// 대사를 미열람으로 갱신해도 노드 ID/실행 위치/선택 이력을 변경하지 않는다.
bool UDialogueNodeInfo::AdvanceReadVersion()
{
	if (ReadVersion < 1 || ReadVersion == MAX_int32)
	{
		return false;
	}
	++ReadVersion;
	return true;
}

// 동일 화자 연속 입력에서 허용된 기존 표현 필드만 복사한다.
void UDialogueNodeInfo::CopySpeakerPresentationFrom(const UDialogueNodeInfo& Source)
{
	SpeakerID = Source.SpeakerID;
	PortraitData.EmoteType = Source.PortraitData.EmoteType;
	PortraitData.SidePosition = Source.PortraitData.SidePosition;
}
