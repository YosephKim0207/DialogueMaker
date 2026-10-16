#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "DialogueFlowEdGraphNode.h"
#include "DialogueChoiceEdGraphNode.h"
#include "DialogueStateChangeEdGraphNode.h"
#include "DialogueRerouteNode.h"
#include "DialogueEdGraphSchema.h"
#include "DialogueLineNodeInfo.h"
#include "SGraphNode.h"
#include "Widgets/Text/STextBlock.h"
#include "UObject/UnrealType.h"

namespace
{
// 실제 Slate 트리의 텍스트 속성을 읽어 캐시된 본문이 남는 회귀를 확인한다.
bool HasNodeText(const TSharedRef<SWidget>& Widget, const FString& Text)
{
	if (Widget->GetType() == FName(TEXT("STextBlock")) && StaticCastSharedRef<STextBlock>(Widget)->GetText().ToString() == Text)
	{
		return true;
	}
	FChildren* Children = Widget->GetChildren();
	for (int32 Index = 0; Index < Children->Num(); ++Index)
	{
		if (HasNodeText(Children->GetChildAt(Index), Text)) return true;
	}
	return false;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDialogueNodeContentTest, "DialogueKit.Editor.LineNodeContent", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// Details와 같은 자료를 변경한 뒤 동일 위젯의 화자/본문 갱신과 파생 노드 분리를 확인한다.
bool FDialogueNodeContentTest::RunTest(const FString& Parameters)
{
	auto* Graph = NewObject<UEdGraph>();
	Graph->Schema = UDialogueEdGraphSchema::StaticClass();
	FNewNodeAction Add(UDialogueFlowEdGraphNode::StaticClass(), FText(), FText(), FText(), 0);
	auto* Node = CastChecked<UDialogueFlowEdGraphNode>(Add.PerformAction(Graph, nullptr, FVector2D::ZeroVector));
	TestEqual(TEXT("Empty title fallback"), Node->GetNodeTitle(ENodeTitleType::FullTitle).ToString(), FString(TEXT("Line")));
	auto SetText = [&](FName Name, const TCHAR* Text)
	{
		FindFProperty<FTextProperty>(UDialogueNodeInfo::StaticClass(), Name)->SetPropertyValue_InContainer(Node->GetNodeInfo(), FText::FromString(Text));
	};
	SetText(TEXT("Title"), TEXT("상인과 첫 만남"));
	SetText(TEXT("DialogueText"), TEXT("어서 오세요.\n어떤 물건을 찾으세요?"));
	TSharedPtr<SGraphNode> View = Node->CreateVisualWidget();
	if (TestTrue(TEXT("Line widget created"), View.IsValid()) == false) return false;
	TestEqual(TEXT("Authored title"), Node->GetNodeTitle(ENodeTitleType::FullTitle).ToString(), FString(TEXT("상인과 첫 만남")));
	TestTrue(TEXT("Actual body widget reads text"), HasNodeText(View.ToSharedRef(), TEXT("어서 오세요.\n어떤 물건을 찾으세요?")));
	TestTrue(TEXT("Speaker shown"), HasNodeText(View.ToSharedRef(), TEXT("Speaker ID: TestNPC")));
	SetText(TEXT("DialogueText"), TEXT("수정된 대사"));
	FindFProperty<FEnumProperty>(UDialogueNodeInfo::StaticClass(), TEXT("SpeakerID"))->GetUnderlyingProperty()->SetIntPropertyValue(
		FindFProperty<FEnumProperty>(UDialogueNodeInfo::StaticClass(), TEXT("SpeakerID"))->ContainerPtrToValuePtr<void>(Node->GetNodeInfo()), static_cast<int64>(ESpeakerID::TestNPC2));
	TestTrue(TEXT("Existing widget refreshes body"), HasNodeText(View.ToSharedRef(), TEXT("수정된 대사")));
	TestTrue(TEXT("Existing widget refreshes speaker"), HasNodeText(View.ToSharedRef(), TEXT("Speaker ID: TestNPC2")));
	TestFalse(TEXT("Old body not retained"), HasNodeText(View.ToSharedRef(), TEXT("어서 오세요.\n어떤 물건을 찾으세요?")));
	TestFalse(TEXT("Choice retains default widget"), NewObject<UDialogueChoiceEdGraphNode>(Graph)->CreateVisualWidget().IsValid());
	TestFalse(TEXT("StateChange retains default widget"), NewObject<UDialogueStateChangeEdGraphNode>(Graph)->CreateVisualWidget().IsValid());
	auto* Knot = NewObject<UDialogueRerouteNode>(Graph);
	Knot->AllocateDefaultPins();
	TestTrue(TEXT("Knot retains its widget"), Knot->CreateVisualWidget().IsValid());
	TestEqual(TEXT("Knot schema tint"), Graph->GetSchema()->GetPinTypeColor(Knot->Pins[1]->PinType), Node->GetNodeTitleColor());
	return true;
}
#endif
