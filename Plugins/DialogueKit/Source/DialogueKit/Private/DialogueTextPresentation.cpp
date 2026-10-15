#include "DialogueTextPresentation.h"
#include "DialogueLineNodeInfo.h"
#include "Internationalization/BreakIterator.h"

// 로컬라이즈된 본문을 문자 경계로 나누어 결합 문자/서로게이트 중간에서 자르지 않는다.
void UDialogueTextPresentation::SetPresentation(const FDialoguePresentation& Presentation)
{
	if (Presentation.bChoicesOnlyUpdate && Current.LineNodeGuid == Presentation.LineNodeGuid && Current.State == Presentation.State)
	{
		Current.Choices = Presentation.Choices;
		Current.ChoiceNodeGuid = Presentation.ChoiceNodeGuid;
		return;
	}
	Current = Presentation;
	FullText = Current.Line ? Current.Line->GetDialogueText().ToString() : FString();
	CharacterEnds.Reset();
	const auto Iterator = FBreakIterator::CreateCharacterBoundaryIterator();
	Iterator->SetString(FullText);
	Iterator->ResetToBeginning();
	for (int32 End = Iterator->MoveToNext(); End != INDEX_NONE; End = Iterator->MoveToNext())
	{
		if (End > 0 && End <= FullText.Len())
		{
			CharacterEnds.Add(End);
		}
	}
	Duration = 0.0;
	if (const auto* Line = Cast<UDialogueLineNodeInfo>(Current.Line))
	{
		if (FMath::IsFinite(Line->LineDuration) && Line->LineDuration > 0.0f)
		{
			Duration = Line->LineDuration;
		}
		else if (FMath::IsFinite(Line->SecondsPerCharacter) && Line->SecondsPerCharacter > 0.0f)
		{
			Duration = double(Line->SecondsPerCharacter) * CharacterEnds.Num();
		}
	}
	Elapsed = 0.0;
	VisibleCharacters = Duration > 0.0 ? 0 : CharacterEnds.Num();
	bVoicePlaying = false;
	// 실제 실행 수명에서 도달 불가능한 범위도 signed overflow 없이 방어한다.
	Revision = Revision == MAX_int64 ? 1 : Revision + 1;
}

// 완료 이후에는 추가 시간을 누적하지 않는다.
void UDialogueTextPresentation::AdvanceTime(float DeltaSeconds)
{
	if (IsTextComplete() || FMath::IsFinite(DeltaSeconds) == false || DeltaSeconds <= 0.0f)
	{
		return;
	}
	Elapsed = FMath::Min(Duration, Elapsed + DeltaSeconds);
	VisibleCharacters = Elapsed >= Duration ? CharacterEnds.Num() : FMath::FloorToInt(Elapsed / Duration * CharacterEnds.Num());
}

// 실제 음향 컴포넌트의 재생/종료 상태를 화면에서 전달한다.
void UDialogueTextPresentation::SetVoicePlaying(bool bPlaying, int64 ExpectedRevision)
{
	if (ExpectedRevision == Revision)
	{
		bVoicePlaying = bPlaying;
	}
}

// 재진입/이전 화면/같은 입력의 중복 전달을 한 경계에서 거부한다.
bool UDialogueTextPresentation::AcceptInput(int64 ExpectedRevision, int64 InputToken)
{
	if (ExpectedRevision != Revision || InputToken <= LastInputToken || InputToken < 0 ||
		(Current.State != EDialogueRunState::WaitingLine && Current.State != EDialogueRunState::WaitingChoice))
	{
		return false;
	}
	LastInputToken = InputToken;
	return true;
}

// 전체 표시 동작은 진행과 분리한다. 호출 화면은 Reveal 결과에서 실제 음성을 정지한다.
EDialoguePresentationInput UDialogueTextPresentation::RevealIfNeeded()
{
	if (IsTextComplete() == false || bVoicePlaying)
	{
		VisibleCharacters = CharacterEnds.Num();
		Elapsed = Duration;
		bVoicePlaying = false;
		return EDialoguePresentationInput::Reveal;
	}
	return EDialoguePresentationInput::None;
}

// 일반 진행 입력으로 선택지의 첫 항목을 자동 선택하지 않는다.
EDialoguePresentationInput UDialogueTextPresentation::RequestAdvance(int64 ExpectedRevision, int64 InputToken)
{
	if (AcceptInput(ExpectedRevision, InputToken) == false)
	{
		return EDialoguePresentationInput::None;
	}
	if (RevealIfNeeded() == EDialoguePresentationInput::Reveal)
	{
		return EDialoguePresentationInput::Reveal;
	}
	return Current.State == EDialogueRunState::WaitingLine && Current.Choices.IsEmpty() ? EDialoguePresentationInput::Advance : EDialoguePresentationInput::None;
}

// 출력/음성 중의 선택 입력 역시 펼치기만 하고 다음 입력에서 실제 선택한다.
EDialoguePresentationInput UDialogueTextPresentation::RequestChoice(FGuid ChoiceId, int64 ExpectedRevision, int64 InputToken)
{
	if (AcceptInput(ExpectedRevision, InputToken) == false)
	{
		return EDialoguePresentationInput::None;
	}
	if (RevealIfNeeded() == EDialoguePresentationInput::Reveal)
	{
		return EDialoguePresentationInput::Reveal;
	}
	return Current.Choices.ContainsByPredicate([ChoiceId](const FDialogueChoiceView& Choice) { return Choice.ChoiceId == ChoiceId; }) ? EDialoguePresentationInput::Select : EDialoguePresentationInput::None;
}

// 완료된 경우 원본 FText를 유지해 로컬라이즈 자료를 잃지 않는다.
FText UDialogueTextPresentation::GetVisibleText() const
{
	if (Current.Line == nullptr)
	{
		return FText();
	}
	return IsTextComplete() ? Current.Line->GetDialogueText() : FText::FromString(FullText.Left(VisibleCharacters > 0 ? CharacterEnds[VisibleCharacters - 1] : 0));
}

// 직접 연결 Choice는 텍스트 완료와 동시에 노출한다. Branch 너머는 탐색하지 않는다.
TArray<FDialogueChoiceView> UDialogueTextPresentation::GetVisibleChoices() const
{
	return IsTextComplete() ? Current.Choices : TArray<FDialogueChoiceView>();
}
