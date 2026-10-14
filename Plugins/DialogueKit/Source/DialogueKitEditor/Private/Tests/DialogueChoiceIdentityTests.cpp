#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "DialogueEdGraphNode.h"
#include "DialogueNodeInfo.h"
#include "EdGraph/EdGraph.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDialogueChoiceIdentityTest,
	"DialogueKit.Contracts.NodeIdentity.ChoiceReorder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// Details의 선택 배열 재정렬 이후에도 기존 핀 ID와 연결이 같은 선택을 가리킨다.
bool FDialogueChoiceIdentityTest::RunTest(const FString& Parameters)
{
	UEdGraph* Graph = NewObject<UEdGraph>();
	UDialogueEdGraphNode* Node = NewObject<UDialogueEdGraphNode>(Graph);
	Node->InitNodeInfo(Node);
	Node->CreateDefaultInputPin();
	UDialogueNodeInfo* Info = Node->GetDialogueNodeInfo();
	Info->AddDialogueChoice(FDialogueChoice(FText::FromString(TEXT("A"))));
	Info->AddDialogueChoice(FDialogueChoice(FText::FromString(TEXT("B"))));
	Node->SyncPinWithResponses();
	UEdGraphPin* A = Node->Pins[1];
	UEdGraphPin* B = Node->Pins[2];
	const FGuid AId = A->PinId;
	UDialogueEdGraphNode* Target = NewObject<UDialogueEdGraphNode>(Graph);
	UEdGraphPin* Input = Target->CreateDefaultInputPin();
	A->MakeLinkTo(Input);
	FArrayProperty* Property = FindFProperty<FArrayProperty>(UDialogueNodeInfo::StaticClass(), TEXT("DialogueChoices"));
	FScriptArrayHelper Choices(Property, Property->ContainerPtrToValuePtr<void>(Info));
	Choices.SwapValues(0, 1);
	Node->SyncPinWithResponses();
	TestTrue(TEXT("B moved with its pin"), Node->Pins[1] == B);
	TestTrue(TEXT("A retains pin and link"), Node->Pins[2] == A && A->PinId == AId && A->LinkedTo.Contains(Input));
	TestTrue(TEXT("Identity matches choice"), A->PersistentGuid == Info->GetDialogueChoices()[1].ChoiceId);
	UDialogueNodeInfo* Copy = DuplicateObject<UDialogueNodeInfo>(Info, Graph);
	TestEqual(TEXT("Duplicate preserves ChoiceId"), Copy->GetDialogueChoices()[0].ChoiceId, Info->GetDialogueChoices()[0].ChoiceId);
	TestTrue(TEXT("Read version advances explicitly"), Info->AdvanceReadVersion());
	TestEqual(TEXT("Other copy retains read version"), Copy->GetReadVersion(), 1);
	TestEqual(TEXT("Current read version"), Info->GetReadVersion(), 2);
	Info->AddDialogueChoice(Info->GetDialogueChoices()[0]);
	TestTrue(TEXT("Added duplicate is a new choice"), Info->GetDialogueChoices()[2].ChoiceId != Info->GetDialogueChoices()[0].ChoiceId);
	return true;
}
#endif
