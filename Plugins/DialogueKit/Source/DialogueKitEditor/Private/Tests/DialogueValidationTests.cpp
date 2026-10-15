#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "DialogueGraphValidation.h"
#include "DialogueGraph.h"
#include "DialogueGraphCodec.h"
#include "DialogueEdGraphSchema.h"
#include "DialogueChoiceEdGraphNode.h"
#include "DialogueChoiceNodeInfo.h"
#include "UObject/Package.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDialogueValidationTest, "DialogueKit.Editor.Validation", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// Choice의 네 구성 규칙과 저장 전 미저장 그래프 검사를 확인한다.
bool FDialogueValidationTest::RunTest(const FString& Parameters)
{
	auto* Package = CreatePackage(*(TEXT("/Temp/DialogueValidation") + FGuid::NewGuid().ToString(EGuidFormats::Digits)));
	auto* Asset = NewObject<UDialogueGraph>(Package, TEXT("Block"));
	Asset->InitializeBlockIdentity();
	auto* Graph = NewObject<UEdGraph>(Asset);
	Graph->Schema = UDialogueEdGraphSchema::StaticClass();
	Graph->GetSchema()->CreateDefaultNodesForGraph(*Graph);
	FNewNodeAction Add(UDialogueChoiceEdGraphNode::StaticClass(), FText(), FText(), FText(), 0);
	auto* Node = CastChecked<UDialogueChoiceEdGraphNode>(Add.PerformAction(Graph, Graph->Nodes[0]->Pins[0], FVector2D::ZeroVector));
	auto* Info = CastChecked<UDialogueChoiceNodeInfo>(Node->GetNodeInfo());
	auto HasCode = [](const TArray<FDialogueGraphIssue>& Issues, FName Code) { return Issues.ContainsByPredicate([Code](const FDialogueGraphIssue& Issue) { return Issue.Code == Code; }); };
	auto Issues = FDialogueGraphValidation::Inspect(Graph);
	TestFalse(TEXT("Unconditional normal needs no fallback warning"), HasCode(Issues, TEXT("NoFallback")));
	Info->Options[0].bDefault = true;
	Issues = FDialogueGraphValidation::Inspect(Graph);
	TestTrue(TEXT("Default-only warning"), HasCode(Issues, TEXT("DefaultOnly")));
	TestTrue(TEXT("Default-only save allowed"), FDialogueGraphValidation::CanSave(Issues));
	Info->Options[0].bDefault = false;
	Info->Options[0].Conditions.Conditions.AddDefaulted();
	Issues = FDialogueGraphValidation::Inspect(Graph);
	TestTrue(TEXT("Conditional-only warns"), HasCode(Issues, TEXT("NoFallback")));
	TestTrue(TEXT("Invalid condition still permits save"), FDialogueGraphValidation::CanSave(Issues));
	FString Error;
	TestTrue(TEXT("Initial runtime snapshot"), FDialogueGraphCodec::ExportGraph(Graph, Asset, Error));
	FDialogueGraphValidation::RegisterWorkingGraph(Asset, Graph);
	Info->Options.Reset();
	Node->OnPropertiesChanged();
	Issues = FDialogueGraphValidation::Inspect(Graph);
	TestFalse(TEXT("Empty Choice blocks save"), FDialogueGraphValidation::CanSave(Issues));
	TestFalse(TEXT("Package guard sees unsaved working graph"), FDialogueGraphValidation::CanSavePackage(Package));
	FDialogueGraphValidation::UnregisterWorkingGraph(Asset);
	TestTrue(TEXT("Closed package checks preserved valid runtime"), FDialogueGraphValidation::CanSavePackage(Package));
	return true;
}
#endif
