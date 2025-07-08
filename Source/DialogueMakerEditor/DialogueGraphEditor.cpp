#include "DialogueGraphEditor.h"

#include "DialogueEdGraph.h"
#include "DialogueEdGraphNode.h"
#include "DialogueEdGraphSchema.h"
#include "DialogueGraphEditorMode.h"
#include "EdGraph/EdGraph.h"
#include "Widgets/Docking/SDockTab.h"
#include "PropertyEditorModule.h"
#include "Widgets/SBoxPanel.h"
#include "GraphEditor.h"
#include "DialogueMaker/DialogueGraph.h"
#include "Kismet2/BlueprintEditorUtils.h"


void FDialogueGraphEditor::RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
    FAssetEditorToolkit::RegisterTabSpawners(InTabManager);

    // InTabManager->RegisterTabSpawner(GraphTabID,
    //     FOnSpawnTab::CreateLambda([this](const FSpawnTabArgs&)
    //     {
    //         return SNew(SDockTab)
    //                .Label(NSLOCTEXT("DialogueGraph", "Graph", "Graph"))
    //                [ SAssignNew(GraphEditor, SGraphEditor)
    //                    .GraphToEdit( Graph->EdGraph ) ];
    //     }));
    //
    // InTabManager->RegisterTabSpawner(DetailsTabID, FOnSpawnTab::CreateLambda([this](const FSpawnTabArgs&)
    // {
    //     return SNew(SDockTab)
    //         .Label(FText::FromString("Details"))
    //         [
    //             DetailsView.ToSharedRef()
    //         ];
    // }));
}

void FDialogueGraphEditor::InitEditor(const EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& InitToolkitHost, UDialogueGraph* InGraph)
{
    // 시작 Dialogue Node 생성
    // if (InGraph->EdGraph == nullptr)
    // {
    //     InGraph->EdGraph = Cast<UDialogueEdGraph>(FBlueprintEditorUtils::CreateNewGraph(InGraph, NAME_None, UDialogueEdGraph::StaticClass(), UDialogueEdGraphSchema::StaticClass()));
    //     InGraph->EdGraph->bAllowDeletion = false;
    //     InGraph->EdGraph->Schema = UDialogueEdGraphSchema::StaticClass();
    // }
    
    // SGraphEditor 위젯 생성
    // SAssignNew(GraphEditor, SGraphEditor).GraphToEdit(InGraph->EdGraph);
    //
    // // 에디터 레이아웃 정의
    // const TSharedRef<FTabManager::FLayout> StandaloneDefaultLayout = FTabManager::NewLayout("Standalone_DialogueGraphEditor_Layout_v1")
    //     ->AddArea
    //     (
    //         FTabManager::NewPrimaryArea()->SetOrientation(Orient_Vertical)
    //         ->Split
    //         (
    //             FTabManager::NewSplitter()
    //             ->SetOrientation(Orient_Horizontal)
    //             ->SetSizeCoefficient(0.7f)
    //             ->Split
    //             (
    //                 FTabManager::NewStack()
    //                 ->SetSizeCoefficient(0.8f)
    //                 ->AddTab("GraphEditor", ETabState::OpenedTab)
    //             )
    //             ->Split
    //             (
    //                 FTabManager::NewStack()
    //                 ->SetSizeCoefficient(0.2f)
    //                 ->AddTab("Details", ETabState::OpenedTab)
    //             )
    //         )
    //     );


    // Details 탭 생성
    // FPropertyEditorModule& PropertyEditorModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
    // FDetailsViewArgs DetailsViewArgs;
    // DetailsViewArgs.bUpdatesFromSelection = true;
    // DetailsViewArgs.bLockable = false;
    // DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::ObjectsUseNameArea;
    //
    // DetailsView = PropertyEditorModule.CreateDetailView(DetailsViewArgs);
    // DetailsView->SetObject(InGraph); // 디테일 뷰에 우리 에셋을 표시

    // InitAssetEditor(Mode, InitToolkitHost, FName("DialogueGraphEditor"), StandaloneDefaultLayout, true, true, InGraph);
    
    // 탭 스포너 등록
    // TabManager->RegisterTabSpawner(GraphTabID, FOnSpawnTab::CreateLambda([this](const FSpawnTabArgs&)
    // {
    //     return SNew(SDockTab)
    //         .Label(FText::FromString("Graph"))
    //         [
    //             GraphEditor.ToSharedRef()
    //         ];
    // }));
    //
    // TabManager->RegisterTabSpawner(DetailsTabID, FOnSpawnTab::CreateLambda([this](const FSpawnTabArgs&)
    // {
    //     return SNew(SDockTab)
    //         .Label(FText::FromString("Details"))
    //         [
    //             DetailsView.ToSharedRef()
    //         ];
    // }));

    WorkingAsset = InGraph;
    WorkingGraph = FBlueprintEditorUtils::CreateNewGraph(WorkingAsset, NAME_None, UEdGraph::StaticClass(), UDialogueEdGraphSchema::StaticClass());

    
    InitAssetEditor(Mode, InitToolkitHost, FName("DialogueGraphEditor"), FTabManager::FLayout::NullLayout, true, true, InGraph);

    // Editor의 Mode를 DialogueGraphEditorMode로 설정
    AddApplicationMode(TEXT("DialogueGraphEditorMode"), MakeShareable(new DialogueGraphEditorMode(SharedThis(this))));
    SetCurrentMode(TEXT("DialogueGraphEditorMode"));

    UpdateEditorGraphFromWorkingAsset();

    GraphChangeListenerHandle = WorkingGraph->AddOnGraphChangedHandler(
        FOnGraphChanged::FDelegate::CreateSP(this, &FDialogueGraphEditor::OnGraphChanged)
    );
}

void FDialogueGraphEditor::OnNodeSelectionChanged(const TSet<UObject*>& NewSelection)
{
    TArray<UObject*> Array = NewSelection.Array();
    // DetailsView->SetObjects(Array.Num() ? Array : TArray<UObject*>{ GraphAsset });
}

void FDialogueGraphEditor::UpdateWorkingAssetFromGraph()
{
    if (WorkingAsset == nullptr || WorkingGraph == nullptr)
    {
        return;
    }

    UDialogueRuntimeGraph* RuntimeGraph = NewObject<UDialogueRuntimeGraph>(WorkingAsset);
    WorkingAsset->Graph = RuntimeGraph;

    TArray<std::pair<FGuid, FGuid>> Connections;
    TMap<FGuid, UDialogueRuntimePin*> IdToPinMap;

    // Node Data 정리
    for (UEdGraphNode* Node : WorkingGraph->Nodes)
    {
        UDialogueRuntimeNode* RuntimeNode = NewObject<UDialogueRuntimeNode>(WorkingGraph);
        RuntimeNode->Position = FVector2D(Node->NodePosX, Node->NodePosY);

        for (UEdGraphPin* Pin : Node->Pins)
        {
            UDialogueRuntimePin* RuntimePin = NewObject<UDialogueRuntimePin>(RuntimeNode);
            RuntimePin->PinName = Pin->PinName;
            RuntimePin->PinId = Pin->PinId;

            // Pin 연결 관계 캐싱
            if (Pin->HasAnyConnections() && Pin->Direction == EGPD_Output)
            {
                for (UEdGraphPin* LinkToPin : Pin->LinkedTo)
                {
                    std::pair<FGuid, FGuid> Connection = std::make_pair(Pin->PinId, LinkToPin->PinId);
                    Connections.Add(Connection);
                }
            }
            
            IdToPinMap.Add(Pin->PinId, RuntimePin);

            if (Pin->Direction == EGPD_Input)
            {
                RuntimeNode->InputPin = RuntimePin;
            }
            else
            {
                RuntimeNode->OutputPins.Add(RuntimePin);
            }
        }

        RuntimeGraph->Nodes.Add(RuntimeNode);
    }

    for (std::pair<FGuid, FGuid> Connection : Connections)
    {
        UDialogueRuntimePin* Pin1 = IdToPinMap[Connection.first];
        UDialogueRuntimePin* Pin2 = IdToPinMap[Connection.second];
        Pin1->Connections.Add(Pin2);
    }
}

void FDialogueGraphEditor::UpdateEditorGraphFromWorkingAsset()
{
    if (WorkingAsset->Graph == nullptr)
    {
        return;
    }

    TArray<std::pair<FGuid, FGuid>> Connections;
    TMap<FGuid, UEdGraphPin*> IdToPinMap;

    for (UDialogueRuntimeNode* RuntimeNode : WorkingAsset->Graph->Nodes)
    {
        UDialogueEdGraphNode* NewNode = NewObject<UDialogueEdGraphNode>(WorkingGraph);
        NewNode->CreateNewGuid();
        NewNode->NodePosX = RuntimeNode->Position.X;
        NewNode->NodePosY = RuntimeNode->Position.Y;

        if (RuntimeNode->InputPin != nullptr)
        {
            UDialogueRuntimePin* RuntimeInputPin = RuntimeNode->InputPin;
            UEdGraphPin* Pin = NewNode->CreateCustomPin(EGPD_Input, RuntimeInputPin->PinName);
            Pin->PinId = RuntimeInputPin->PinId;

            if (RuntimeInputPin->Connections.Num() > 0)
            {
                for (UDialogueRuntimePin* ConnectPin : RuntimeInputPin->Connections)
                {
                    Connections.Add(std::make_pair(RuntimeInputPin->PinId, ConnectPin->PinId));
                }
            }

            IdToPinMap.Add(RuntimeInputPin->PinId, Pin);

            for (UDialogueRuntimePin* RuntimeOutputPin : RuntimeNode->OutputPins)
            {
                UEdGraphPin* OutputPin = NewNode->CreateCustomPin(EGPD_Output, RuntimeOutputPin->PinName);
                OutputPin->PinId = RuntimeOutputPin->PinId;

                if (RuntimeOutputPin->Connections.Num() > 0)
                {
                    for (UDialogueRuntimePin* ConnectPin : RuntimeOutputPin->Connections)
                    {
                        Connections.Add(std::make_pair(RuntimeOutputPin->PinId, ConnectPin->PinId));
                    }
                }
                
                IdToPinMap.Add(RuntimeOutputPin->PinId, OutputPin);
            }

            WorkingGraph->AddNode(NewNode, true, true);
        }
    }

    for (std::pair<FGuid, FGuid> Connection : Connections)
    {
        UEdGraphPin* FromPin = IdToPinMap[Connection.first];
        UEdGraphPin* ToPin = IdToPinMap[Connection.second];
        FromPin->LinkedTo.Add(ToPin);
        ToPin->LinkedTo.Add(FromPin);
    }
}

void FDialogueGraphEditor::OnClose()
{
    UpdateWorkingAssetFromGraph();
    WorkingGraph->RemoveOnGraphChangedHandler(GraphChangeListenerHandle);

    FAssetEditorToolkit::OnClose();
}

void FDialogueGraphEditor::OnGraphChanged(const FEdGraphEditAction& EditAction)
{
   UpdateWorkingAssetFromGraph();
}

FName FDialogueGraphEditor::GetToolkitFName() const
{
    return FName("DialogueGraphEditor");
}
FText FDialogueGraphEditor::GetBaseToolkitName() const
{
    return NSLOCTEXT("DialogueGraph", "App", "Dialogue Graph");
}
FString FDialogueGraphEditor::GetWorldCentricTabPrefix() const
{
    return TEXT("DialogueGraph ");
}
FLinearColor FDialogueGraphEditor::GetWorldCentricTabColorScale() const
{
    return FLinearColor::White;
}
