#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "DialogueGraph.h"
#include "DialogueGraphCodec.h"
#include "DialogueEdGraphSchema.h"
#include "DialogueEdStartGraphNode.h"
#include "DialogueEdEndGraphNode.h"
#include "DialogueEdGraphNode.h"
#include "DialogueFlowEdGraphNode.h"
#include "DialogueLineNodeInfo.h"
#include "DialogueBlockRunner.h"
#include "DialogueChoiceEdGraphNode.h"
#include "DialogueBranchEdGraphNode.h"
#include "DialogueBranchNodeInfoBase.h"
#include "DialogueChoiceNodeInfo.h"
#include "DialogueSampleStateProvider.h"
#include "DialogueSampleEffectProvider.h"
#include "DialogueStateChangeNodeInfo.h"
#include "DialogueStateChangeEdGraphNode.h"
#include "DialogueEndNodeInfo.h"
#include "DialogueSubsystem.h"
#include "DialoguePresentationProbe.h"
#include "DialogueLocalizationCSVConverter.h"
#include "Engine/GameInstance.h"
#include "EdGraph/EdGraph.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "PackageTools.h"
#include "UObject/SavePackage.h"
#include "UObject/Package.h"
#include "UObject/UnrealType.h"

// 실제 생성 액션과 동일한 경로로 저작용 노드를 만든다.
static UDialogueEdGraphNodeBase* AddFlowTestNode(UEdGraph* Graph, UClass* NodeClass, UEdGraphPin* From = nullptr)
{
	FNewNodeAction Action(NodeClass, FText(), FText(), FText(), 0);
	return CastChecked<UDialogueEdGraphNodeBase>(Action.PerformAction(Graph, From, FVector2D::ZeroVector));
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDialogueGraphLineRoundtripTest,
	"DialogueKit.Graph.LinePackageRoundtrip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// 독립 임시 패키지를 저장/언로드/재로드하여 소유·본문·ID·핀 및 실제 실행을 확인한다.
bool FDialogueGraphLineRoundtripTest::RunTest(const FString& Parameters)
{
	const FString Id = FGuid::NewGuid().ToString(EGuidFormats::Digits);
	const FString Mount = TEXT("/DialogueKitTest") + Id + TEXT("/");
	const FString Folder = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("AI/GraphTests") / Id) + TEXT("/");
	IFileManager::Get().MakeDirectory(*Folder, true);
	FPackageName::RegisterMountPoint(Mount, Folder);
	const FString PackageName = Mount + TEXT("LineGraph");
	const FString Filename = Folder + TEXT("LineGraph.uasset");
	UPackage* Package = CreatePackage(*PackageName);
	UDialogueGraph* Asset = NewObject<UDialogueGraph>(Package, TEXT("LineGraph"), RF_Public | RF_Standalone);
	Asset->InitializeBlockIdentity();
	const FGuid BlockId = Asset->GetBlockId();
	UEdGraph* Graph = NewObject<UEdGraph>(Asset);
	Graph->Schema = UDialogueEdGraphSchema::StaticClass();
	Graph->GetSchema()->CreateDefaultNodesForGraph(*Graph);
	UDialogueEdGraphNodeBase* Line = AddFlowTestNode(Graph, UDialogueFlowEdGraphNode::StaticClass(), Graph->Nodes[0]->Pins[0]);
	UDialogueEdGraphNodeBase* End = AddFlowTestNode(Graph, UDialogueEdEndGraphNode::StaticClass(), Line->Pins[1]);
	const FGuid LineId = Line->NodeGuid;
	const FGuid PinId = Line->Pins[1]->PinId;
	FTextProperty* TextProperty = FindFProperty<FTextProperty>(UDialogueNodeInfo::StaticClass(), TEXT("DialogueText"));
	if (TestNotNull(TEXT("Line text property"), TextProperty) == false)
	{
		FPackageName::UnRegisterMountPoint(Mount, Folder);
		return false;
	}
	TextProperty->SetPropertyValue_InContainer(Line->GetNodeInfo(), FText::FromString(TEXT("S2 line")));
	FString Error;
	if (TestTrue(TEXT("Export graph"), FDialogueGraphCodec::ExportGraph(Graph, Asset, Error)) == false) return false;
	TestTrue(TEXT("Runtime node owned by runtime graph"), Asset->Graph->Nodes[1]->GetOuter() == Asset->Graph);
	FSavePackageArgs Args;
	Args.TopLevelFlags = RF_Public | RF_Standalone;
	Args.SaveFlags = SAVE_NoError;
	const bool bSaved = UPackage::SavePackage(Package, Asset, *Filename, Args);
	TestTrue(TEXT("Package saved"), bSaved);
	TArray<UPackage*> Packages = {Package};
	const bool bUnloaded = UPackageTools::UnloadPackages(Packages);
	TestTrue(TEXT("Original package unloaded"), bUnloaded);
	if (bSaved && bUnloaded)
	{
		UPackage* Reloaded = LoadPackage(nullptr, *PackageName, LOAD_None);
		UDialogueGraph* Loaded = Reloaded ? FindObject<UDialogueGraph>(Reloaded, TEXT("LineGraph")) : nullptr;
		if (TestNotNull(TEXT("Asset reloaded from disk"), Loaded))
		{
			TestTrue(TEXT("Block ID preserved"), Loaded->GetBlockId() == BlockId);
			UEdGraph* Reopened = NewObject<UEdGraph>(Loaded);
			Reopened->Schema = UDialogueEdGraphSchema::StaticClass();
			TestTrue(TEXT("Import reopened graph"), FDialogueGraphCodec::ImportGraph(Loaded, Reopened, Error));
			TestEqual(TEXT("All nodes restored"), Reopened->Nodes.Num(), 3);
			UDialogueBlockRunner* Runner = NewObject<UDialogueBlockRunner>();
			int32 Events = 0;
			Runner->OnPresentationChangedNative.AddLambda([&](const FDialoguePresentation& State) { ++Events; });
			TestTrue(TEXT("Start restored block"), Runner->StartBlock(Loaded, nullptr, nullptr, nullptr));
			FDialoguePresentation State = Runner->GetPresentation();
			TestTrue(TEXT("Line waiting"), State.State == EDialogueRunState::WaitingLine && State.NodeGuid == LineId);
			if (TestNotNull(TEXT("Line payload"), State.Line.Get()))
			{
				TestEqual(TEXT("Text roundtrip"), State.Line->GetDialogueText().ToString(), FString(TEXT("S2 line")));
				TestTrue(TEXT("Line has no embedded choice"), State.Line->GetDialogueChoices().IsEmpty());
			}
			TestTrue(TEXT("Direct next End without ending"), Runner->GetDirectNextType() == EDialogueType::EndNode && Runner->GetPresentation().State == EDialogueRunState::WaitingLine);
			TestTrue(TEXT("Output pin identity retained"), Loaded->Graph->Nodes[1]->OutputPins[0]->PinId == PinId);
			TestFalse(TEXT("Active run cannot be replaced"), Runner->StartBlock(Loaded, nullptr, nullptr, nullptr));
			TestTrue(TEXT("Presentation is isolated"), State.Line.Get() != Loaded->Graph->Nodes[1]->NodeInfo);
			TestTrue(TEXT("Advance reaches End"), Runner->AdvanceLine());
			TestTrue(TEXT("Completed"), Runner->GetPresentation().State == EDialogueRunState::Completed);
			TestEqual(TEXT("Presentation events delivered"), Events, 2);
			TestFalse(TEXT("Completed cannot advance"), Runner->AdvanceLine());
		}
	}
	// 저장 후 다음 틱의 에디터 검증도 경로를 조회하므로 이 고유 마운트는 테스트 프로세스 종료까지 유지한다.
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDialogueGraphInvalidLinkTest,
	"DialogueKit.Runtime.InvalidLinkStops",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// 필요한 연결 오류가 정상 완료로 잘못 처리되지 않는지 확인한다.
bool FDialogueGraphInvalidLinkTest::RunTest(const FString& Parameters)
{
	UDialogueGraph* Asset = NewObject<UDialogueGraph>();
	Asset->InitializeBlockIdentity();
	UEdGraph* Graph = NewObject<UEdGraph>(Asset);
	Graph->Schema = UDialogueEdGraphSchema::StaticClass();
	Graph->GetSchema()->CreateDefaultNodesForGraph(*Graph);
	UDialogueEdGraphNodeBase* Line = AddFlowTestNode(Graph, UDialogueFlowEdGraphNode::StaticClass(), Graph->Nodes[0]->Pins[0]);
	FString Error;
	TestTrue(TEXT("Unfinished edit may convert"), FDialogueGraphCodec::ExportGraph(Graph, Asset, Error));
	UDialogueBlockRunner* Runner = NewObject<UDialogueBlockRunner>();
	TestTrue(TEXT("Show line before missing link"), Runner->StartBlock(Asset, nullptr, nullptr, nullptr));
	TestFalse(TEXT("Missing link fails"), Runner->AdvanceLine());
	TestTrue(TEXT("Failure not completion"), Runner->GetPresentation().State == EDialogueRunState::Failed);
	TestEqual(TEXT("Connection error code"), Runner->GetPresentation().ErrorCode, FName(TEXT("InvalidConnection")));
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDialogueGraphConnectionTest,
	"DialogueKit.Graph.SingleOutputMultiInput",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// 출력 교체 시 기존 목적지의 다른 입력 연결과 핀 ID는 보존한다.
bool FDialogueGraphConnectionTest::RunTest(const FString& Parameters)
{
	UEdGraph* Graph = NewObject<UEdGraph>();
	Graph->Schema = UDialogueEdGraphSchema::StaticClass();
	UDialogueEdGraphNodeBase* A = AddFlowTestNode(Graph, UDialogueFlowEdGraphNode::StaticClass());
	UDialogueEdGraphNodeBase* B = AddFlowTestNode(Graph, UDialogueFlowEdGraphNode::StaticClass());
	UDialogueEdGraphNodeBase* C = AddFlowTestNode(Graph, UDialogueFlowEdGraphNode::StaticClass());
	UDialogueEdGraphNodeBase* D = AddFlowTestNode(Graph, UDialogueFlowEdGraphNode::StaticClass());
	const UEdGraphSchema* Schema = Graph->GetSchema();
	TestTrue(TEXT("A to B"), Schema->TryCreateConnection(A->Pins[1], B->Pins[0]));
	TestTrue(TEXT("D to B"), Schema->TryCreateConnection(D->Pins[1], B->Pins[0]));
	TestEqual(TEXT("Multi input"), B->Pins[0]->LinkedTo.Num(), 2);
	TestFalse(TEXT("Invalid output to output"), Schema->TryCreateConnection(A->Pins[1], C->Pins[1]));
	TestTrue(TEXT("Invalid attempt preserves old connection"), A->Pins[1]->LinkedTo.Contains(B->Pins[0]));
	TestTrue(TEXT("Replace A to C"), Schema->TryCreateConnection(C->Pins[0], A->Pins[1]));
	TestEqual(TEXT("Single output"), A->Pins[1]->LinkedTo.Num(), 1);
	TestTrue(TEXT("Other incoming preserved"), B->Pins[0]->LinkedTo.Num() == 1 && B->Pins[0]->LinkedTo[0] == D->Pins[1]);
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDialogueChoiceFlowTest,
	"DialogueKit.Runtime.ChoiceFilteringAndLoop",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// 직접 Choice 동시 표시, 기본 선택지, ID 기반 선택과 정상 질문 반복을 확인한다.
bool FDialogueChoiceFlowTest::RunTest(const FString& Parameters)
{
	UDialogueGraph* Asset = NewObject<UDialogueGraph>();
	Asset->InitializeBlockIdentity();
	UEdGraph* Graph = NewObject<UEdGraph>(Asset);
	Graph->Schema = UDialogueEdGraphSchema::StaticClass();
	Graph->GetSchema()->CreateDefaultNodesForGraph(*Graph);
	UDialogueEdGraphNodeBase* Line = AddFlowTestNode(Graph, UDialogueFlowEdGraphNode::StaticClass(), Graph->Nodes[0]->Pins[0]);
	UDialogueEdGraphNodeBase* Choice = AddFlowTestNode(Graph, UDialogueChoiceEdGraphNode::StaticClass(), Line->Pins[1]);
	UDialogueChoiceNodeInfo* Info = CastChecked<UDialogueChoiceNodeInfo>(Choice->GetNodeInfo());
	FDialogueOption Default;
	Default.bDefault = true;
	Default.Text = FText::FromString(TEXT("Fallback"));
	Info->Options.Add(Default);
	Choice->OnPropertiesChanged();
	const FGuid OrdinaryId = Info->Options[0].ChoiceId;
	const FGuid DefaultId = Info->Options[1].ChoiceId;
	Graph->GetSchema()->TryCreateConnection(Choice->Pins[1], Line->Pins[0]);
	AddFlowTestNode(Graph, UDialogueEdEndGraphNode::StaticClass(), Choice->Pins[2]);
	const FGuid OrdinaryPin = Choice->Pins[1]->PinId;
	Info->Options.Swap(0, 1);
	Choice->OnPropertiesChanged();
	TestTrue(TEXT("Reorder retains identity and connection"), Choice->Pins[2]->PinId == OrdinaryPin && Choice->Pins[2]->LinkedTo.Contains(Line->Pins[0]));
	FString Error;
	if (TestTrue(TEXT("Export choices"), FDialogueGraphCodec::ExportGraph(Graph, Asset, Error)) == false) return false;
	UDialogueBlockRunner* Runner = NewObject<UDialogueBlockRunner>();
	TestTrue(TEXT("Start"), Runner->StartBlock(Asset, nullptr, nullptr, nullptr));
	FDialoguePresentation View = Runner->GetPresentation();
	TestTrue(TEXT("Line and direct Choice together"), View.State == EDialogueRunState::WaitingLine && View.ChoiceNodeGuid == Choice->NodeGuid && View.Choices.Num() == 1 && View.Choices[0].ChoiceId == OrdinaryId);
	TestFalse(TEXT("Hidden default cannot be selected"), Runner->SelectChoice(DefaultId));
	for (int32 Index = 0; Index < 3; ++Index)
	{
		TestTrue(TEXT("Normal player loop"), Runner->SelectChoice(OrdinaryId));
		TestTrue(TEXT("Returns to original Line"), Runner->GetPresentation().NodeGuid == Line->NodeGuid);
	}
	// 조회 실패 True 대체와 구분하기 위해 등록된 기본값 0으로 False 조건을 구성한다.
	UDialogueSampleStateProvider* Provider = NewObject<UDialogueSampleStateProvider>();
	FDialogueStateDefinition Definition;
	Definition.Key.Kind = EDialogueStateKind::Value;
	Definition.Key.ItemId = TEXT("Level");
	Provider->AddDefinition(Definition);
	FDialogueCondition Condition;
	Condition.Key = Definition.Key;
	Condition.Comparison = EDialogueComparison::Greater;
	Condition.ValueOperand = 10;
	Info->Options[1].Conditions.Conditions.Add(Condition);
	if (FDialogueGraphCodec::ExportGraph(Graph, Asset, Error) == false) return false;
	Runner = NewObject<UDialogueBlockRunner>();
	TestTrue(TEXT("Start fallback"), Runner->StartBlock(Asset, Provider, nullptr, nullptr));
	View = Runner->GetPresentation();
	TestTrue(TEXT("Only default shown"), View.Choices.Num() == 1 && View.Choices[0].ChoiceId == DefaultId);
		// 표시되지 않았던 ID는 조건이 바뀌어도 첫 요청에서 실행하지 않는다.
	UDialogueChoiceNodeInfo* RuntimeInfo = CastChecked<UDialogueChoiceNodeInfo>(Asset->Graph->Nodes[2]->NodeInfo);
	RuntimeInfo->Options[1].Conditions.Conditions.Reset();
	TestFalse(TEXT("Newly eligible hidden ID rejected"), Runner->SelectChoice(OrdinaryId));
	TestTrue(TEXT("Next input accepts now published ID"), Runner->SelectChoice(OrdinaryId));
	RuntimeInfo->Options[1].Conditions.Conditions.Add(Condition);
	TestFalse(TEXT("Stale ordinary rejected and fallback published"), Runner->SelectChoice(OrdinaryId));
	TestTrue(TEXT("Default reaches End"), Runner->SelectChoice(DefaultId));
	TestTrue(TEXT("Completed"), Runner->GetPresentation().State == EDialogueRunState::Completed);
	Info->Options.RemoveAt(0);
	Choice->OnPropertiesChanged();
	if (FDialogueGraphCodec::ExportGraph(Graph, Asset, Error) == false) return false;
	Runner = NewObject<UDialogueBlockRunner>();
	Runner->StartBlock(Asset, Provider, nullptr, nullptr);
	TestTrue(TEXT("Preview does not complete empty choice"), Runner->GetPresentation().State == EDialogueRunState::WaitingLine);
	TestTrue(TEXT("Entering empty filtered choice completes normally"), Runner->AdvanceLine());
	TestTrue(TEXT("Fallback completion"), Runner->GetPresentation().State == EDialogueRunState::Completed);
	Info->Options[0].Conditions.Conditions.Reset();
	if (FDialogueGraphCodec::ExportGraph(Graph, Asset, Error) == false) return false;
	Runner = NewObject<UDialogueBlockRunner>();
	Runner->StartBlock(Asset, Provider, nullptr, nullptr);
	Runner->AdvanceLine();
	TestTrue(TEXT("Waiting for explicit choice"), Runner->GetPresentation().State == EDialogueRunState::WaitingChoice);
	TestTrue(TEXT("Choice does not predict first option path"), Runner->GetDirectNextType() == EDialogueType::Unknown);
	CastChecked<UDialogueChoiceNodeInfo>(Asset->Graph->Nodes[2]->NodeInfo)->Options[0].Conditions.Conditions.Add(Condition);
	TestFalse(TEXT("Stale choice returns false"), Runner->SelectChoice(OrdinaryId));
	TestTrue(TEXT("No permanent wait after all choices disappear"), Runner->GetPresentation().State == EDialogueRunState::Completed);
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDialogueBranchFlowTest,
	"DialogueKit.Runtime.BranchFlowAndCycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// 사전 조회는 Branch를 평가하지 않고 실제 진입은 양쪽 분기와 자동 순환을 구분한다.
bool FDialogueBranchFlowTest::RunTest(const FString& Parameters)
{
	UDialogueGraph* Asset = NewObject<UDialogueGraph>();
	Asset->InitializeBlockIdentity();
	UEdGraph* Graph = NewObject<UEdGraph>(Asset);
	Graph->Schema = UDialogueEdGraphSchema::StaticClass();
	Graph->GetSchema()->CreateDefaultNodesForGraph(*Graph);
	UDialogueEdGraphNodeBase* Line = AddFlowTestNode(Graph, UDialogueFlowEdGraphNode::StaticClass(), Graph->Nodes[0]->Pins[0]);
	UDialogueEdGraphNodeBase* Branch = AddFlowTestNode(Graph, UDialogueBranchEdGraphNode::StaticClass(), Line->Pins[1]);
	UDialogueEdGraphNodeBase* TrueLine = AddFlowTestNode(Graph, UDialogueFlowEdGraphNode::StaticClass(), Branch->Pins[1]);
	UDialogueEdGraphNodeBase* FalseLine = AddFlowTestNode(Graph, UDialogueFlowEdGraphNode::StaticClass(), Branch->Pins[2]);
	UDialogueBranchNodeInfoBase* Info = CastChecked<UDialogueBranchNodeInfoBase>(Branch->GetNodeInfo());
	UDialogueSampleStateProvider* Provider = NewObject<UDialogueSampleStateProvider>();
	FDialogueStateDefinition Definition;
	Definition.Key.Kind = EDialogueStateKind::Value;
	Definition.Key.ItemId = TEXT("Level");
	Provider->AddDefinition(Definition);
	FDialogueCondition Condition;
	Condition.Key = Definition.Key;
	Condition.Comparison = EDialogueComparison::Greater;
	Condition.ValueOperand = 0;
	Info->Conditions.Conditions.Add(Condition);
	FString Error;
	if (FDialogueGraphCodec::ExportGraph(Graph, Asset, Error) == false) return false;
	UDialogueDiagnosticRouter* Router = NewObject<UDialogueDiagnosticRouter>();
	int32 Diagnostics = 0;
	Router->OnDiagnosticNative.AddLambda([&](const FDialogueDiagnostic&) { ++Diagnostics; });
	UDialogueBlockRunner* Runner = NewObject<UDialogueBlockRunner>();
	Runner->StartBlock(Asset, Provider, nullptr, Router);
	TestTrue(TEXT("Direct Branch only"), Runner->GetDirectNextType() == EDialogueType::BranchNode && Runner->GetPresentation().Choices.IsEmpty());
	TestEqual(TEXT("No evaluation on preview"), Diagnostics, 0);
	TestTrue(TEXT("Evaluate false"), Runner->AdvanceLine());
	TestTrue(TEXT("False destination"), Runner->GetPresentation().NodeGuid == FalseLine->NodeGuid);
	Graph->GetSchema()->TryCreateConnection(FalseLine->Pins[1], Branch->Pins[0]);
	if (FDialogueGraphCodec::ExportGraph(Graph, Asset, Error) == false) return false;
	Runner = NewObject<UDialogueBlockRunner>();
	Runner->StartBlock(Asset, Provider, nullptr, Router);
	Runner->AdvanceLine();
	TestTrue(TEXT("Same Branch allowed after player input"), Runner->AdvanceLine());
	TestTrue(TEXT("Normal Branch loop remains waiting"), Runner->GetPresentation().NodeGuid == FalseLine->NodeGuid && Runner->GetPresentation().State == EDialogueRunState::WaitingLine);
	Info->Conditions.Conditions.Reset();
	if (FDialogueGraphCodec::ExportGraph(Graph, Asset, Error) == false) return false;
	Runner = NewObject<UDialogueBlockRunner>();
	Runner->StartBlock(Asset, Provider, nullptr, Router);
	TestEqual(TEXT("Empty branch not evaluated during Line"), Diagnostics, 0);
	Runner->AdvanceLine();
	TestTrue(TEXT("Empty branch True"), Runner->GetPresentation().NodeGuid == TrueLine->NodeGuid);
	TestEqual(TEXT("Empty branch diagnostic"), Diagnostics, 1);
	Info->Conditions.Conditions.Add(Condition);
	if (FDialogueGraphCodec::ExportGraph(Graph, Asset, Error) == false) return false;
	Runner = NewObject<UDialogueBlockRunner>();
	Runner->StartBlock(Asset, nullptr, nullptr, Router);
	Runner->AdvanceLine();
	TestTrue(TEXT("Missing provider substitutes True"), Runner->GetPresentation().NodeGuid == TrueLine->NodeGuid);
	Info->Conditions.Conditions.Reset();
	UDialogueEdGraphNodeBase* BranchB = AddFlowTestNode(Graph, UDialogueBranchEdGraphNode::StaticClass(), Branch->Pins[1]);
	Graph->GetSchema()->TryCreateConnection(BranchB->Pins[1], Branch->Pins[0]);
	if (FDialogueGraphCodec::ExportGraph(Graph, Asset, Error) == false) return false;
	Runner = NewObject<UDialogueBlockRunner>();
	Runner->StartBlock(Asset, Provider, nullptr, Router);
	Diagnostics = 0;
	TestFalse(TEXT("Automatic cycle stops"), Runner->AdvanceLine());
	TestEqual(TEXT("Two evaluations plus cycle diagnostic only"), Diagnostics, 3);
	TestEqual(TEXT("Cycle code"), Runner->GetPresentation().ErrorCode, FName(TEXT("AutomaticBranchCycle")));
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDialogueStateChangeFlowTest,
	"DialogueKit.Runtime.StateChangeBoundary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// 사전 조회 무변경, 실제 진입 효과, 후속 실행 전 경계 및 실패 무진행을 확인한다.
bool FDialogueStateChangeFlowTest::RunTest(const FString& Parameters)
{
	UDialogueGraph* Asset = NewObject<UDialogueGraph>();
	Asset->InitializeBlockIdentity();
	UEdGraph* Graph = NewObject<UEdGraph>(Asset);
	Graph->Schema = UDialogueEdGraphSchema::StaticClass();
	Graph->GetSchema()->CreateDefaultNodesForGraph(*Graph);
	UDialogueEdGraphNodeBase* Line = AddFlowTestNode(Graph, UDialogueFlowEdGraphNode::StaticClass(), Graph->Nodes[0]->Pins[0]);
	UDialogueEdGraphNodeBase* Change = AddFlowTestNode(Graph, UDialogueStateChangeEdGraphNode::StaticClass(), Line->Pins[1]);
	UDialogueEdGraphNodeBase* Choice = AddFlowTestNode(Graph, UDialogueChoiceEdGraphNode::StaticClass(), Change->Pins[1]);
	Graph->GetSchema()->TryCreateConnection(Choice->Pins[1], Line->Pins[0]);
	UDialogueSampleEffectProvider* Provider = NewObject<UDialogueSampleEffectProvider>();
	FDialogueStateDefinition Definition;
	Definition.Key.Kind = EDialogueStateKind::Count;
	Definition.Key.ItemId = TEXT("Coin");
	Provider->AddDefinition(Definition);
	FDialogueEffect Effect;
	Effect.Key = Definition.Key;
	Effect.Count = 1;
	CastChecked<UDialogueStateChangeNodeInfo>(Change->GetNodeInfo())->Effects.Add(Effect);
	UDialogueChoiceNodeInfo* ChoiceInfo = CastChecked<UDialogueChoiceNodeInfo>(Choice->GetNodeInfo());
	ChoiceInfo->Options[0].Effects.Add(Effect);
	const FGuid ChoiceId = ChoiceInfo->Options[0].ChoiceId;
	FString Error;
	if (FDialogueGraphCodec::ExportGraph(Graph, Asset, Error) == false) return false;
	UDialogueBlockRunner* Runner = NewObject<UDialogueBlockRunner>();
	int32 Boundaries = 0;
	TSet<FGuid> Executions;
	Runner->OnExecutionBoundaryNative.AddLambda([&](const FDialogueExecutionBoundary& Boundary)
	{
		++Boundaries;
		TestTrue(TEXT("Execution ID unique for normal occurrence"), Boundary.Context.ExecutionId.IsValid() && Executions.Contains(Boundary.Context.ExecutionId) == false);
		Executions.Add(Boundary.Context.ExecutionId);
		TestTrue(TEXT("Save requested after successful effect"), Boundary.bSaveRequested && Boundary.bHasEffects && Boundary.NextNodeGuid.IsValid());
		int64 Count = 0;
		Provider->QueryDialogueCount_Implementation(NAME_None, TEXT("Coin"), Count);
		TestEqual(TEXT("State is available at boundary"), Count, static_cast<int64>(Boundaries));
		TestFalse(TEXT("Boundary cannot reenter progression"), Runner->AdvanceLine());
	});
	TestTrue(TEXT("Start"), Runner->StartBlock(Asset, Provider, Provider, nullptr));
	TestTrue(TEXT("Preview StateChange type only"), Runner->GetDirectNextType() == EDialogueType::StateChangeNode);
	int64 Count = -1;
	Provider->QueryDialogueCount_Implementation(NAME_None, TEXT("Coin"), Count);
	TestEqual(TEXT("No effect during preview"), Count, int64(0));
	TestEqual(TEXT("No boundary during preview"), Boundaries, 0);
	TestTrue(TEXT("StateChange enters next Choice"), Runner->AdvanceLine());
	TestTrue(TEXT("Choice now waiting"), Runner->GetPresentation().State == EDialogueRunState::WaitingChoice);
	TestEqual(TEXT("One state boundary"), Boundaries, 1);
	TestTrue(TEXT("Choice effect and loop"), Runner->SelectChoice(ChoiceId));
	TestEqual(TEXT("Choice boundary"), Boundaries, 2);
	TestTrue(TEXT("Normal repeated StateChange allowed"), Runner->AdvanceLine());
	TestEqual(TEXT("Repeated effect new execution"), Boundaries, 3);
	UDialogueBlockRunner* Failed = NewObject<UDialogueBlockRunner>();
	int32 FailedBoundaries = 0;
	Failed->OnExecutionBoundaryNative.AddLambda([&](const FDialogueExecutionBoundary&) { ++FailedBoundaries; });
	Failed->StartBlock(Asset, Provider, nullptr, nullptr);
	TestFalse(TEXT("Provider failure stops"), Failed->AdvanceLine());
	TestTrue(TEXT("Failed not completed"), Failed->GetPresentation().State == EDialogueRunState::Failed);
	TestEqual(TEXT("No success checkpoint boundary"), FailedBoundaries, 0);
	Change->Pins[1]->BreakAllPinLinks();
	if (FDialogueGraphCodec::ExportGraph(Graph, Asset, Error) == false) return false;
	Failed = NewObject<UDialogueBlockRunner>();
	Failed->StartBlock(Asset, Provider, Provider, nullptr);
	TestFalse(TEXT("Broken continuation fails before effect"), Failed->AdvanceLine());
	Provider->QueryDialogueCount_Implementation(NAME_None, TEXT("Coin"), Count);
	TestEqual(TEXT("No grant on broken continuation"), Count, int64(3));
	Graph->GetSchema()->TryCreateConnection(Change->Pins[1], Choice->Pins[0]);
	CastChecked<UDialogueStateChangeNodeInfo>(Change->GetNodeInfo())->Effects[0].Key.ItemId = NAME_None;
	if (FDialogueGraphCodec::ExportGraph(Graph, Asset, Error) == false) return false;
	Failed = NewObject<UDialogueBlockRunner>();
	Failed->StartBlock(Asset, Provider, Provider, nullptr);
	TestFalse(TEXT("Invalid effect fails"), Failed->AdvanceLine());
	Provider->QueryDialogueCount_Implementation(NAME_None, TEXT("Coin"), Count);
	TestEqual(TEXT("Invalid effect leaves state unchanged"), Count, int64(3));
	// 효과 없는 선택은 기록 경계를 보내되 실행 ID는 발급하지 않는다.
	Graph->GetSchema()->TryCreateConnection(Line->Pins[1], Choice->Pins[0]);
	ChoiceInfo->Options[0].Effects.Reset();
	for (const bool bSave : {false, true})
	{
		ChoiceInfo->bSaveOnSelection = bSave;
		if (FDialogueGraphCodec::ExportGraph(Graph, Asset, Error) == false) return false;
		UDialogueBlockRunner* NoEffect = NewObject<UDialogueBlockRunner>();
		int32 Records = 0;
		NoEffect->OnExecutionBoundaryNative.AddLambda([&](const FDialogueExecutionBoundary& Boundary)
		{
			++Records;
			TestTrue(TEXT("No effect has no execution ID"), Boundary.bHasEffects == false && Boundary.Context.ExecutionId.IsValid() == false);
			TestTrue(TEXT("Selection record has origin and next"), Boundary.ChoiceId == ChoiceId && Boundary.NextNodeGuid == Line->NodeGuid);
			TestEqual(TEXT("Explicit save flag"), Boundary.bSaveRequested, bSave);
		});
		NoEffect->StartBlock(Asset, Provider, Provider, nullptr);
		TestTrue(TEXT("No-effect choice accepted"), NoEffect->SelectChoice(ChoiceId));
		TestEqual(TEXT("One record boundary"), Records, 1);
	}
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDialogueAllNodesRoundtripTest,
	"DialogueKit.Graph.AllNodesPackageAndUIBridge",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// 모든 신규 노드를 실제 파일로 왕복하고 Subsystem/Blueprint UI 이벤트를 통해 실행한다.
bool FDialogueAllNodesRoundtripTest::RunTest(const FString& Parameters)
{
	const FString Id = FGuid::NewGuid().ToString(EGuidFormats::Digits);
	const FString Mount = TEXT("/DialogueFlowTest") + Id + TEXT("/");
	const FString Folder = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("AI/GraphTests") / Id) + TEXT("/");
	IFileManager::Get().MakeDirectory(*Folder, true);
	FPackageName::RegisterMountPoint(Mount, Folder);
	const FString PackageName = Mount + TEXT("AllNodes");
	UPackage* Package = CreatePackage(*PackageName);
	UDialogueGraph* Asset = NewObject<UDialogueGraph>(Package, TEXT("AllNodes"), RF_Public | RF_Standalone);
	Asset->InitializeBlockIdentity();
	UEdGraph* Graph = NewObject<UEdGraph>(Asset);
	Graph->Schema = UDialogueEdGraphSchema::StaticClass();
	Graph->GetSchema()->CreateDefaultNodesForGraph(*Graph);
	UDialogueEdGraphNodeBase* Line = AddFlowTestNode(Graph, UDialogueFlowEdGraphNode::StaticClass(), Graph->Nodes[0]->Pins[0]);
	UDialogueEdGraphNodeBase* Choice = AddFlowTestNode(Graph, UDialogueChoiceEdGraphNode::StaticClass(), Line->Pins[1]);
	UDialogueEdGraphNodeBase* Branch = AddFlowTestNode(Graph, UDialogueBranchEdGraphNode::StaticClass(), Choice->Pins[1]);
	UDialogueEdGraphNodeBase* Change = AddFlowTestNode(Graph, UDialogueStateChangeEdGraphNode::StaticClass(), Branch->Pins[1]);
	UDialogueEdGraphNodeBase* LastLine = AddFlowTestNode(Graph, UDialogueFlowEdGraphNode::StaticClass(), Change->Pins[1]);
	UDialogueEdGraphNodeBase* End = AddFlowTestNode(Graph, UDialogueEdEndGraphNode::StaticClass(), LastLine->Pins[1]);
	Graph->GetSchema()->TryCreateConnection(Branch->Pins[2], End->Pins[0]);
	UDialogueChoiceNodeInfo* ChoiceInfo = CastChecked<UDialogueChoiceNodeInfo>(Choice->GetNodeInfo());
	ChoiceInfo->Options[0].Text = FText::FromString(TEXT("Answer"));
	ChoiceInfo->bSaveOnSelection = true;
	Choice->OnPropertiesChanged();
	const FGuid ChoiceId = ChoiceInfo->Options[0].ChoiceId;
	const FGuid LastLineId = LastLine->NodeGuid;
	FDialogueEffect Effect;
	Effect.Key.Kind = EDialogueStateKind::Count;
	Effect.Key.ItemId = TEXT("Coin");
	Effect.Count = 2;
	ChoiceInfo->Options[0].Effects.Add(Effect);
	Effect.Count = 3;
	CastChecked<UDialogueStateChangeNodeInfo>(Change->GetNodeInfo())->Effects.Add(Effect);
	CastChecked<UDialogueEndNodeInfo>(End->GetNodeInfo())->bSaveOnEnd = true;
	FDialogueCondition Condition;
	Condition.Key.Kind = EDialogueStateKind::Value;
	Condition.Key.ItemId = TEXT("Level");
	CastChecked<UDialogueBranchNodeInfoBase>(Branch->GetNodeInfo())->Conditions.Conditions.Add(Condition);
	FString Error;
	if (TestTrue(TEXT("All nodes export"), FDialogueGraphCodec::ExportGraph(Graph, Asset, Error)) == false) return false;
	TArray<FGuid> NodeIds;
	TArray<FGuid> PinIds;
	for (UDialogueRuntimeNode* Node : Asset->Graph->Nodes)
	{
		NodeIds.Add(Node->NodeGuid);
		for (UDialogueRuntimePin* Pin : Node->OutputPins) PinIds.Add(Pin->PinId);
	}
	FSavePackageArgs Args;
	Args.TopLevelFlags = RF_Public | RF_Standalone;
	Args.SaveFlags = SAVE_NoError;
	if (TestTrue(TEXT("All nodes saved"), UPackage::SavePackage(Package, Asset, *(Folder + TEXT("AllNodes.uasset")), Args)) == false) return false;
	TArray<UPackage*> Packages = {Package};
	if (TestTrue(TEXT("All nodes unloaded"), UPackageTools::UnloadPackages(Packages)) == false) return false;
	UPackage* Reloaded = LoadPackage(nullptr, *PackageName, LOAD_None);
	UDialogueGraph* Loaded = Reloaded ? FindObject<UDialogueGraph>(Reloaded, TEXT("AllNodes")) : nullptr;
	if (TestNotNull(TEXT("All nodes reloaded"), Loaded) == false) return false;
	UEdGraph* Reopened = NewObject<UEdGraph>(Loaded);
	Reopened->Schema = UDialogueEdGraphSchema::StaticClass();
	if (TestTrue(TEXT("All nodes import"), FDialogueGraphCodec::ImportGraph(Loaded, Reopened, Error)) == false) return false;
	if (TestTrue(TEXT("All nodes re-export"), FDialogueGraphCodec::ExportGraph(Reopened, Loaded, Error)) == false) return false;
	TArray<FGuid> ReopenedNodes;
	TArray<FGuid> ReopenedPins;
	for (UDialogueRuntimeNode* Node : Loaded->Graph->Nodes)
	{
		ReopenedNodes.Add(Node->NodeGuid);
		for (UDialogueRuntimePin* Pin : Node->OutputPins) ReopenedPins.Add(Pin->PinId);
	}
	TestTrue(TEXT("Node identities preserved"), NodeIds == ReopenedNodes);
	TestTrue(TEXT("Pin identities preserved"), PinIds == ReopenedPins);
	TestTrue(TEXT("Standalone Choice exported for localization"), FDialogueLocalizationCSVConverter::BuildDialogueGraphCSVFromAsset(Loaded).Contains(TEXT("Answer")));
	UDialogueSampleEffectProvider* Provider = NewObject<UDialogueSampleEffectProvider>();
	FDialogueStateDefinition Definition;
	Definition.Key = Effect.Key;
	Provider->AddDefinition(Definition);
	Definition.Key = Condition.Key;
	Provider->AddDefinition(Definition);
	UGameInstance* GameInstance = NewObject<UGameInstance>();
	UDialogueSubsystem* Subsystem = NewObject<UDialogueSubsystem>(GameInstance);
	UDialoguePresentationProbe* Probe = NewObject<UDialoguePresentationProbe>();
	Subsystem->OnBlockPresentationChanged.AddDynamic(Probe, &UDialoguePresentationProbe::Receive);
	Subsystem->ConfigureBlockProviders(Provider, Provider, nullptr);
	int32 Boundaries = 0;
	int32 Completions = 0;
	Subsystem->GetBlockRunner()->OnExecutionBoundaryNative.AddLambda([&](const FDialogueExecutionBoundary& Boundary)
	{
		++Boundaries;
		if (Boundary.bBlockCompleted)
		{
			++Completions;
			TestTrue(TEXT("End requests configured save"), Boundary.bSaveRequested && Boundary.Context.ExecutionId.IsValid() == false && Boundary.NextNodeGuid.IsValid() == false);
			TestFalse(TEXT("Completion callback cannot replace active context"), Subsystem->StartDialogueBlock(Loaded));
		}
	});
	TestTrue(TEXT("Subsystem starts new graph"), Subsystem->StartDialogueBlock(Loaded));
	TestTrue(TEXT("Dynamic UI receives line and Choice"), Probe->Events == 1 && Probe->LastPresentation.Line && Probe->LastPresentation.Choices.Num() == 1);
	TArray<FText> Texts;
	TArray<int32> Indices;
	Subsystem->GetSelectableChoiceTexts(nullptr, Texts, Indices);
	TestTrue(TEXT("Legacy UI choice adapter"), Texts.Num() == 1 && Texts[0].ToString() == TEXT("Answer") && Indices[0] == 0);
	TestTrue(TEXT("ID selection through Subsystem"), Subsystem->SelectBlockChoice(ChoiceId));
	TestTrue(TEXT("Branch and StateChange reach next Line"), Probe->LastPresentation.NodeGuid == LastLineId && Probe->Events == 2);
	int64 Count = 0;
	Provider->QueryDialogueCount_Implementation(NAME_None, TEXT("Coin"), Count);
	TestEqual(TEXT("Choice and StateChange effects preserved"), Count, int64(5));
	TestTrue(TEXT("End via legacy progression adapter"), Subsystem->ProgressNextDialogue() == nullptr && Probe->LastPresentation.State == EDialogueRunState::Completed);
	TestEqual(TEXT("Choice state and End boundaries"), Boundaries, 3);
	TestEqual(TEXT("Single completion"), Completions, 1);
	TestEqual(TEXT("UI receives completion"), Probe->Events, 3);
	// 프로세스 수명까지 마운트를 유지하여 저장 후 지연 검증도 같은 파일을 확인한다.
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDialogueCompletionAndMixedGraphTest,
	"DialogueKit.Runtime.DefaultEndAndMixedGraph",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// 기본 End는 저장 요청 없이 완료하며 혼합 Graph는 효과 전에 거부한다.
bool FDialogueCompletionAndMixedGraphTest::RunTest(const FString& Parameters)
{
	UDialogueGraph* Asset = NewObject<UDialogueGraph>();
	Asset->InitializeBlockIdentity();
	UEdGraph* Graph = NewObject<UEdGraph>(Asset);
	Graph->Schema = UDialogueEdGraphSchema::StaticClass();
	Graph->GetSchema()->CreateDefaultNodesForGraph(*Graph);
	UDialogueEdGraphNodeBase* Line = AddFlowTestNode(Graph, UDialogueFlowEdGraphNode::StaticClass(), Graph->Nodes[0]->Pins[0]);
	UDialogueEdGraphNodeBase* End = AddFlowTestNode(Graph, UDialogueEdEndGraphNode::StaticClass(), Line->Pins[1]);
	UDialogueEndNodeInfo* EndInfo = CastChecked<UDialogueEndNodeInfo>(End->GetNodeInfo());
	TestFalse(TEXT("SaveOnEnd defaults off"), EndInfo->bSaveOnEnd);
	// legacy Quest가 지정되어 있어도 새 실행기는 이 액션을 실행하지 않는다.
	EndInfo->Action = EDialogueNodeAction::StartQuest;
	FString Error;
	if (FDialogueGraphCodec::ExportGraph(Graph, Asset, Error) == false) return false;
	UGameInstance* GameInstance = NewObject<UGameInstance>();
	UDialogueSubsystem* Subsystem = NewObject<UDialogueSubsystem>(GameInstance);
	int32 Completed = 0;
	Subsystem->GetBlockRunner()->OnExecutionBoundaryNative.AddLambda([&](const FDialogueExecutionBoundary& Boundary)
	{
		++Completed;
		TestTrue(TEXT("Completion without save or effect"), Boundary.bBlockCompleted && Boundary.bSaveRequested == false && Boundary.bHasEffects == false);
	});
	TestTrue(TEXT("New block start"), Subsystem->StartDialogueBlock(Asset));
	Subsystem->ProgressNextDialogue();
	TestEqual(TEXT("Exactly one default completion"), Completed, 1);
	TestTrue(TEXT("Completed without legacy Quest subsystem"), Subsystem->GetBlockPresentation().State == EDialogueRunState::Completed);
	AddFlowTestNode(Graph, UDialogueEdGraphNode::StaticClass());
	if (FDialogueGraphCodec::ExportGraph(Graph, Asset, Error) == false) return false;
	TestFalse(TEXT("Mixed legacy/new rejected before any presentation or effect"), Subsystem->StartDialogueBlock(Asset));
	TestEqual(TEXT("Mixed graph diagnostic"), Subsystem->GetBlockPresentation().ErrorCode, FName(TEXT("UnsupportedNodeType")));
	TestEqual(TEXT("No completion on failed graph"), Completed, 1);
	return true;
}
#endif
