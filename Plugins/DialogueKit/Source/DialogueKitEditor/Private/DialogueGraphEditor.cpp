#include "DialogueGraphEditor.h"

#include "AssetToolsModule.h"
#include "DesktopPlatformModule.h"
#include "DialogueBranchEdGraphNode.h"
#include "DialogueBranchNodeInfoBase.h"
#include "DialogueEdEndGraphNode.h"
#include "DialogueEdGraphNode.h"
#include "DialogueEdGraphSchema.h"
#include "DialogueEdStartGraphNode.h"
#include "DialogueEndNodeInfo.h"
#include "DialogueGraphEditorCommands.h"
#include "DialogueGraphEditorMode.h"
#include "EdGraph/EdGraph.h"
#include "Engine/DataTable.h"
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
#include "Misc/Paths.h"
#include "Styling/AppStyle.h"
#include "UObject/UnrealType.h"

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

template <typename TEnum>
FString EnumToString(const TEnum EnumValue)
{
    if (const UEnum* Enum = StaticEnum<TEnum>())
    {
        return Enum->GetNameStringByValue(static_cast<int64>(EnumValue));
    }

    return LexToString(static_cast<int64>(EnumValue));
}

FString GuidToString(const FGuid& Guid)
{
    return Guid.IsValid() ? Guid.ToString(EGuidFormats::DigitsWithHyphensLower) : TEXT("");
}

FString ExportTagQuery(const FGameplayTagQuery& TagQuery)
{
    FString Exported;
    FGameplayTagQuery::StaticStruct()->ExportText(Exported, &TagQuery, nullptr, nullptr, PPF_None, nullptr);
    return Exported;
}

FString ExportPrivatePropertyText(const UObject* SourceObject, const FName PropertyName)
{
    if (SourceObject == nullptr)
    {
        return TEXT("");
    }

    const FProperty* Property = SourceObject->GetClass()->FindPropertyByName(PropertyName);
    if (Property == nullptr)
    {
        return TEXT("");
    }

    UObject* MutableSourceObject = const_cast<UObject*>(SourceObject);
    const void* ValuePtr = Property->ContainerPtrToValuePtr<void>(MutableSourceObject);
    FString Exported;
    Property->ExportTextItem_Direct(Exported, ValuePtr, nullptr, MutableSourceObject, PPF_None);
    return Exported;
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
    AppendCSVRow(OutCSV, {
        TEXT("RecordType"),
        TEXT("GraphAssetPath"),
        TEXT("GraphName"),
        TEXT("NodeGuid"),
        TEXT("NodeType"),
        TEXT("PinId"),
        TEXT("LinkedNodeGuid"),
        TEXT("Index"),
        TEXT("Key"),
        TEXT("Value")
    });

    const FString GraphAssetPath = WorkingAsset->GetPathName();
    const FString GraphName = WorkingAsset->GetName();
    const auto AddRecord = [&OutCSV, &GraphAssetPath, &GraphName](
        const FString& RecordType,
        const FString& NodeGuid,
        const FString& NodeType,
        const FString& PinId,
        const FString& LinkedNodeGuid,
        const FString& Index,
        const FString& Key,
        const FString& Value)
    {
        AppendCSVRow(OutCSV, {
            RecordType,
            GraphAssetPath,
            GraphName,
            NodeGuid,
            NodeType,
            PinId,
            LinkedNodeGuid,
            Index,
            Key,
            Value
        });
    };

    AddRecord(TEXT("Graph"), TEXT(""), TEXT(""), TEXT(""), TEXT(""), TEXT(""), TEXT("PrimaryAssetId"), WorkingAsset->GetPrimaryAssetId().ToString());
    AddRecord(TEXT("Graph"), TEXT(""), TEXT(""), TEXT(""), TEXT(""), TEXT(""), TEXT("SpeakerID"), EnumToString(WorkingAsset->SpeakerID));
    AddRecord(TEXT("Graph"), TEXT(""), TEXT(""), TEXT(""), TEXT(""), TEXT(""), TEXT("ChapterID"), EnumToString(WorkingAsset->ChapterID));
    AddRecord(TEXT("Graph"), TEXT(""), TEXT(""), TEXT(""), TEXT(""), TEXT(""), TEXT("DialogueGraphType"), EnumToString(WorkingAsset->DialogueGraphType));
    AddRecord(TEXT("Graph"), TEXT(""), TEXT(""), TEXT(""), TEXT(""), TEXT(""), TEXT("DialoguePriorityWeight"), LexToString(WorkingAsset->DialoguePriorityWeight));
    AddRecord(TEXT("Graph"), TEXT(""), TEXT(""), TEXT(""), TEXT(""), TEXT(""), TEXT("RequiredAllTags"), WorkingAsset->RequiredAllTags.ToString());
    AddRecord(TEXT("Graph"), TEXT(""), TEXT(""), TEXT(""), TEXT(""), TEXT(""), TEXT("RequiredAnyTags"), WorkingAsset->RequiredAnyTags.ToString());
    AddRecord(TEXT("Graph"), TEXT(""), TEXT(""), TEXT(""), TEXT(""), TEXT(""), TEXT("BlockedAnyTags"), WorkingAsset->BlockedAnyTags.ToString());

    for (int32 PortraitIndex = 0; PortraitIndex < WorkingAsset->InitPortraits.Num(); ++PortraitIndex)
    {
        const FPortraitInitData& InitPortrait = WorkingAsset->InitPortraits[PortraitIndex];
        AddRecord(TEXT("InitPortrait"), TEXT(""), TEXT(""), TEXT(""), TEXT(""), LexToString(PortraitIndex), TEXT("SpeakerID"), EnumToString(InitPortrait.Speaker));
        AddRecord(TEXT("InitPortrait"), TEXT(""), TEXT(""), TEXT(""), TEXT(""), LexToString(PortraitIndex), TEXT("EmoteType"), EnumToString(InitPortrait.EmoteType));
        AddRecord(TEXT("InitPortrait"), TEXT(""), TEXT(""), TEXT(""), TEXT(""), LexToString(PortraitIndex), TEXT("PortraitSide"), EnumToString(InitPortrait.PortraitSide));
    }

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

        const FString NodeGuid = GuidToString(RuntimeNode->NodeGuid);
        const FString NodeType = EnumToString(RuntimeNode->DialogueNodeType);

        AddRecord(TEXT("Node"), NodeGuid, NodeType, TEXT(""), TEXT(""), TEXT(""), TEXT("PositionX"), LexToString(RuntimeNode->Position.X));
        AddRecord(TEXT("Node"), NodeGuid, NodeType, TEXT(""), TEXT(""), TEXT(""), TEXT("PositionY"), LexToString(RuntimeNode->Position.Y));
        AddRecord(TEXT("Node"), NodeGuid, NodeType, TEXT(""), TEXT(""), TEXT(""), TEXT("NodeInfoClass"), RuntimeNode->NodeInfo ? RuntimeNode->NodeInfo->GetClass()->GetPathName() : TEXT(""));

        if (RuntimeNode->InputPin != nullptr)
        {
            AddRecord(TEXT("Pin"), NodeGuid, NodeType, GuidToString(RuntimeNode->InputPin->PinId), GuidToString(RuntimeNode->InputPin->LinkedToNodeGuid), TEXT("0"), TEXT("Direction"), TEXT("Input"));
            AddRecord(TEXT("Pin"), NodeGuid, NodeType, GuidToString(RuntimeNode->InputPin->PinId), GuidToString(RuntimeNode->InputPin->LinkedToNodeGuid), TEXT("0"), TEXT("PinName"), RuntimeNode->InputPin->PinName.ToString());
        }

        for (int32 PinIndex = 0; PinIndex < RuntimeNode->OutputPins.Num(); ++PinIndex)
        {
            const UDialogueRuntimePin* OutputPin = RuntimeNode->OutputPins[PinIndex];
            if (OutputPin == nullptr)
            {
                continue;
            }

            const FString OutputPinId = GuidToString(OutputPin->PinId);
            AddRecord(TEXT("Pin"), NodeGuid, NodeType, OutputPinId, GuidToString(OutputPin->LinkedToNodeGuid), LexToString(PinIndex), TEXT("Direction"), TEXT("Output"));
            AddRecord(TEXT("Pin"), NodeGuid, NodeType, OutputPinId, GuidToString(OutputPin->LinkedToNodeGuid), LexToString(PinIndex), TEXT("PinName"), OutputPin->PinName.ToString());

            for (int32 ConnectionIndex = 0; ConnectionIndex < OutputPin->Connections.Num(); ++ConnectionIndex)
            {
                const UDialogueRuntimePin* ConnectedPin = OutputPin->Connections[ConnectionIndex];
                if (ConnectedPin == nullptr)
                {
                    continue;
                }

                AddRecord(TEXT("Edge"), NodeGuid, NodeType, OutputPinId, GuidToString(ConnectedPin->OwnerNodeGuid), LexToString(ConnectionIndex), TEXT("ToPinId"), GuidToString(ConnectedPin->PinId));
                AddRecord(TEXT("Edge"), NodeGuid, NodeType, OutputPinId, GuidToString(ConnectedPin->OwnerNodeGuid), LexToString(ConnectionIndex), TEXT("ToPinName"), ConnectedPin->PinName.ToString());
            }
        }

        if (const UDialogueNodeInfo* DialogueNodeInfo = Cast<UDialogueNodeInfo>(RuntimeNode->NodeInfo))
        {
            AddRecord(TEXT("DialogueInfo"), NodeGuid, NodeType, TEXT(""), TEXT(""), TEXT(""), TEXT("Title"), DialogueNodeInfo->GetTitle().ToString());
            AddRecord(TEXT("DialogueInfo"), NodeGuid, NodeType, TEXT(""), TEXT(""), TEXT(""), TEXT("DialogueText"), DialogueNodeInfo->GetDialogueText().ToString());
            AddRecord(TEXT("DialogueInfo"), NodeGuid, NodeType, TEXT(""), TEXT(""), TEXT(""), TEXT("SpeakerID"), EnumToString(DialogueNodeInfo->GetSpeakerEmotePair().Speaker));
            AddRecord(TEXT("DialogueInfo"), NodeGuid, NodeType, TEXT(""), TEXT(""), TEXT(""), TEXT("IsShown"), DialogueNodeInfo->IsDialogueAlreadyShown() ? TEXT("true") : TEXT("false"));
            AddRecord(TEXT("DialogueInfo"), NodeGuid, NodeType, TEXT(""), TEXT(""), TEXT(""), TEXT("QuestToGive"), ExportPrivatePropertyText(DialogueNodeInfo, TEXT("QuestToGive")));
            AddRecord(TEXT("DialogueInfo"), NodeGuid, NodeType, TEXT(""), TEXT(""), TEXT(""), TEXT("QuestToClear"), ExportPrivatePropertyText(DialogueNodeInfo, TEXT("QuestToClear")));

            const FPortraitData PortraitData = DialogueNodeInfo->GetPortraitData();
            AddRecord(TEXT("DialogueInfo"), NodeGuid, NodeType, TEXT(""), TEXT(""), TEXT(""), TEXT("Portrait.ActionType"), EnumToString(PortraitData.ActionType));
            AddRecord(TEXT("DialogueInfo"), NodeGuid, NodeType, TEXT(""), TEXT(""), TEXT(""), TEXT("Portrait.EmoteType"), EnumToString(PortraitData.EmoteType));
            AddRecord(TEXT("DialogueInfo"), NodeGuid, NodeType, TEXT(""), TEXT(""), TEXT(""), TEXT("Portrait.SidePosition"), EnumToString(PortraitData.SidePosition));

            const TArray<FDialogueChoice>& Choices = DialogueNodeInfo->GetDialogueChoices();
            for (int32 ChoiceIndex = 0; ChoiceIndex < Choices.Num(); ++ChoiceIndex)
            {
                const FDialogueChoice& Choice = Choices[ChoiceIndex];
                const UDialogueRuntimePin* ChoicePin = RuntimeNode->OutputPins.IsValidIndex(ChoiceIndex) ? RuntimeNode->OutputPins[ChoiceIndex] : nullptr;
                const FString ChoicePinId = ChoicePin ? GuidToString(ChoicePin->PinId) : TEXT("");
                const FString ChoiceLinkedNodeGuid = ChoicePin ? GuidToString(ChoicePin->LinkedToNodeGuid) : TEXT("");

                AddRecord(TEXT("Choice"), NodeGuid, NodeType, ChoicePinId, ChoiceLinkedNodeGuid, LexToString(ChoiceIndex), TEXT("ResponseText"), Choice.ResponseText.ToString());
                AddRecord(TEXT("Choice"), NodeGuid, NodeType, ChoicePinId, ChoiceLinkedNodeGuid, LexToString(ChoiceIndex), TEXT("RequiredLevel"), LexToString(Choice.SelectableChoiceEvalCriteria.RequiredLevel));
                AddRecord(TEXT("Choice"), NodeGuid, NodeType, ChoicePinId, ChoiceLinkedNodeGuid, LexToString(ChoiceIndex), TEXT("RequiredTagQuery"), ExportTagQuery(Choice.SelectableChoiceEvalCriteria.RequiredTagQuery));
            }

            const TArray<FPortraitActionData>& PortraitActions = DialogueNodeInfo->GetPortraitActionDatas();
            for (int32 ActionIndex = 0; ActionIndex < PortraitActions.Num(); ++ActionIndex)
            {
                const FPortraitActionData& Action = PortraitActions[ActionIndex];
                AddRecord(TEXT("PortraitAction"), NodeGuid, NodeType, TEXT(""), TEXT(""), LexToString(ActionIndex), TEXT("ActionTargetSpeakerID"), EnumToString(Action.ActionTargetSpeakerID));
                AddRecord(TEXT("PortraitAction"), NodeGuid, NodeType, TEXT(""), TEXT(""), LexToString(ActionIndex), TEXT("PortraitActionEmoteType"), EnumToString(Action.PortraitActionEmoteType));
                AddRecord(TEXT("PortraitAction"), NodeGuid, NodeType, TEXT(""), TEXT(""), LexToString(ActionIndex), TEXT("ActionType"), EnumToString(Action.ActionType));
                AddRecord(TEXT("PortraitAction"), NodeGuid, NodeType, TEXT(""), TEXT(""), LexToString(ActionIndex), TEXT("Delay"), LexToString(Action.Delay));
                AddRecord(TEXT("PortraitAction"), NodeGuid, NodeType, TEXT(""), TEXT(""), LexToString(ActionIndex), TEXT("Duration"), LexToString(Action.Duration));
                AddRecord(TEXT("PortraitAction"), NodeGuid, NodeType, TEXT(""), TEXT(""), LexToString(ActionIndex), TEXT("FromTranslation"), FString::Printf(TEXT("%.3f,%.3f"), Action.FromTranslation.X, Action.FromTranslation.Y));
                AddRecord(TEXT("PortraitAction"), NodeGuid, NodeType, TEXT(""), TEXT(""), LexToString(ActionIndex), TEXT("TargetSide"), EnumToString(Action.TargetSide));
                AddRecord(TEXT("PortraitAction"), NodeGuid, NodeType, TEXT(""), TEXT(""), LexToString(ActionIndex), TEXT("TargetSideOffset"), FString::Printf(TEXT("%.3f,%.3f"), Action.TargetSideOffset.X, Action.TargetSideOffset.Y));
            }
        }

        if (const UDialogueBranchNodeInfoBase* BranchNodeInfo = Cast<UDialogueBranchNodeInfoBase>(RuntimeNode->NodeInfo))
        {
            AddRecord(TEXT("BranchCondition"), NodeGuid, NodeType, TEXT(""), TEXT(""), TEXT(""), TEXT("RequiredLevel"), LexToString(BranchNodeInfo->DialoguePassCondition.RequiredLevel));
            AddRecord(TEXT("BranchCondition"), NodeGuid, NodeType, TEXT(""), TEXT(""), TEXT(""), TEXT("RequiredTagQuery"), ExportTagQuery(BranchNodeInfo->DialoguePassCondition.RequiredTagQuery));

            for (const UDialogueRuntimePin* OutputPin : RuntimeNode->OutputPins)
            {
                if (OutputPin == nullptr)
                {
                    continue;
                }

                if (OutputPin->PinName.ToString().Equals(TEXT("True"), ESearchCase::IgnoreCase))
                {
                    AddRecord(TEXT("BranchCondition"), NodeGuid, NodeType, GuidToString(OutputPin->PinId), GuidToString(OutputPin->LinkedToNodeGuid), TEXT(""), TEXT("TrueNodeGuid"), GuidToString(OutputPin->LinkedToNodeGuid));
                }
                else if (OutputPin->PinName.ToString().Equals(TEXT("False"), ESearchCase::IgnoreCase))
                {
                    AddRecord(TEXT("BranchCondition"), NodeGuid, NodeType, GuidToString(OutputPin->PinId), GuidToString(OutputPin->LinkedToNodeGuid), TEXT(""), TEXT("FalseNodeGuid"), GuidToString(OutputPin->LinkedToNodeGuid));
                }
            }
        }

        if (const UDialogueEndNodeInfo* EndNodeInfo = Cast<UDialogueEndNodeInfo>(RuntimeNode->NodeInfo))
        {
            AddRecord(TEXT("EndInfo"), NodeGuid, NodeType, TEXT(""), TEXT(""), TEXT(""), TEXT("Action"), EnumToString(EndNodeInfo->Action));
            AddRecord(TEXT("EndInfo"), NodeGuid, NodeType, TEXT(""), TEXT(""), TEXT(""), TEXT("ActionDetails"), EndNodeInfo->ActionDetails);
            AddRecord(TEXT("EndInfo"), NodeGuid, NodeType, TEXT(""), TEXT(""), TEXT(""), TEXT("ClearTag"), EndNodeInfo->ClearTag.ToString());
            AddRecord(TEXT("EndInfo"), NodeGuid, NodeType, TEXT(""), TEXT(""), TEXT(""), TEXT("QuestBase"), EndNodeInfo->QuestBase.ToSoftObjectPath().ToString());
            AddRecord(TEXT("EndInfo"), NodeGuid, NodeType, TEXT(""), TEXT(""), TEXT(""), TEXT("QuestRootTag"), EndNodeInfo->QuestRootTag.ToString());
            AddRecord(TEXT("EndInfo"), NodeGuid, NodeType, TEXT(""), TEXT(""), TEXT(""), TEXT("SelectedQuestStepTag"), EndNodeInfo->SelectedQuestStepTag.ToString());
            AddRecord(TEXT("EndInfo"), NodeGuid, NodeType, TEXT(""), TEXT(""), TEXT(""), TEXT("SelectedQuestStepClearTag"), EndNodeInfo->SelectedQuestStep.ClearTag.ToString());

            for (int32 RewardIndex = 0; RewardIndex < EndNodeInfo->SelectedQuestStep.RewardItems.Num(); ++RewardIndex)
            {
                const FDataTableRowHandle& RewardHandle = EndNodeInfo->SelectedQuestStep.RewardItems[RewardIndex];
                const FString RewardDataTablePath = RewardHandle.DataTable ? RewardHandle.DataTable->GetPathName() : TEXT("");
                const FString RewardRowName = RewardHandle.RowName.ToString();
                AddRecord(TEXT("EndReward"), NodeGuid, NodeType, TEXT(""), TEXT(""), LexToString(RewardIndex), TEXT("DataTableRow"), FString::Printf(TEXT("%s|%s"), *RewardDataTablePath, *RewardRowName));
            }
        }
    }

    return OutCSV;
}

FString FDialogueGraphEditor::OpenCSVSaveWindow() const
{
    if (WorkingAsset == nullptr)
    {
        return TEXT("");
    }

    const FString DefaultDirectory = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("DialogueCSV"));
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
