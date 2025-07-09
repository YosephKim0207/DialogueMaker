#pragma once

#include "CoreMinimal.h"
#include "DialogueEdGraph.h"
#include "DialogueMaker/DialogueGraph.h"
#include "WorkflowOrientedApp/WorkflowCentricApplication.h"

class FDialogueGraphEditor : public FWorkflowCentricApplication, public FEditorUndoClient, public FNotifyHook
{
public:
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

	virtual void OnClose() override;
	void OnNodeDetailViewPropertiesUpdated(const FPropertyChangedEvent& Event);
	void OnGraphChanged(const FEdGraphEditAction& EditAction);
	
	// void OnNodeSelectionChanged(const TSet<UObject*>& NewSelection);
		
protected:
	void UpdateWorkingAssetFromGraph();
	void UpdateEditorGraphFromWorkingAsset();

private:
	TSharedPtr<SGraphEditor> GraphEditor;
	TSharedPtr<IDetailsView> DetailsView;
	
	UPROPERTY(EditAnywhere)
	TObjectPtr<UDialogueGraph> WorkingAsset;
	
	UPROPERTY()
	TObjectPtr<UEdGraph> WorkingGraph;	// 작업 중인 Graph의 Data
	
	const FName GraphTabID = FName("DialogueMaker_GraphEditor");  // "DialogueGraph_Tab"
	const FName DetailsTabID = FName("DialogueMaker_Details");	// "DialogueGraph_Tab"

	FDelegateHandle GraphChangeListenerHandle;
	// Graph가 그려지는 Slate Widget
	TSharedPtr<SGraphEditor> WorkingGraphUI;
	TSharedPtr<IDetailsView> SelectedDetailView;
};


