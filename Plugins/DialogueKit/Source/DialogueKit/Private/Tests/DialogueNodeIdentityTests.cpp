#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "DialogueRuntimeGraph.h"
#include "DialogueNodeInfo.h"
#include "Serialization/ObjectReader.h"
#include "Serialization/ObjectWriter.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDialogueNodeIdentityTest,
	"DialogueKit.Contracts.NodeIdentity.Validation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// 올바른 ID/핀 대응과 누락/중복 검사 및 NodeInfo 직렬화를 확인한다.
bool FDialogueNodeIdentityTest::RunTest(const FString& Parameters)
{
	UDialogueRuntimeGraph* Graph = NewObject<UDialogueRuntimeGraph>();
	UDialogueRuntimeNode* Node = NewObject<UDialogueRuntimeNode>(Graph);
	Node->NodeGuid = FGuid::NewGuid();
	UDialogueNodeInfo* Info = NewObject<UDialogueNodeInfo>(Node);
	Info->AddDialogueChoice(FDialogueChoice(FText::FromString(TEXT("A"))));
	Node->NodeInfo = Info;
	Node->InputPin = NewObject<UDialogueRuntimePin>(Node);
	Node->InputPin->PinId = FGuid::NewGuid();
	Node->InputPin->OwnerNodeGuid = Node->NodeGuid;
	UDialogueRuntimePin* Output = NewObject<UDialogueRuntimePin>(Node);
	Output->PinId = FGuid::NewGuid();
	Output->OwnerNodeGuid = Node->NodeGuid;
	Output->ChoiceId = Info->GetDialogueChoices()[0].ChoiceId;
	Node->OutputPins.Add(Output);
	Graph->Nodes.Add(Node);
	TArray<FString> Issues;
	TestTrue(TEXT("Valid identity graph"), Graph->ValidateIdentifiers(Issues));
	Output->ChoiceId.Invalidate();
	TestFalse(TEXT("Mismatch rejected"), Graph->ValidateIdentifiers(Issues));
	TestFalse(TEXT("Validation does not repair"), Output->ChoiceId.IsValid());
	Output->ChoiceId = Info->GetDialogueChoices()[0].ChoiceId;
	Graph->Nodes.Add(Node);
	TestFalse(TEXT("Duplicate rejected"), Graph->ValidateIdentifiers(Issues));
	Graph->Nodes.Pop();
	TArray<uint8> Bytes;
	FObjectWriter Writer(Info, Bytes);
	UDialogueNodeInfo* Restored = NewObject<UDialogueNodeInfo>();
	FObjectReader Reader(Restored, Bytes);
	TestEqual(TEXT("Choice ID archive"), Restored->GetDialogueChoices()[0].ChoiceId, Output->ChoiceId);
	TestEqual(TEXT("Read version archive"), Restored->GetReadVersion(), 1);
	FIntProperty* ReadProperty = FindFProperty<FIntProperty>(UDialogueNodeInfo::StaticClass(), TEXT("ReadVersion"));
	ReadProperty->SetPropertyValue_InContainer(Info, 0);
	TestFalse(TEXT("Invalid read version diagnosed"), Graph->ValidateIdentifiers(Issues));
	TestFalse(TEXT("Invalid read version not advanced"), Info->AdvanceReadVersion());
	ReadProperty->SetPropertyValue_InContainer(Info, MAX_int32);
	TestFalse(TEXT("Read version overflow rejected"), Info->AdvanceReadVersion());
	TestEqual(TEXT("Overflow preserves value"), Info->GetReadVersion(), MAX_int32);
	FDialogueChoice Legacy;
	TestFalse(TEXT("Legacy missing choice ID stays invalid"), Legacy.ChoiceId.IsValid());
	return true;
}
#endif
