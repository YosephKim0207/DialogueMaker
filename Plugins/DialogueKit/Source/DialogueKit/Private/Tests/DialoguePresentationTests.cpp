#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "DialogueTextPresentation.h"
#include "DialogueLineNodeInfo.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDialogueTextTimingTest, "DialogueKit.Presentation.TextAndInput", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// 실제 프레임 대신 명시 시간을 사용해 표시와 진행 입력의 분리를 검증한다.
bool FDialogueTextTimingTest::RunTest(const FString& Parameters)
{
	auto* Line = NewObject<UDialogueLineNodeInfo>();
	FTextProperty* Text = FindFProperty<FTextProperty>(UDialogueNodeInfo::StaticClass(), TEXT("DialogueText"));
	Text->SetPropertyValue_InContainer(Line, FText::FromString(TEXT("가나다라")));
	FDialoguePresentation View;
	View.State = EDialogueRunState::WaitingLine;
	View.Line = Line;
	View.LineNodeGuid = FGuid::NewGuid();
	FDialogueChoiceView Choice;
	Choice.ChoiceId = FGuid::NewGuid();
	Choice.Text = FText::FromString(TEXT("선택"));
	View.Choices.Add(Choice);
	auto* Display = NewObject<UDialogueTextPresentation>();
	Display->SetPresentation(View);
	TestTrue(TEXT("Zero immediately complete"), Display->IsTextComplete());
	TestEqual(TEXT("Choice immediately visible"), Display->GetVisibleChoices().Num(), 1);
	Line->SetTextTiming(true, 1.0f);
	Display->SetPresentation(View);
	const int64 Revision = Display->GetRevision();
	TestEqual(TEXT("Initial empty"), Display->GetVisibleText().ToString(), FString());
	TestTrue(TEXT("Choice hidden during text"), Display->GetVisibleChoices().IsEmpty());
	Display->AdvanceTime(.5f);
	TestEqual(TEXT("Whole line duration half"), Display->GetVisibleText().ToString(), FString(TEXT("가나")));
	View.bChoicesOnlyUpdate = true;
	Display->SetPresentation(View);
	TestEqual(TEXT("Choice refresh keeps partial text"), Display->GetVisibleText().ToString(), FString(TEXT("가나")));
	TestEqual(TEXT("Choice refresh keeps revision"), Display->GetRevision(), Revision);
	Display->SetVoicePlaying(true, Display->GetRevision());
	TestEqual(TEXT("Hidden choice only reveals"), Display->RequestChoice(Choice.ChoiceId, Revision, 10), EDialoguePresentationInput::Reveal);
	TestFalse(TEXT("Voice stop requested"), Display->IsVoicePlaying());
	TestTrue(TEXT("Reveal complete"), Display->IsTextComplete());
	TestEqual(TEXT("Same frame cannot select"), Display->RequestChoice(Choice.ChoiceId, Revision, 10), EDialoguePresentationInput::None);
	TestEqual(TEXT("Next frame selects"), Display->RequestChoice(Choice.ChoiceId, Revision, 11), EDialoguePresentationInput::Select);
	View.bChoicesOnlyUpdate = false;
	View.Choices.Reset();
	Line->SetTextTiming(false, .25f);
	Display->SetPresentation(View);
	TestEqual(TEXT("Old revision rejected"), Display->RequestAdvance(Revision, 12), EDialoguePresentationInput::None);
	Display->AdvanceTime(1.0f);
	TestTrue(TEXT("Per character total duration"), Display->IsTextComplete());
	Display->SetVoicePlaying(true, Display->GetRevision());
	TestEqual(TEXT("Voice alone consumes advance"), Display->RequestAdvance(Display->GetRevision(), 12), EDialoguePresentationInput::Reveal);
	TestEqual(TEXT("Subsequent input advances"), Display->RequestAdvance(Display->GetRevision(), 13), EDialoguePresentationInput::Advance);
	Display->SetPresentation(View);
	TestEqual(TEXT("New view keeps frame guard"), Display->RequestAdvance(Display->GetRevision(), 13), EDialoguePresentationInput::None);
	Display->SetVoicePlaying(true, Revision);
	TestFalse(TEXT("Stale voice ignored"), Display->IsVoicePlaying());
	TestEqual(TEXT("New view starts again"), Display->GetVisibleText().ToString(), FString());
	Text->SetPropertyValue_InContainer(Line, FText::FromString(TEXT("e\u0301x")));
	Display->SetPresentation(View);
	Display->AdvanceTime(.25f);
	TestEqual(TEXT("Combining character stays intact"), Display->GetVisibleText().ToString(), FString(TEXT("e\u0301")));
	return true;
}
#endif
