#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "DialogueGraphSearch.h"
#include "DialogueEdGraphSchema.h"
#include "DialogueFlowEdGraphNode.h"
#include "DialogueChoiceEdGraphNode.h"
#include "DialogueChoiceNodeInfo.h"
#include "DialogueLineNodeInfo.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDialogueSearchTest, "DialogueKit.Editor.Search", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// 화자/대사 AND, 선택지 단독, 혼합 거부와 결과 없음 갱신을 검증한다.
bool FDialogueSearchTest::RunTest(const FString& Parameters)
{
	auto* Graph = NewObject<UEdGraph>();
	Graph->Schema = UDialogueEdGraphSchema::StaticClass();
	FNewNodeAction Add(UDialogueFlowEdGraphNode::StaticClass(), FText(), FText(), FText(), 0);
	auto* Line = CastChecked<UDialogueFlowEdGraphNode>(Add.PerformAction(Graph, nullptr, FVector2D::ZeroVector));
	auto* Info = CastChecked<UDialogueLineNodeInfo>(Line->GetNodeInfo());
	FindFProperty<FTextProperty>(UDialogueNodeInfo::StaticClass(), TEXT("DialogueText"))->SetPropertyValue_InContainer(Info, FText::FromString(TEXT("마을의 사건")));
	FNewNodeAction AddChoice(UDialogueChoiceEdGraphNode::StaticClass(), FText(), FText(), FText(), 0);
	auto* Choice = CastChecked<UDialogueChoiceEdGraphNode>(AddChoice.PerformAction(Graph, Line->Pins[1], FVector2D::ZeroVector));
	auto* ChoiceInfo = CastChecked<UDialogueChoiceNodeInfo>(Choice->GetNodeInfo());
	ChoiceInfo->Options[0].Text = FText::FromString(TEXT("사건 질문"));
	Choice->OnPropertiesChanged();
	const FString Name = StaticEnum<ESpeakerID>()->GetDisplayNameTextByValue(int64(Info->GetSpeakerEmotePair().Speaker)).ToString();
	TArray<FDialogueSearchResult> Results;
	FText Error;
	TestTrue(TEXT("Speaker and text accepted"), FDialogueGraphSearch::Search(Graph, Name, TEXT("사건"), TEXT(""), Results, Error));
	TestEqual(TEXT("AND finds Line"), Results.Num(), 1);
	TestTrue(TEXT("Choice text accepted"), FDialogueGraphSearch::Search(Graph, TEXT(""), TEXT(""), TEXT("질문"), Results, Error));
	TestEqual(TEXT("Choice ID identifies matching option"), Results[0].ChoiceId, ChoiceInfo->Options[0].ChoiceId);
	TestFalse(TEXT("Mixed search refused"), FDialogueGraphSearch::Search(Graph, Name, TEXT(""), TEXT("질문"), Results, Error));
	TestEqual(TEXT("Mixed search preserves results"), Results.Num(), 1);
	TestTrue(TEXT("No match is valid search"), FDialogueGraphSearch::Search(Graph, Name, TEXT("없는 내용"), TEXT(""), Results, Error));
	TestTrue(TEXT("No match clears old results with message"), Results.IsEmpty() && Error.IsEmpty() == false);
	TestFalse(TEXT("Empty search refused"), FDialogueGraphSearch::Search(Graph, TEXT(" "), TEXT(""), TEXT(""), Results, Error));
	return true;
}
#endif
