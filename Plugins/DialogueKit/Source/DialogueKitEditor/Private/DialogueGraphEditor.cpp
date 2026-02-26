#include "DialogueGraphEditor.h"

#include "DesktopPlatformModule.h"
#include "DialogueBranchEdGraphNode.h"
#include "DialogueEdEndGraphNode.h"
#include "DialogueEdGraphNode.h"
#include "DialogueEdGraphSchema.h"
#include "DialogueEdStartGraphNode.h"
#include "DialogueGraphEditorCommands.h"
#include "DialogueGraphEditorMode.h"
#include "DialogueLocalizationCSVConverter.h"
#include "EdGraph/EdGraph.h"
#include "GraphEditor.h"
#include "IDesktopPlatform.h"
#include "PropertyEditorModule.h"
#include "DialogueRuntimeGraph.h"
#include "DialogueGraph.h"
#include "DialogueNodeInfo.h"
#include "Framework/Application/SlateApplication.h"
#include "HAL/FileManager.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Misc/MessageDialog.h"
#include "Misc/Paths.h"
#include "Styling/AppStyle.h"
#include "Internationalization/Internationalization.h"
#include "Internationalization/Culture.h"

#define LOCTEXT_NAMESPACE "DialogueGraphEditor"

DEFINE_LOG_CATEGORY_STATIC(DialogueKitEditorSub, Log, All);

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
        Commands.ConvertToCSV,
        FExecuteAction::CreateSP(this, &FDialogueGraphEditor::OnConvertToCSVButtonClicked),
        FCanExecuteAction::CreateSP(this, &FDialogueGraphEditor::CanConvertCSV)
        );
    GraphEditorCommands->MapAction(
        Commands.ConvertCSVToDialogueLocalization,
        FExecuteAction::CreateSP(this, &FDialogueGraphEditor::OnConvertCSVToDialogueLocalizationButtonClicked),
        FCanExecuteAction::CreateSP(this, &FDialogueGraphEditor::CanConvertCSV)
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

// 선택된 Node를 편집할 경우 Properties를 갱신하기 위한 작업
void FDialogueGraphEditor::SetSelectedDetailView(TSharedPtr<IDetailsView> NewDetailsView)
{
    SelectedDetailView = NewDetailsView;
    SelectedDetailView->OnFinishedChangingProperties().AddRaw(this, &FDialogueGraphEditor::OnNodeDetailViewPropertiesUpdated);
}

// Graph Editor에서 Node 선택시 Properties에 노출시킬 정보 갱신
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
        if (Node->NodeGuid.IsValid())
        {
            RuntimeNode->NodeGuid = Node->NodeGuid;
        }
        else
        {
            Node->CreateNewGuid();
            RuntimeNode->NodeGuid = Node->NodeGuid;
            UE_LOG(DialogueKitEditorSub, Warning, TEXT("FDialogueGraphEditor::UpdateWorkingAssetFromGraph : Regenerate invalid NodeGuid"));
        }

        for (UEdGraphPin* Pin : Node->Pins)
        {
            UDialogueRuntimePin* RuntimePin = NewObject<UDialogueRuntimePin>(RuntimeNode);
            RuntimePin->PinName = Pin->PinName;
            RuntimePin->PinId = Pin->PinId;
            RuntimePin->OwnerNodeGuid = RuntimeNode->NodeGuid;
            
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
        UDialogueRuntimePin* PinFrom = IdToPinMap[Connection.first];
        UDialogueRuntimePin* PinTo = IdToPinMap[Connection.second];
        PinFrom->Connections.Add(PinTo);
        PinFrom->LinkedToNodeGuid = PinTo->OwnerNodeGuid;
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
        else if (RuntimeNode->DialogueNodeType == EDialogueType::BranchNode)
        {
            NewNode = NewObject<UDialogueBranchEdGraphNode>(WorkingGraph);
        }
        else
        {
            UE_LOG(DialogueKitEditorSub, Error, TEXT("FDialogueGraphEditor::UpdateEditorGraphFromWorkingAsset : Unknown Node Type"));
            continue;
        }
        
        if (RuntimeNode->NodeGuid.IsValid())
        {
            NewNode->NodeGuid = RuntimeNode->NodeGuid;
        }
        else
        {
            NewNode->CreateNewGuid();
            RuntimeNode->NodeGuid = NewNode->NodeGuid;
            UE_LOG(DialogueKitEditorSub, Warning, TEXT("FDialogueGraphEditor::UpdateEditorGraphFromWorkingAsset : RuntimeNodeGuid is invalid. Generated new Guid"));
        }
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

// 에디터의 Properties 상에서 편집된 NodeInfo로 업데이트
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

// Toolbar에 Convert to CSV를 위한 버튼 생성
void FDialogueGraphEditor::FillToolbar(FToolBarBuilder& ToolbarBuilder)
{
    ToolbarBuilder.BeginSection("DialogueFunctions");
    ToolbarBuilder.AddToolBarButton(
        FDialogueGraphEditorCommands::Get().ConvertToCSV,
        NAME_None,
        LOCTEXT("ConvertToCSV_Toolbar", "Convert to CSV"),
        LOCTEXT("ConvertToCSV_Toolbar_Tooltip", "Export DialogueGraph to CSV"),
        FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.Tabs.DataLayers")
        );
    ToolbarBuilder.AddToolBarButton(
        FDialogueGraphEditorCommands::Get().ConvertCSVToDialogueLocalization,
        NAME_None,
        LOCTEXT("ConvertCSVToDialogueLocalization_Toolbar", "CSV to DialogueLocalization"),
        LOCTEXT("ConvertCSVToDialogueLocalization_Tooltip", "Convert CSV to DialogueLocalization DataAsset"),
        FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Import")
        );
    ToolbarBuilder.EndSection();
}

bool FDialogueGraphEditor::CanConvertCSV() const
{
    return WorkingAsset != nullptr && WorkingGraph != nullptr;
}

// 툴바의 Convert to CSV 버튼 클릭 시 현재 그래프를 CSV 파일로 내보낸다.
void FDialogueGraphEditor::OnConvertToCSVButtonClicked()
{
    UE_LOG(DialogueKitEditorSub, Warning, TEXT("FDialogueGraphEditor::OnConvertToCSVButtonClicked : Enter"));

    if (CanConvertCSV() == false)
    {
        FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("ConvertCSV_InvalidEditorState", "Cannot export CSV because editor state is invalid."));
        return;
    }

    // 내보내기 전에 편집 중 그래프 상태를 런타임 데이터(WorkingAsset->Graph)에 먼저 반영한다.
    UpdateWorkingAssetFromGraph();

    const FString CSVFilePath = OpenCSVSaveWindow();
    if (CSVFilePath.IsEmpty())
    {
        return;
    }

    // DialogueGraph를 CSV 문자열로 직렬화 후 파일로 저장한다.
    if (ExportDialogueGraphToCSV(CSVFilePath))
    {
        FMessageDialog::Open(
            EAppMsgType::Ok,
            FText::Format(LOCTEXT("ConvertCSV_Success", "DialogueGraph exported to CSV.\n{0}"), FText::FromString(CSVFilePath)));
        return;
    }

    FMessageDialog::Open(
        EAppMsgType::Ok,
        FText::Format(LOCTEXT("ConvertCSV_Fail", "Failed to export DialogueGraph to CSV.\n{0}"), FText::FromString(CSVFilePath)));
}

// 툴바의 CSV to DialogueLocalization 버튼 클릭 시 CSV를 DataAsset으로 변환한다.
void FDialogueGraphEditor::OnConvertCSVToDialogueLocalizationButtonClicked()
{
    if (CanConvertCSV() == false)
    {
        FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("ConvertCSVToDialogueLocalization_InvalidState", "Cannot convert CSV because editor state is invalid."));
        return;
    }

    const FString CSVFilePath = OpenCSVLoadWindow();
    if (CSVFilePath.IsEmpty())
    {
        return;
    }

    if (!ConvertCSVToDialogueLocalizationDataAsset(CSVFilePath))
    {
        return;
    }

    FMessageDialog::Open(
        EAppMsgType::Ok,
        FText::Format(LOCTEXT("ConvertCSVToDialogueLocalization_Success", "DialogueLocalization DataAsset created/updated from CSV.\n{0}"), FText::FromString(CSVFilePath)));
}

// CSV 파일을 읽어 DialogueLocalization DataAsset을 생성/갱신한다.
bool FDialogueGraphEditor::ConvertCSVToDialogueLocalizationDataAsset(const FString& CSVFilePath)
{
    if (WorkingAsset == nullptr)
    {
        return false;
    }

    FDialogueLocalizationCSVConvertOptions ConvertOptions;
    ConvertOptions.bValidateExpectedPrimaryAssetId = true;
    ConvertOptions.ExpectedPrimaryAssetId = WorkingAsset->GetPrimaryAssetId();
    ConvertOptions.DestinationRootPackagePath = TEXT("/Game/DialogueLocalization");
    ConvertOptions.bUseUnderscoreCultureFolder = true;

    FString ErrorMessage;
    if (FDialogueLocalizationCSVConverter::ConvertCSVToDialogueLocalizationDataAsset(CSVFilePath, ConvertOptions, &ErrorMessage, nullptr))
    {
        return true;
    }

    FMessageDialog::Open(
        EAppMsgType::Ok,
        ErrorMessage.IsEmpty()
            ? LOCTEXT("ConvertCSVToDialogueLocalization_UnknownFail", "Failed to convert CSV to DialogueLocalization DataAsset.")
            : FText::FromString(ErrorMessage));
    return false;
}

// 지정한 DialogueGraph 에셋을 CSV 파일로 저장한다.
bool FDialogueGraphEditor::ExportDialogueGraphAssetToCSV(const UDialogueGraph* InDialogueGraph, const FString& CSVFilePath)
{
    return FDialogueLocalizationCSVConverter::ExportDialogueGraphAssetToCSV(InDialogueGraph, CSVFilePath, nullptr);
}

// 현재 에디터에서 작업 중인 DialogueGraph를 CSV 파일로 저장한다.
bool FDialogueGraphEditor::ExportDialogueGraphToCSV(const FString& CSVFilePath) const
{
    return ExportDialogueGraphAssetToCSV(WorkingAsset, CSVFilePath);
}

// 지정한 DialogueGraph의 번역 대상 데이터(PrimaryAssetId/DialogueText/ResponseText)를 CSV 문자열로 구성한다.
FString FDialogueGraphEditor::BuildDialogueGraphCSVFromAsset(const UDialogueGraph* InDialogueGraph)
{
    return FDialogueLocalizationCSVConverter::BuildDialogueGraphCSVFromAsset(InDialogueGraph);
}

// 현재 에디터에서 작업 중인 DialogueGraph의 CSV 문자열을 반환한다.
FString FDialogueGraphEditor::BuildDialogueGraphCSV() const
{
    return BuildDialogueGraphCSVFromAsset(WorkingAsset);
}

// CSV 파일 선택 창을 띄우고 사용자가 선택한 경로를 반환한다.
FString FDialogueGraphEditor::OpenCSVLoadWindow() const
{
    FString LanguageName = TEXT("Invariant");
    if (const FCulturePtr CurrentLanguage = FInternationalization::Get().GetCurrentLanguage())
    {
        LanguageName = CurrentLanguage->GetName();
    }

    const FString DefaultDirectory = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("DialogueCSV"), LanguageName);
    IFileManager::Get().MakeDirectory(*DefaultDirectory, true);

    IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
    if (DesktopPlatform == nullptr)
    {
        return TEXT("");
    }

    TArray<FString> OutFilePaths;
    const void* ParentWindowHandle = FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr);
    const bool bSelected = DesktopPlatform->OpenFileDialog(
        ParentWindowHandle,
        TEXT("Load Dialogue CSV"),
        DefaultDirectory,
        TEXT(""),
        TEXT("CSV file (*.csv)|*.csv"),
        EFileDialogFlags::None,
        OutFilePaths);

    if (!bSelected || OutFilePaths.Num() == 0)
    {
        return TEXT("");
    }

    return OutFilePaths[0];
}

// CSV 저장 창을 띄우고 사용자가 지정한 저장 경로를 반환한다.
FString FDialogueGraphEditor::OpenCSVSaveWindow() const
{
    if (WorkingAsset == nullptr)
    {
        return TEXT("");
    }

    FString LanguageName = TEXT("Invariant");
    if (const FCulturePtr CurrentLanguage = FInternationalization::Get().GetCurrentLanguage())
    {
        LanguageName = CurrentLanguage->GetName();
    }

    const FString DefaultDirectory = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("DialogueCSV"), LanguageName);
    IFileManager::Get().MakeDirectory(*DefaultDirectory, true);

    const FString DefaultFileName = FString::Printf(TEXT("%s.csv"), *WorkingAsset->GetName());
    IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
    if (DesktopPlatform == nullptr)
    {
        return FPaths::Combine(DefaultDirectory, DefaultFileName);
    }

    TArray<FString> OutFilePaths;
    const void* ParentWindowHandle = FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr);
    const bool bSelected = DesktopPlatform->SaveFileDialog(
        ParentWindowHandle,
        TEXT("Export DialogueGraph to CSV"),
        DefaultDirectory,
        DefaultFileName,
        TEXT("CSV file (*.csv)|*.csv"),
        EFileDialogFlags::None,
        OutFilePaths);

    if (!bSelected || OutFilePaths.Num() == 0)
    {
        return TEXT("");
    }

    FString CSVFilePath = OutFilePaths[0];
    if (FPaths::GetExtension(CSVFilePath).IsEmpty())
    {
        CSVFilePath += TEXT(".csv");
    }

    return CSVFilePath;
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
