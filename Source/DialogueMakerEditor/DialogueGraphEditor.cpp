#include "DialogueGraphEditor.h"

#include "AssetToolsModule.h"
#include "DialogueEdEndGraphNode.h"
#include "DialogueEdGraphNode.h"
#include "DialogueEdGraphSchema.h"
#include "DialogueEdStartGraphNode.h"
#include "DialogueGraphEditorCommands.h"
#include "DialogueGraphEditorMode.h"
#include "EditorStyleSet.h"
#include "EdGraph/EdGraph.h"
#include "Widgets/Docking/SDockTab.h"
#include "PropertyEditorModule.h"
#include "Widgets/SBoxPanel.h"
#include "GraphEditor.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "DialogueMaker/DialogueGraph.h"
#include "DialogueMaker/DialogueNodeInfo.h"
#include "Kismet2/BlueprintEditorUtils.h"

#define LOCTEXT_NAMESPACE "DialogueGraphEditor"
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

    // Toolbar Button 초기화
    GraphEditorCommands = MakeShareable(new FUICommandList);
    const FDialogueGraphEditorCommands& Commands = FDialogueGraphEditorCommands::Get();
    GraphEditorCommands->MapAction(
        Commands.ConvertToDataTable,
        FExecuteAction::CreateSP(this, &FDialogueGraphEditor::OnConvertToDataTableButtonClicked),
        FCanExecuteAction::CreateSP(this, &FDialogueGraphEditor::CanConvertToDataTable)
        );

    // Toolbar 생성
    TSharedPtr<FExtender> ToolbarExtender = MakeShareable(new FExtender());
    ToolbarExtender->AddToolBarExtension(
        "Asset",
        EExtensionHook::After,
        GraphEditorCommands,
        FToolBarExtensionDelegate::CreateSP(this, &FDialogueGraphEditor::FillToolbar)
        );
    
    AddToolbarExtender(ToolbarExtender);
    
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

// Toolbar에 Convert to DataTable을 위한 버튼 생성
void FDialogueGraphEditor::FillToolbar(FToolBarBuilder& ToolbarBuilder)
{
    ToolbarBuilder.BeginSection("DialogueFunctions");
    ToolbarBuilder.AddToolBarButton(
        FDialogueGraphEditorCommands::Get().ConvertToDataTable,
        NAME_None,
        LOCTEXT("ConvertToDataTable_Toolbar", "Convert to DataTable"),
        LOCTEXT("ConvertToDataTable_Toolbar_Tooltip", "Convert to DataTable"),
        FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.DataLayers")
        );
    ToolbarBuilder.EndSection();
}

bool FDialogueGraphEditor::CanConvertToDataTable() const
{
    return true;
}

// Convert to DataTable 로직
void FDialogueGraphEditor::OnConvertToDataTableButtonClicked()
{
    UE_LOG(LogTemp, Warning, TEXT("FDialogueGraphEditor::OnConvertToDataTableButtonClicked : Enter"));

    if (DataTable == nullptr)
    {
        CreateNewDataTable();
    }
    

}

void FDialogueGraphEditor::CreateNewDataTable()
{

    // Check Speaker Name
    // TODO WorkingAsset의 이름이 기본 이름("Enter Dialogue Name Here")인 경우 경고 팝업 띄우기

    // Creat DataTable Asset
    const FString DataTableRoot = TEXT("/Game/DialogueDataTable");
    // const FString FileName = TEXT("DT_Dialogue_") + WorkingAsset->GetSpeakerName();
    const FString FileName = FString("DT_Dialogue_") + FString("TEST");

    const FString AssetPath = DataTableRoot + "/" + FileName;
    UPackage* Package = CreatePackage(*AssetPath);
    DataTable = NewObject<UDataTable>(Package, UDataTable::StaticClass(), *FileName, RF_Public|RF_Standalone);
    DataTable->RowStruct = FDialogueStructure::StaticStruct();
    
    FAssetRegistryModule::AssetCreated(DataTable);
    Package->MarkPackageDirty();

    // Nodes를 순회하며 Node의 정보들을 DialogueStructure의 형태로 DataTable에 저장
    TMap<FGuid, FDialogueStructure> DialogueNodeDataMap;
    CollectDialogueData(DialogueNodeDataMap);
    int32 DataRowIndex = 0;
    for (TPair<FGuid, FDialogueStructure> Pair : DialogueNodeDataMap)
    {
        DataTable->AddRow(*Pair.Key.ToString(), Pair.Value);
        DataRowIndex++;
    }

    // Save Asset
    FString FilePath = FPackageName::LongPackageNameToFilename(AssetPath, FPackageName::GetAssetPackageExtension());
    UPackage::SavePackage(Package, DataTable, RF_Public|RF_Standalone, *FilePath);
}

/* Dialogue Nodes를 outpin을 기준으로 DFS 탐색하며
 * 대화 노드의 연결과 Dialogue Data를 추출한다.
 * 이때 DFS를 하는 이유는 DataTable에서 대화를 살필 때 편의성을 위한다.
*/
void FDialogueGraphEditor::CollectDialogueData(TMap<FGuid, FDialogueStructure>& OutDialogueDataMap)
{
    UDialogueEdGraphNodeBase* StartNode = FindStartNode();
    if (StartNode == nullptr)
    {
        return;
    }

    // Start Node에서부터 DFS 탐색하며 Dialogue 정리
    TSet<FGuid> VisitedNodeSet;
    DFSDialogueGraph(StartNode, OutDialogueDataMap, VisitedNodeSet);
}

UDialogueEdGraphNodeBase* FDialogueGraphEditor::FindStartNode() const
{
    for (auto EdGraphNode : WorkingGraph->Nodes)
    {
        if (UDialogueEdGraphNodeBase* DialogueEdGraphNodeBase = Cast<UDialogueEdGraphNodeBase>(EdGraphNode))
        {
            if (DialogueEdGraphNodeBase->GetDialogueNodeType() == EDialogueType::StartNode)
            {
                return DialogueEdGraphNodeBase;
            }
        }
    }

    return nullptr;
}

void FDialogueGraphEditor::DFSDialogueGraph(UDialogueEdGraphNodeBase* Node, TMap<FGuid, FDialogueStructure>& OutDialogueDataMap, TSet<FGuid>& VisitedSet)
{
    if (Node == nullptr)
    {
        UE_LOG(LogTemp, Error, TEXT("FDialogueGraphEditor::DFSDialogueGraph : Node is null"));
        return;
    }
    
    if (Node->GetDialogueNodeType() == EDialogueType::EndNode)
    {
        UE_LOG(LogTemp, Verbose, TEXT("FDialogueGraphEditor::DFSDialogueGraph : DialogueType is EndNode"));
        return;
    }

    FGuid NodeGuid = Node->NodeGuid;
    if (VisitedSet.Contains(NodeGuid))
    {
        UE_LOG(LogTemp, Verbose, TEXT("FDialogueGraphEditor::DFSDialogueGraph : Already visited - %s"), *NodeGuid.ToString());
        return;
    }

    VisitedSet.Add(NodeGuid);
    
    TArray<UEdGraphPin*> OutputPins;
    for (UEdGraphPin* Pin : Node->Pins)
    {
        if (Pin->Direction == EGPD_Output)
        {
            OutputPins.Add(Pin);
        }
    }

    if (Node->GetDialogueNodeType() == EDialogueType::StartNode)
    {
        if (OutputPins.Num() > 0)
        {
            if (OutputPins[0]->LinkedTo.Num() == 0)
            {
                UE_LOG(LogTemp, Error, TEXT("FDialogueGraphEditor::DFSDialogueGraph : Linking pin is nullptr - %s"), *OutputPins[0]->PinName.ToString());
                return;
            }
            
            UEdGraphPin* LinkedPin = OutputPins[0]->LinkedTo[0];
            UEdGraphNode* LinkedNode = LinkedPin->GetOwningNode();
            if (UDialogueEdGraphNode* DialogueNode = Cast<UDialogueEdGraphNode>(LinkedNode))
            {
                DFSDialogueGraph(DialogueNode, OutDialogueDataMap, VisitedSet);
                return;
            }
        }
    }

    UDialogueEdGraphNode* DialogueNode = Cast<UDialogueEdGraphNode>(Node);
    if (DialogueNode == nullptr)
    {
        UE_LOG(LogTemp, Error, TEXT(""));
        return;
    }
    
    // DialogueStruct 생성
    UDialogueNodeInfo* DialogueNodeInfo = DialogueNode->GetDialogueNodeInfo();
    FDialogueStructure DialogueStructure;
    
    DialogueStructure.CurrentDialogueId = NodeGuid;
    DialogueStructure.SpeakerName = FText::FromString(WorkingAsset->GetSpeakerName());
    DialogueStructure.DialogueText = DialogueNodeInfo->DialogueText;
    
    // 선택지 존재시 선택지 내용 입력
    bool bHasChoices = DialogueNodeInfo->DialogueResponses.Num() > 1;
    if (bHasChoices)
    {
        int32 ChoicesIndex = 0;
        for (UEdGraphPin* OutputPin : OutputPins)
        {
            if (ChoicesIndex >= DialogueNodeInfo->DialogueResponses.Num())
            {
                UE_LOG(LogTemp, Error, TEXT("FDialogueGraphEditor::DFSDialogueGraph : ChoicesIndex %d, DialogueResponses %d, %s"),
                    ChoicesIndex, DialogueNodeInfo->DialogueResponses.Num(), *OutputPin->PinName.ToString());
                return;
            }

            FDialogueChoice DialogueChoice;;
            DialogueChoice.ResponseText = DialogueNodeInfo->DialogueResponses[ChoicesIndex];

            if (OutputPin->LinkedTo.Num() > 0)
            {
                // outputPin에 연결된 pin은 1개로 가정한다.
                UEdGraphNode* LinkedNode = OutputPin->LinkedTo[0]->GetOwningNode();
                DialogueChoice.NextDialogueId = LinkedNode->NodeGuid;
                
                DialogueStructure.Choices.Add(DialogueChoice);
                ChoicesIndex++;
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("FDialogueGraphEditor::DFSDialogueGraph : Responses Output pin is not linking - %s-%s"), *NodeGuid.ToString(), *OutputPin->PinName.ToString());
                return;
            }
        }

        // DialogueStructure에 복수의 Response와 연결된 Node의 Guid 저장 후 Add
        OutDialogueDataMap.Add(NodeGuid, DialogueStructure);

        for (UEdGraphPin* OutputPin : OutputPins)
        {
            if (OutputPin->LinkedTo.Num() == 0)
            {
                UE_LOG(LogTemp, Error, TEXT("FDialogueGraphEditor::DFSDialogueGraph : Linking pin is nullptr - %s"), *OutputPin->PinName.ToString());
                return;
            }
            
            UEdGraphNode* LinkedNode = OutputPin->LinkedTo[0]->GetOwningNode();
            UDialogueEdGraphNodeBase* NextDialogueNode = Cast<UDialogueEdGraphNodeBase>(LinkedNode);
            DFSDialogueGraph(NextDialogueNode, OutDialogueDataMap, VisitedSet);
        }
    }
    else if (OutputPins.Num() == 1)
    {
        UEdGraphPin* OutputPin = OutputPins[0];
        if (OutputPin->LinkedTo.Num() > 0)
        {
            if (OutputPin->LinkedTo.Num() == 0)
            {
                UE_LOG(LogTemp, Error, TEXT("FDialogueGraphEditor::DFSDialogueGraph : Linking pin is nullptr - %s"), *OutputPin->PinName.ToString());
                return;
            }
            
            // outputPin에 연결된 pin은 1개로 가정한다.
            UEdGraphNode* LinkedNode = OutputPin->LinkedTo[0]->GetOwningNode();
            DialogueStructure.NextDialogueId = LinkedNode->NodeGuid;

            OutDialogueDataMap.Add(NodeGuid, DialogueStructure);
            UDialogueEdGraphNodeBase* NextDialogueNode = Cast<UDialogueEdGraphNodeBase>(LinkedNode);
            DFSDialogueGraph(NextDialogueNode, OutDialogueDataMap, VisitedSet);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("FDialogueGraphEditor::DFSDialogueGraph : Output pin is not linking - %s-%s"), *NodeGuid.ToString(), *OutputPin->PinName.ToString());
            return;
        }
    }
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
#undef LOCTEXT_NAMESPACE