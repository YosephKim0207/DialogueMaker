#pragma once

#include "CoreMinimal.h"
#include "DialogueEdGraph.h"
#include "DialogueGraph.h"
#include "WorkflowOrientedApp/WorkflowCentricApplication.h"

class FDialogueGraphEditor : public FWorkflowCentricApplication, public FEditorUndoClient, public FNotifyHook
{
public:
	virtual void RegisterTabSpawners  (const TSharedRef<FTabManager>&) override;
	void InitEditor(const EToolkitMode::Type Mode, const TSharedPtr<class IToolkitHost>& InitToolkitHost, class UDialogueGraph* InGraph);
	UDialogueGraph* GetWorkingAsset() const { return Graph; }
	UEdGraph* GetWorkingGraph() const { return EdGraph; }
	
	virtual FName GetToolkitFName() const override;
	virtual FText GetBaseToolkitName() const override;
	virtual FString GetWorldCentricTabPrefix() const override;
	virtual FLinearColor GetWorldCentricTabColorScale() const override;
	virtual void OnToolkitHostingStarted(const TSharedRef<IToolkit>& Toolkit) override { };
	virtual void OnToolkitHostingFinished(const TSharedRef<IToolkit>& Toolkit) override { };
	
	

	void OnNodeSelectionChanged(const TSet<UObject*>& NewSelection);
private:
	TSharedPtr<SGraphEditor> GraphEditor;
	TSharedPtr<IDetailsView> DetailsView;
	/** 데이터 */
	UPROPERTY(EditAnywhere)
	TObjectPtr<UDialogueGraph> Graph;
	UPROPERTY()
	TObjectPtr<UEdGraph> EdGraph;
	
	const FName GraphTabID = FName("DialogueMaker_GraphEditor");  // "DialogueGraph_Tab"
	const FName DetailsTabID = FName("DialogueMaker_Details");	// "DialogueGraph_Tab"
};
