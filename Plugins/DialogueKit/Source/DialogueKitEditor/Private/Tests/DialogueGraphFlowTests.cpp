#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "DialogueGraph.h"
#include "DialogueGraphCodec.h"
#include "DialogueEdGraphSchema.h"
#include "DialogueEdStartGraphNode.h"
#include "DialogueEdEndGraphNode.h"
#include "DialogueFlowEdGraphNode.h"
#include "DialogueLineNodeInfo.h"
#include "DialogueBlockRunner.h"
#include "DialogueChoiceEdGraphNode.h"
#include "DialogueChoiceNodeInfo.h"
#include "DialogueSampleStateProvider.h"
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
	CastChecked<UDialogueChoiceNodeInfo>(Asset->Graph->Nodes[2]->NodeInfo)->Options[0].Conditions.Conditions.Add(Condition);
	TestFalse(TEXT("Stale choice returns false"), Runner->SelectChoice(OrdinaryId));
	TestTrue(TEXT("No permanent wait after all choices disappear"), Runner->GetPresentation().State == EDialogueRunState::Completed);
	return true;
}
#endif
