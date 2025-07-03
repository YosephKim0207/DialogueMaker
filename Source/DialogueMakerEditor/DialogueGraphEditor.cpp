#include "DialogueGraphEditor.h"

#include "DialogueEdGraph.h"
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

    Graph = Cast<UDialogueGraph>(InGraph);
    InitAssetEditor(Mode, InitToolkitHost, FName("DialogueGraphEditor"), FTabManager::FLayout::NullLayout, true, true, InGraph);

    // Editor의 Mode를 DialogueGraphEditorMode로 설정
    AddApplicationMode(TEXT("DialogueGraphEditorMode"), MakeShareable(new DialogueGraphEditorMode(SharedThis(this))));
    SetCurrentMode(TEXT("DialogueGraphEditorMode"));
}

void FDialogueGraphEditor::OnNodeSelectionChanged(const TSet<UObject*>& NewSelection)
{
    TArray<UObject*> Array = NewSelection.Array();
    // DetailsView->SetObjects(Array.Num() ? Array : TArray<UObject*>{ GraphAsset });
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
