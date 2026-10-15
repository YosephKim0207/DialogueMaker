#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "DialogueGraph.h"
#include "DialogueGraphCodec.h"
#include "DialogueEdGraphSchema.h"
#include "DialogueFlowEdGraphNode.h"
#include "DialogueRerouteNode.h"
#include "DialogueLineAuthoring.h"
#include "EdGraphNode_Comment.h"
#include "Editor.h"
#include "DialogueCommentNode.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "PackageTools.h"
#include "UObject/SavePackage.h"
#include "UObject/Package.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDialogueLayoutTest, "DialogueKit.Editor.LayoutRoundtrip", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// 편집 배치와 실행 연결을 분리하고 잘못된 배치 저장은 기존 정본을 보존한다.
bool FDialogueLayoutTest::RunTest(const FString& Parameters)
{
	auto* Block = NewObject<UDialogueGraph>(GetTransientPackage(), NAME_None, RF_Transactional);
	Block->InitializeBlockIdentity();
	auto* Graph = NewObject<UEdGraph>(Block, NAME_None, RF_Transactional);
	Graph->Schema = UDialogueEdGraphSchema::StaticClass();
	const auto* Schema = CastChecked<UDialogueEdGraphSchema>(Graph->GetSchema());
	Schema->CreateDefaultNodesForGraph(*Graph);
	FNewNodeAction Add(UDialogueFlowEdGraphNode::StaticClass(), FText(), FText(), FText(), 0);
	auto* A = Add.PerformAction(Graph, Graph->Nodes[0]->Pins[0], FVector2D::ZeroVector);
	auto* B = Add.PerformAction(Graph, A->Pins[1], FVector2D(400, 0));
	Schema->OnPinConnectionDoubleCicked(A->Pins[1], B->Pins[0], FVector2D(200, 50));
	auto* Reroute = CastChecked<UDialogueRerouteNode>(A->Pins[1]->LinkedTo[0]->GetOwningNode());
	TestTrue(TEXT("Reroute points to destination"), Reroute->Pins[1]->LinkedTo.Contains(B->Pins[0]));
	TestTrue(TEXT("Undo insert"), GEditor->UndoTransaction());
	TestTrue(TEXT("Direct edge restored"), A->Pins[1]->LinkedTo.Contains(B->Pins[0]));
	TestTrue(TEXT("Redo insert"), GEditor->RedoTransaction());
	TestTrue(TEXT("Reroute restored"), Graph->Nodes.Contains(Reroute));
	TestTrue(TEXT("Continuation crosses reroute"), FDialogueLineAuthoring::FindNearestLines(Reroute->Pins[1]).Contains(CastChecked<UDialogueFlowEdGraphNode>(A)));
	auto* Comment = NewObject<UEdGraphNode_Comment>(Graph, NAME_None, RF_Transactional);
	Comment->CreateNewGuid();
	Comment->NodeComment = TEXT("테스트 영역");
	Comment->NodeWidth = 640;
	Comment->CommentColor = FLinearColor::Green;
	FDialogueCommentDetails::SetDescription(Comment, FText::FromString(TEXT("분기 설명")));
	Graph->AddNode(Comment);
	FString Error;
	TestTrue(TEXT("Layout export"), FDialogueGraphCodec::ExportGraph(Graph, Block, Error));
	TestEqual(TEXT("Runtime excludes layout nodes"), Block->Graph->Nodes.Num(), 3);
	TestEqual(TEXT("Layout stores both nodes"), Block->EditorLayout.Nodes.Num(), 2);
	auto* Imported = NewObject<UEdGraph>(Block);
	Imported->Schema = UDialogueEdGraphSchema::StaticClass();
	TestTrue(TEXT("Layout import"), FDialogueGraphCodec::ImportGraph(Block, Imported, Error));
	TestEqual(TEXT("All editor nodes restored"), Imported->Nodes.Num(), 5);
	for (UEdGraphNode* Node : Imported->Nodes)
	{
		if (Node->NodeGuid == Comment->NodeGuid)
		{
			TestEqual(TEXT("Comment text restored"), Node->NodeComment, Comment->NodeComment);
			TestEqual(TEXT("Comment size restored"), Node->NodeWidth, 640);
			TestEqual(TEXT("Comment description restored"), FDialogueCommentDetails::GetDescription(Node).ToString(), FString(TEXT("분기 설명")));
		}
	}
	const FGuid SavedTarget = Block->EditorLayout.Edges[0].InputId;
	Block->EditorLayout.Edges[0].InputId = FGuid::NewGuid();
	TestFalse(TEXT("Unknown layout target rejected"), FDialogueGraphCodec::ImportGraph(Block, Imported, Error));
	TestEqual(TEXT("Failed import preserves editor nodes"), Imported->Nodes.Num(), 5);
	Block->EditorLayout.Edges[0].InputId = SavedTarget;
	UDialogueRuntimeGraph* Previous = Block->Graph;
	Reroute->Pins[1]->BreakAllPinLinks();
	Reroute->Pins[1]->MakeLinkTo(Reroute->Pins[0]);
	TestFalse(TEXT("Reroute cycle rejected"), FDialogueGraphCodec::ExportGraph(Graph, Block, Error));
	TestTrue(TEXT("Rejected export preserves runtime"), Block->Graph == Previous);
	TestEqual(TEXT("Rejected export preserves layout"), Block->EditorLayout.Nodes.Num(), 2);
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDialogueLayoutFileTest, "DialogueKit.Editor.LayoutFileRoundtrip", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// 격리된 파일에서 재열어 배치 자료가 메모리 캐시가 아닌 실제 에셋에 남는지 확인한다.
bool FDialogueLayoutFileTest::RunTest(const FString& Parameters)
{
	const FString Id = FGuid::NewGuid().ToString(EGuidFormats::Digits);
	const FString Mount = TEXT("/DialogueLayoutTest") + Id + TEXT("/");
	const FString Folder = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("AI/LayoutTests") / Id) + TEXT("/");
	IFileManager::Get().MakeDirectory(*Folder, true);
	FPackageName::RegisterMountPoint(Mount, Folder);
	UPackage* Package = CreatePackage(*(Mount + TEXT("Layout")));
	auto* Asset = NewObject<UDialogueGraph>(Package, TEXT("Layout"), RF_Public | RF_Standalone);
	Asset->InitializeBlockIdentity();
	auto* Graph = NewObject<UEdGraph>(Asset);
	Graph->Schema = UDialogueEdGraphSchema::StaticClass();
	const auto* Schema = CastChecked<UDialogueEdGraphSchema>(Graph->GetSchema());
	Schema->CreateDefaultNodesForGraph(*Graph);
	FNewNodeAction Add(UDialogueFlowEdGraphNode::StaticClass(), FText(), FText(), FText(), 0);
	auto* Line = Add.PerformAction(Graph, Graph->Nodes[0]->Pins[0], FVector2D(200, 0));
	Schema->OnPinConnectionDoubleCicked(Graph->Nodes[0]->Pins[0], Line->Pins[0], FVector2D(100, 50));
	const FGuid KnotId = Graph->Nodes.Last()->NodeGuid;
	FString Error;
	TestTrue(TEXT("Export file fixture"), FDialogueGraphCodec::ExportGraph(Graph, Asset, Error));
	FSavePackageArgs Args;
	Args.TopLevelFlags = RF_Public | RF_Standalone;
	Args.SaveFlags = SAVE_NoError;
	TestTrue(TEXT("Save layout package"), UPackage::SavePackage(Package, Asset, *(Folder + TEXT("Layout.uasset")), Args));
	TArray<UPackage*> Packages = {Package};
	if (TestTrue(TEXT("Unload layout package"), UPackageTools::UnloadPackages(Packages)))
	{
		UPackage* Reloaded = LoadPackage(nullptr, *(Mount + TEXT("Layout")), LOAD_None);
		auto* Loaded = Reloaded ? FindObject<UDialogueGraph>(Reloaded, TEXT("Layout")) : nullptr;
		if (TestNotNull(TEXT("Reload layout package"), Loaded))
		{
			auto* Reopened = NewObject<UEdGraph>(Loaded);
			Reopened->Schema = UDialogueEdGraphSchema::StaticClass();
			TestTrue(TEXT("Import file layout"), FDialogueGraphCodec::ImportGraph(Loaded, Reopened, Error));
			TestEqual(TEXT("File layout node count"), Reopened->Nodes.Num(), 3);
			TestEqual(TEXT("Stable reroute ID"), Reopened->Nodes.Last()->NodeGuid, KnotId);
			TestEqual(TEXT("Reroute position"), Reopened->Nodes.Last()->NodePosY, 50);
		}
	}
	// 저장 뒤 지연 검증이 경로를 조회하므로 격리 마운트는 프로세스 종료까지 유지한다.
	return true;
}
#endif
