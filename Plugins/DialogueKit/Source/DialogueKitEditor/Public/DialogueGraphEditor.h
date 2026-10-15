#pragma once

#include "CoreMinimal.h"
#include "UObject/GCObject.h"
#include "DialogueGraph.h"
#include "WorkflowOrientedApp/WorkflowCentricApplication.h"

class FDialogueGraphEditor : public FWorkflowCentricApplication, public FEditorUndoClient, public FNotifyHook, public FGCObject
{
public:
	// Toolkit은 UObject가 아니므로 작업 그래프를 GC에 명시적으로 보존한다.
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	virtual FString GetReferencerName() const override { return TEXT("FDialogueGraphEditor"); }
	virtual void SaveAsset_Execute() override;
	virtual void SaveAssetAs_Execute() override;
	virtual void RegisterTabSpawners  (const TSharedRef<FTabManager>&) override;
	void InitEditor(const EToolkitMode::Type Mode, const TSharedPtr<class IToolkitHost>& InitToolkitHost, class UDialogueGraph* InGraph);
	UDialogueGraph* GetWorkingAsset() const { return WorkingAsset; }
	UEdGraph* GetWorkingGraph() const { return WorkingGraph; }
	void SetWorkingGraphUI(TSharedPtr<SGraphEditor> NewWorkingGraphUI);
	void SetSelectedDetailView(TSharedPtr<IDetailsView> NewDetailsView);
	void OnGraphSelectionChanged(const FGraphPanelSelectionSet& NewSelection);
	
	virtual FName GetToolkitFName() const override;
	virtual FText GetBaseToolkitName() const override;
	virtual FString GetWorldCentricTabPrefix() const override;
	virtual FLinearColor GetWorldCentricTabColorScale() const override;
	virtual void OnToolkitHostingStarted(const TSharedRef<IToolkit>& Toolkit) override { };
	virtual void OnToolkitHostingFinished(const TSharedRef<IToolkit>& Toolkit) override { };

	static bool ExportDialogueGraphAssetToCSV(const UDialogueGraph* InDialogueGraph, const FString& CSVFilePath);
	static FString BuildDialogueGraphCSVFromAsset(const UDialogueGraph* InDialogueGraph);

	virtual void OnClose() override;
	virtual void PostUndo(bool bSuccess) override;
	virtual void PostRedo(bool bSuccess) override { PostUndo(bSuccess); }
	void OnNodeDetailViewPropertiesUpdated(const FPropertyChangedEvent& Event);
	void OnWorkingAssetPreSave();

protected:
	void UpdateWorkingAssetFromGraph();
	void UpdateEditorGraphFromWorkingAsset();
	class UDialogueEdGraphNodeBase* GetSelectedNode(const FGraphPanelSelectionSet& Selection);

	// Toolbar
	void FillToolbar(FToolBarBuilder& ToolbarBuilder);
	bool CanConvertCSV() const;
	
	// Convert To CSV
	void OnConvertToCSVButtonClicked();
	bool ExportDialogueGraphToCSV(const FString& CSVFilePath) const;
	FString BuildDialogueGraphCSV() const;
	FString OpenCSVSaveWindow() const;

	// CSV To DialogueLocalization DataAsset
	void OnConvertCSVToDialogueLocalizationButtonClicked();
	bool ConvertCSVToDialogueLocalizationDataAsset(const FString& CSVFilePath);
	FString OpenCSVLoadWindow() const;

	
private:
	bool bCanCommitWorkingGraph = false;
	TSharedPtr<SGraphEditor> GraphEditor;
	TSharedPtr<IDetailsView> DetailsView;
	
        UPROPERTY(EditAnywhere, Category = "DialogueKit|DialogueGraphEditor|Asset")
        TObjectPtr<UDialogueGraph> WorkingAsset;
	
	UPROPERTY()
	TObjectPtr<UEdGraph> WorkingGraph;	// 작업 중인 Graph의 Data
	
	const FName GraphTabID = FName("DialogueKit_GraphEditor");  // "DialogueGraph_Tab"
	const FName DetailsTabID = FName("DialogueKit_Details");	// "DialogueGraph_Tab"
	
	// Graph가 그려지는 Slate Widget
	TSharedPtr<SGraphEditor> WorkingGraphUI;
	TSharedPtr<IDetailsView> SelectedDetailView;

	// Toolbar
	TSharedPtr<FUICommandList> GraphEditorCommands;
};
