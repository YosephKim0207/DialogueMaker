#include "DialogueGraphEditor.h"

#include "DesktopPlatformModule.h"
#include "DialogueBranchEdGraphNode.h"
#include "DialogueEdEndGraphNode.h"
#include "DialogueEdGraphNode.h"
#include "DialogueEdGraphSchema.h"
#include "DialogueEdStartGraphNode.h"
#include "DialogueGraphEditorCommands.h"
#include "DialogueGraphEditorMode.h"
#include "DialogueLocalizationDataAsset.h"
#include "AssetRegistry/AssetRegistryModule.h"
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
#include "Misc/FileHelper.h"
#include "Misc/MessageDialog.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Styling/AppStyle.h"
#include "Internationalization/Internationalization.h"
#include "Internationalization/Culture.h"
#include "UObject/SavePackage.h"

#define LOCTEXT_NAMESPACE "DialogueGraphEditor"

DEFINE_LOG_CATEGORY_STATIC(DialogueKitEditorSub, Log, All);

namespace
{
FString CSVEscape(const FString& Input)
{
    FString Escaped = Input;
    Escaped.ReplaceInline(TEXT("\""), TEXT("\"\""));

    if (Escaped.Contains(TEXT(",")) || Escaped.Contains(TEXT("\"")) || Escaped.Contains(TEXT("\n")) || Escaped.Contains(TEXT("\r")))
    {
        return FString::Printf(TEXT("\"%s\""), *Escaped);
    }

    return Escaped;
}

void AppendCSVRow(FString& OutCSV, const TArray<FString>& Columns)
{
    TArray<FString> EscapedColumns;
    EscapedColumns.Reserve(Columns.Num());
    for (const FString& Column : Columns)
    {
        EscapedColumns.Add(CSVEscape(Column));
    }

    OutCSV += FString::Join(EscapedColumns, TEXT(","));
    OutCSV += LINE_TERMINATOR;
}

FString GuidToString(const FGuid& Guid)
{
    return Guid.IsValid() ? Guid.ToString(EGuidFormats::DigitsWithHyphensLower) : TEXT("");
}

FString NormalizeCSVToken(const FString& Input)
{
    return Input.TrimStartAndEnd().ToLower();
}

FString GetCSVValueAt(const TArray<FString>& Row, const int32 Index)
{
    return Row.IsValidIndex(Index) ? Row[Index] : TEXT("");
}

bool ParseCSVRows(const FString& CSVContent, TArray<TArray<FString>>& OutRows)
{
    OutRows.Reset();

    TArray<FString> CurrentRow;
    FString CurrentCell;
    bool bInQuotes = false;

    for (int32 Index = 0; Index < CSVContent.Len(); ++Index)
    {
        const TCHAR Character = CSVContent[Index];
        if (Character == TEXT('"'))
        {
            if (bInQuotes && (Index + 1) < CSVContent.Len() && CSVContent[Index + 1] == TEXT('"'))
            {
                CurrentCell.AppendChar(TEXT('"'));
                ++Index;
            }
            else
            {
                bInQuotes = !bInQuotes;
            }
            continue;
        }

        if (!bInQuotes && Character == TEXT(','))
        {
            CurrentRow.Add(CurrentCell);
            CurrentCell.Reset();
            continue;
        }

        if (!bInQuotes && (Character == TEXT('\n') || Character == TEXT('\r')))
        {
            CurrentRow.Add(CurrentCell);
            CurrentCell.Reset();
            OutRows.Add(CurrentRow);
            CurrentRow.Reset();

            if (Character == TEXT('\r') && (Index + 1) < CSVContent.Len() && CSVContent[Index + 1] == TEXT('\n'))
            {
                ++Index;
            }
            continue;
        }

        CurrentCell.AppendChar(Character);
    }

    if (!CurrentCell.IsEmpty() || CurrentRow.Num() > 0)
    {
        CurrentRow.Add(CurrentCell);
        OutRows.Add(CurrentRow);
    }

    if (OutRows.Num() > 0 && OutRows[0].Num() > 0)
    {
        OutRows[0][0].RemoveFromStart(TEXT("\xFEFF"));
    }

    return OutRows.Num() > 0;
}

bool TryParsePrimaryAssetIdString(const FString& InPrimaryAssetId, FPrimaryAssetId& OutPrimaryAssetId)
{
    FString PrimaryAssetTypeString;
    FString PrimaryAssetNameString;
    if (!InPrimaryAssetId.Split(TEXT(":"), &PrimaryAssetTypeString, &PrimaryAssetNameString))
    {
        return false;
    }

    if (PrimaryAssetTypeString.IsEmpty() || PrimaryAssetNameString.IsEmpty())
    {
        return false;
    }

    OutPrimaryAssetId = FPrimaryAssetId(FName(*PrimaryAssetTypeString), FName(*PrimaryAssetNameString));
    return true;
}

FString SanitizeForObjectName(const FString& Input)
{
    FString Result;
    Result.Reserve(Input.Len());
    for (const TCHAR Character : Input)
    {
        if (FChar::IsAlnum(Character) || Character == TEXT('_'))
        {
            Result.AppendChar(Character);
        }
        else
        {
            Result.AppendChar(TEXT('_'));
        }
    }

    if (Result.IsEmpty())
    {
        return TEXT("Unknown");
    }

    return Result;
}

FString ResolveCultureNameFromCSVPath(const FString& CSVFilePath)
{
    const FString ParentDirectory = FPaths::GetPath(CSVFilePath);
    const FString CultureCandidate = FPaths::GetCleanFilename(ParentDirectory).TrimStartAndEnd();
    if (!CultureCandidate.IsEmpty())
    {
        return CultureCandidate;
    }

    if (const FCulturePtr CurrentCulture = FInternationalization::Get().GetCurrentCulture())
    {
        return CurrentCulture->GetName();
    }

    return TEXT("Invariant");
}
}

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

void FDialogueGraphEditor::OnConvertToCSVButtonClicked()
{
    UE_LOG(DialogueKitEditorSub, Warning, TEXT("FDialogueGraphEditor::OnConvertToCSVButtonClicked : Enter"));

    if (CanConvertCSV() == false)
    {
        FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("ConvertCSV_InvalidEditorState", "Cannot export CSV because editor state is invalid."));
        return;
    }

    // 에디터에서 편집 중인 내용을 먼저 런타임 그래프로 동기화한다.
    UpdateWorkingAssetFromGraph();

    const FString CSVFilePath = OpenCSVSaveWindow();
    if (CSVFilePath.IsEmpty())
    {
        return;
    }

    // Dialogue Graph를 CSV로 변환
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

bool FDialogueGraphEditor::ConvertCSVToDialogueLocalizationDataAsset(const FString& CSVFilePath)
{
    if (WorkingAsset == nullptr)
    {
        return false;
    }

    FString CSVContent;
    if (!FFileHelper::LoadFileToString(CSVContent, *CSVFilePath))
    {
        FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("ConvertCSVToDialogueLocalization_LoadFail", "Failed to read CSV file."));
        return false;
    }

    TArray<TArray<FString>> Rows;
    if (!ParseCSVRows(CSVContent, Rows) || Rows.Num() < 2)
    {
        FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("ConvertCSVToDialogueLocalization_InvalidCSV", "Invalid CSV format."));
        return false;
    }

    const TArray<FString>& Header = Rows[0];
    int32 NodeGuidColumn = INDEX_NONE;
    int32 PinIdColumn = INDEX_NONE;
    int32 KeyColumn = INDEX_NONE;
    int32 ValueColumn = INDEX_NONE;

    for (int32 HeaderIndex = 0; HeaderIndex < Header.Num(); ++HeaderIndex)
    {
        const FString NormalizedHeader = NormalizeCSVToken(Header[HeaderIndex]);
        if (NormalizedHeader == TEXT("nodeguid"))
        {
            NodeGuidColumn = HeaderIndex;
        }
        else if (NormalizedHeader == TEXT("pinid"))
        {
            PinIdColumn = HeaderIndex;
        }
        else if (NormalizedHeader == TEXT("key"))
        {
            KeyColumn = HeaderIndex;
        }
        else if (NormalizedHeader == TEXT("value"))
        {
            ValueColumn = HeaderIndex;
        }
    }

    if (NodeGuidColumn == INDEX_NONE || PinIdColumn == INDEX_NONE || KeyColumn == INDEX_NONE || ValueColumn == INDEX_NONE)
    {
        FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("ConvertCSVToDialogueLocalization_InvalidHeader", "Invalid CSV header. Required columns: NodeGuid, PinId, Key, Value"));
        return false;
    }

    FPrimaryAssetId TargetDialoguePrimaryAssetId;
    bool bPrimaryAssetIdFound = false;
    TArray<FDialogueLocalizationEntry> Entries;
    Entries.Reserve(Rows.Num() - 1);

    for (int32 RowIndex = 1; RowIndex < Rows.Num(); ++RowIndex)
    {
        const TArray<FString>& Row = Rows[RowIndex];
        const FString Key = NormalizeCSVToken(GetCSVValueAt(Row, KeyColumn));
        const FString Value = GetCSVValueAt(Row, ValueColumn);

        if (Key == TEXT("primaryassetid"))
        {
            FPrimaryAssetId ParsedPrimaryAssetId;
            if (!TryParsePrimaryAssetIdString(Value.TrimStartAndEnd(), ParsedPrimaryAssetId))
            {
                FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("ConvertCSVToDialogueLocalization_InvalidPrimaryAssetId", "PrimaryAssetId parse failed. Please check CSV file."));
                return false;
            }

            TargetDialoguePrimaryAssetId = ParsedPrimaryAssetId;
            bPrimaryAssetIdFound = true;
            continue;
        }

        if (Key != TEXT("dialoguetext") && Key != TEXT("responsetext"))
        {
            continue;
        }

        FGuid ParsedNodeGuid;
        if (!FGuid::Parse(GetCSVValueAt(Row, NodeGuidColumn).TrimStartAndEnd(), ParsedNodeGuid))
        {
            FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("ConvertCSVToDialogueLocalization_NodeGuidParseFail", "NodeGuid parse failed. Please check CSV file."));
            return false;
        }

        FDialogueLocalizationEntry NewEntry;
        NewEntry.NodeGuid = ParsedNodeGuid;
        NewEntry.Key = (Key == TEXT("dialoguetext")) ? FName(TEXT("DialogueText")) : FName(TEXT("ResponseText"));
        NewEntry.Value = FText::FromString(Value);

        if (Key == TEXT("responsetext"))
        {
            if (!FGuid::Parse(GetCSVValueAt(Row, PinIdColumn).TrimStartAndEnd(), NewEntry.PinId))
            {
                FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("ConvertCSVToDialogueLocalization_PinIdParseFail", "PinId parse failed. Please check CSV file."));
                return false;
            }
        }

        Entries.Add(MoveTemp(NewEntry));
    }

    if (!bPrimaryAssetIdFound)
    {
        FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("ConvertCSVToDialogueLocalization_MissingPrimaryAssetId", "PrimaryAssetId is missing in CSV file."));
        return false;
    }

    if (TargetDialoguePrimaryAssetId != WorkingAsset->GetPrimaryAssetId())
    {
        FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("ConvertCSVToDialogueLocalization_AssetMismatch", "PrimaryAssetId in CSV does not match current DialogueGraph asset."));
        return false;
    }

    const FString CultureName = ResolveCultureNameFromCSVPath(CSVFilePath);
    const FString CulturePackageSegment = SanitizeForObjectName(CultureName);
    const FString DialogueGraphAssetName = SanitizeForObjectName(WorkingAsset->GetName());
    const FString LocalizationAssetName = FString::Printf(TEXT("DL_%s_%s"), *DialogueGraphAssetName, *CulturePackageSegment);
    const FString LocalizationFolder = FString::Printf(TEXT("/Game/DialogueLocalization/%s"), *CulturePackageSegment);
    const FString LocalizationPackageName = LocalizationFolder + TEXT("/") + LocalizationAssetName;

    UPackage* Package = CreatePackage(*LocalizationPackageName);
    if (Package == nullptr)
    {
        FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("ConvertCSVToDialogueLocalization_PackageCreateFail", "Failed to create package for DialogueLocalization DataAsset."));
        return false;
    }

    UDialogueLocalizationDataAsset* LocalizationAsset = LoadObject<UDialogueLocalizationDataAsset>(nullptr, *(LocalizationPackageName + TEXT(".") + LocalizationAssetName));
    bool bCreated = false;
    if (LocalizationAsset == nullptr)
    {
        LocalizationAsset = NewObject<UDialogueLocalizationDataAsset>(Package, *LocalizationAssetName, RF_Public | RF_Standalone | RF_Transactional);
        if (LocalizationAsset == nullptr)
        {
            FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("ConvertCSVToDialogueLocalization_CreateFail", "Failed to create DialogueLocalization DataAsset."));
            return false;
        }
        bCreated = true;
    }

    LocalizationAsset->Modify();
    LocalizationAsset->TargetDialoguePrimaryAssetId = TargetDialoguePrimaryAssetId;
    LocalizationAsset->CultureName = CultureName;
    LocalizationAsset->Entries = MoveTemp(Entries);
    LocalizationAsset->MarkPackageDirty();
    Package->MarkPackageDirty();

    if (bCreated)
    {
        FAssetRegistryModule::AssetCreated(LocalizationAsset);
    }

    const FString PackageFilePath = FPackageName::LongPackageNameToFilename(LocalizationPackageName, FPackageName::GetAssetPackageExtension());
    FSavePackageArgs SaveArgs;
    SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
    SaveArgs.SaveFlags = SAVE_NoError;
    if (!UPackage::SavePackage(Package, LocalizationAsset, *PackageFilePath, SaveArgs))
    {
        FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("ConvertCSVToDialogueLocalization_SaveFail", "Failed to save DialogueLocalization DataAsset package."));
        return false;
    }

    return true;
}

bool FDialogueGraphEditor::ExportDialogueGraphToCSV(const FString& CSVFilePath) const
{
    const FString CSVContent = BuildDialogueGraphCSV();
    if (CSVContent.IsEmpty())
    {
        return false;
    }

    const FString DirectoryPath = FPaths::GetPath(CSVFilePath);
    if (!DirectoryPath.IsEmpty())
    {
        IFileManager::Get().MakeDirectory(*DirectoryPath, true);
    }

    return FFileHelper::SaveStringToFile(CSVContent, *CSVFilePath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
}

FString FDialogueGraphEditor::BuildDialogueGraphCSV() const
{
    if (WorkingAsset == nullptr)
    {
        return TEXT("");
    }

    FString OutCSV;
    AppendCSVRow(OutCSV, {TEXT("NodeGuid"), TEXT("PinId"), TEXT("Key"), TEXT("Value")});

    const auto AddRecord = [&OutCSV](const FString& NodeGuid, const FString& PinId, const FString& Key, const FString& Value)
    {
        AppendCSVRow(OutCSV, {NodeGuid, PinId, Key, Value});
    };

    AddRecord(TEXT(""), TEXT(""), TEXT("PrimaryAssetId"), WorkingAsset->GetPrimaryAssetId().ToString());
    
    if (WorkingAsset->Graph == nullptr)
    {
        return OutCSV;
    }

    for (const UDialogueRuntimeNode* RuntimeNode : WorkingAsset->Graph->Nodes)
    {
        if (RuntimeNode == nullptr)
        {
            continue;
        }

        const UDialogueNodeInfo* DialogueNodeInfo = Cast<UDialogueNodeInfo>(RuntimeNode->NodeInfo);
        if (DialogueNodeInfo == nullptr)
        {
            continue;
        }

        const FString NodeGuid = GuidToString(RuntimeNode->NodeGuid);
        AddRecord(NodeGuid, TEXT(""), TEXT("DialogueText"), DialogueNodeInfo->GetDialogueText().ToString());

        const TArray<FDialogueChoice>& Choices = DialogueNodeInfo->GetDialogueChoices();
        for (int32 ChoiceIndex = 0; ChoiceIndex < Choices.Num(); ++ChoiceIndex)
        {
            const FDialogueChoice& Choice = Choices[ChoiceIndex];
            const UDialogueRuntimePin* ChoicePin = RuntimeNode->OutputPins.IsValidIndex(ChoiceIndex) ? RuntimeNode->OutputPins[ChoiceIndex] : nullptr;
            const FString PinId = ChoicePin ? GuidToString(ChoicePin->PinId) : TEXT("");
            AddRecord(NodeGuid, PinId, TEXT("ResponseText"), Choice.ResponseText.ToString());
        }
    }

    return OutCSV;
}

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
