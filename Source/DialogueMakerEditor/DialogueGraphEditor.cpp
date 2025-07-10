#include "DialogueGraphEditor.h"

#include "DialogueEdEndGraphNode.h"
#include "DialogueEdGraphNode.h"
#include "DialogueEdGraphSchema.h"
#include "DialogueEdStartGraphNode.h"
#include "DialogueGraphEditorMode.h"
#include "EdGraph/EdGraph.h"
#include "Widgets/Docking/SDockTab.h"
#include "PropertyEditorModule.h"
#include "Widgets/SBoxPanel.h"
#include "GraphEditor.h"
#include "DialogueMaker/DialogueGraph.h"
#include "DialogueMaker/DialogueNodeInfo.h"
#include "Kismet2/BlueprintEditorUtils.h"

DEFINE_LOG_CATEGORY_STATIC(DialogueMakerEditorSub, Log, All);

void FDialogueGraphEditor::RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
    FAssetEditorToolkit::RegisterTabSpawners(InTabManager);
}

void FDialogueGraphEditor::InitEditor(const EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& InitToolkitHost, UDialogueGraph* InGraph)
{
    WorkingAsset = InGraph;
    WorkingAsset->SetPreSaveListener([this] () { OnWorkingAssetPreSave(); });

    WorkingGraph = FBlueprintEditorUtils::CreateNewGraph(WorkingAsset, NAME_None, UEdGraph::StaticClass(), UDialogueEdGraphSchema::StaticClass());
    
    
    InitAssetEditor(Mode, InitToolkitHost, FName("DialogueGraphEditor"), FTabManager::FLayout::NullLayout, true, true, InGraph);

    // Editor의 Mode를 DialogueGraphEditorMode로 설정
    AddApplicationMode(TEXT("DialogueGraphEditorMode"), MakeShareable(new DialogueGraphEditorMode(SharedThis(this))));
    SetCurrentMode(TEXT("DialogueGraphEditorMode"));

    UpdateEditorGraphFromWorkingAsset();
}

void FDialogueGraphEditor::SetWorkingGraphUI(TSharedPtr<SGraphEditor> NewWorkingGraphUI)
{
    WorkingGraphUI = NewWorkingGraphUI;
}

// 선택한 Node로 DetailView 정보 변경하기 위한 작업
void FDialogueGraphEditor::SetSelectedDetailView(TSharedPtr<IDetailsView> NewDetailsView)
{
    SelectedDetailView = NewDetailsView;
    SelectedDetailView->OnFinishedChangingProperties().AddRaw(this, &FDialogueGraphEditor::OnNodeDetailViewPropertiesUpdated);
}

void FDialogueGraphEditor::OnGraphSelectionChanged(const FGraphPanelSelectionSet& NewSelection)
{
    UDialogueEdGraphNodeBase* SelectedNode = GetSelectedNode(NewSelection);
    if (SelectedNode != nullptr)
    {
        SelectedDetailView->SetObject(SelectedNode->GetNodeInfo());
    }
    else
    {
        SelectedDetailView->SetObject(nullptr);
    }
}

// Graph UI로부터 Data Asset 저장
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
        
        UDialogueEdGraphNodeBase* EdGraphNode = Cast<UDialogueEdGraphNodeBase>(Node);
        RuntimeNode->NodeInfo = DuplicateObject(EdGraphNode->GetNodeInfo(), RuntimeNode);
        RuntimeNode->DialogueNodeType = EdGraphNode->GetDialogueNodeType();
        
        RuntimeGraph->Nodes.Add(RuntimeNode);
    }

    for (std::pair<FGuid, FGuid> Connection : Connections)
    {
        UDialogueRuntimePin* Pin1 = IdToPinMap[Connection.first];
        UDialogueRuntimePin* Pin2 = IdToPinMap[Connection.second];
        Pin1->Connections.Add(Pin2);
    }
}

// Asset Data로부터 Graph UI 시각화 
void FDialogueGraphEditor::UpdateEditorGraphFromWorkingAsset()
{
    if (WorkingAsset->Graph == nullptr)
    {
        WorkingGraph->GetSchema()->CreateDefaultNodesForGraph(*WorkingGraph.Get());
        return;
    }

    TArray<std::pair<FGuid, FGuid>> Connections;
    TMap<FGuid, UEdGraphPin*> IdToPinMap;

    for (UDialogueRuntimeNode* RuntimeNode : WorkingAsset->Graph->Nodes)
    {
        UDialogueEdGraphNodeBase* NewNode = nullptr;
        if (RuntimeNode->DialogueNodeType == EDialogueType::DialogueNode)
        {
            NewNode = NewObject<UDialogueEdGraphNode>(WorkingGraph);
        }
        else if (RuntimeNode->DialogueNodeType == EDialogueType::StartNode)
        {
            NewNode = NewObject<UDialogueEdStartGraphNode>(WorkingGraph);
        }
        else if (RuntimeNode->DialogueNodeType == EDialogueType::EndNode)
        {
            NewNode = NewObject<UDialogueEdEndGraphNode>(WorkingGraph);
        }
        else
        {
            UE_LOG(DialogueMakerEditorSub, Error, TEXT("FDialogueGraphEditor::UpdateEditorGraphFromWorkingAsset : Unknown Node Type"));
            continue;
        }
        
        NewNode->CreateNewGuid();
        NewNode->NodePosX = RuntimeNode->Position.X;
        NewNode->NodePosY = RuntimeNode->Position.Y;
        
        if (RuntimeNode->NodeInfo != nullptr)
        {
            NewNode->SetDialogueNodeInfo(DuplicateObject(RuntimeNode->NodeInfo, NewNode));
        }
        else
        {
            NewNode->InitNodeInfo(NewNode);
        }

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
        }

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

    // Node의 각 pin 연결 구현
    for (std::pair<FGuid, FGuid> Connection : Connections)
    {
        UEdGraphPin* FromPin = IdToPinMap[Connection.first];
        UEdGraphPin* ToPin = IdToPinMap[Connection.second];
        FromPin->LinkedTo.Add(ToPin);
        ToPin->LinkedTo.Add(FromPin);
    }
}

class UDialogueEdGraphNodeBase* FDialogueGraphEditor::GetSelectedNode(const FGraphPanelSelectionSet& Selection)
{
    // 첫번째 노드 정보 가져오기
    for (UObject* Object : Selection)
    {
        UDialogueEdGraphNodeBase* Node = Cast<UDialogueEdGraphNodeBase>(Object);
        if (Node != nullptr)
        {
            return Node;
        }
    }

    return nullptr;
}

void FDialogueGraphEditor::OnClose()
{
    UpdateWorkingAssetFromGraph();
    WorkingAsset->SetPreSaveListener(nullptr);

    FAssetEditorToolkit::OnClose();
}

// 선택한 Node 정보로 업데이트
void FDialogueGraphEditor::OnNodeDetailViewPropertiesUpdated(const FPropertyChangedEvent& Event)
{
    if (WorkingGraphUI != nullptr)
    {
        UDialogueEdGraphNodeBase* DialogueEdGraphNode = GetSelectedNode(WorkingGraphUI->GetSelectedNodes());
        if (DialogueEdGraphNode != nullptr)
        {
                DialogueEdGraphNode->OnPropertiesChanged();
        }
        
        WorkingGraphUI->NotifyGraphChanged();
    }
}

void FDialogueGraphEditor::OnWorkingAssetPreSave()
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
