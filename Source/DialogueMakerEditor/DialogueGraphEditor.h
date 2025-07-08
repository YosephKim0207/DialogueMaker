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
	
	virtual FName GetToolkitFName() const override;
	virtual FText GetBaseToolkitName() const override;
	virtual FString GetWorldCentricTabPrefix() const override;
	virtual FLinearColor GetWorldCentricTabColorScale() const override;
	virtual void OnToolkitHostingStarted(const TSharedRef<IToolkit>& Toolkit) override { };
	virtual void OnToolkitHostingFinished(const TSharedRef<IToolkit>& Toolkit) override { };

	virtual void OnClose() override;
	void OnGraphChanged(const FEdGraphEditAction& EditAction);
	
	void OnNodeSelectionChanged(const TSet<UObject*>& NewSelection);

protected:
	void UpdateWorkingAssetFromGraph();
	void UpdateEditorGraphFromWorkingAsset();

private:
	TSharedPtr<SGraphEditor> GraphEditor;
	TSharedPtr<IDetailsView> DetailsView;
	/** 데이터 */
	UPROPERTY(EditAnywhere)
	TObjectPtr<UDialogueGraph> WorkingAsset;
	UPROPERTY()
	TObjectPtr<UEdGraph> WorkingGraph;
	
	const FName GraphTabID = FName("DialogueMaker_GraphEditor");  // "DialogueGraph_Tab"
	const FName DetailsTabID = FName("DialogueMaker_Details");	// "DialogueGraph_Tab"

	FDelegateHandle GraphChangeListenerHandle;
};


