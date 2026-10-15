#include "DialogueGraphEditor.h"
#include "DialogueLineAuthoring.h"
#include "DialogueGraphCodec.h"

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
    if (bCanCommitWorkingGraph == false)
    {
        return;
    }
    FString Error;
    if (FDialogueGraphCodec::ExportGraph(WorkingGraph, WorkingAsset, Error) == false)
    {
        UE_LOG(DialogueKitEditorSub, Warning, TEXT("FDialogueGraphEditor::UpdateWorkingAssetFromGraph : %s"), *Error);
    }
}

// 에셋의 실행 자료를 동일한 공통 변환 경로로 복원한다.
void FDialogueGraphEditor::UpdateEditorGraphFromWorkingAsset()
{
    FString Error;
    bCanCommitWorkingGraph = FDialogueGraphCodec::ImportGraph(WorkingAsset, WorkingGraph, Error);
    if (bCanCommitWorkingGraph == false)
    {
        UE_LOG(DialogueKitEditorSub, Warning, TEXT("FDialogueGraphEditor::UpdateEditorGraphFromWorkingAsset : %s"), *Error);
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
    ToolbarBuilder.AddToolBarButton(FUIAction(FExecuteAction::CreateLambda([this]()
    {
        FDialogueLineAuthoring::ShowTimingDialog(WorkingGraph);
    })), NAME_None, FText::FromString(TEXT("출력 시간 일괄 변경")),
        FText::FromString(TEXT("현재 Block의 모든 Line 출력 시간을 변경합니다.")), FSlateIcon());
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

    // 변환 실패 시 이전 에셋 내용을 새 편집 결과로 잘못 내보내지 않는다.
    FString ConversionError;
    if (bCanCommitWorkingGraph == false || FDialogueGraphCodec::ExportGraph(WorkingGraph, WorkingAsset, ConversionError) == false)
    {
        FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(ConversionError));
        return;
    }

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

// 닫히기 전까지 편집용 객체와 원본 에셋의 수명을 유지한다.
void FDialogueGraphEditor::AddReferencedObjects(FReferenceCollector& Collector)
{
    Collector.AddReferencedObject(WorkingAsset);
    Collector.AddReferencedObject(WorkingGraph);
}

// 변환 실패를 성공 저장처럼 보이지 않게 하고 원래 에셋을 보존한다.
void FDialogueGraphEditor::SaveAsset_Execute()
{
    FString Error;
    if (bCanCommitWorkingGraph == false || FDialogueGraphCodec::ExportGraph(WorkingGraph, WorkingAsset, Error) == false)
    {
        FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(TEXT("Graph conversion failed. The existing asset was preserved. ") + Error));
        return;
    }
    FAssetEditorToolkit::SaveAsset_Execute();
}

// 다른 이름 저장에도 같은 기본 구조 검사를 적용한다.
void FDialogueGraphEditor::SaveAssetAs_Execute()
{
    FString Error;
    if (bCanCommitWorkingGraph == false || FDialogueGraphCodec::ExportGraph(WorkingGraph, WorkingAsset, Error) == false)
    {
        FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(TEXT("Graph conversion failed. The existing asset was preserved. ") + Error));
        return;
    }
    FAssetEditorToolkit::SaveAssetAs_Execute();
}
