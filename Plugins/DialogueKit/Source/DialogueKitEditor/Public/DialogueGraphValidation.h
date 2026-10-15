#pragma once
#include "CoreMinimal.h"
class UEdGraph;
class UEdGraphNode;
class UDialogueGraph;
class UPackage;
struct FDialogueGraphIssue
{
	TWeakObjectPtr<UEdGraphNode> Node;
	FName Code;
	FText Message;
	bool bBlocksSave = false;
};
class FDialogueGraphValidation
{
public:
	static TArray<FDialogueGraphIssue> Inspect(UEdGraph* Graph);
	static void RegisterWorkingGraph(UDialogueGraph* Asset, UEdGraph* Graph);
	static void UnregisterWorkingGraph(UDialogueGraph* Asset);
	static void Startup();
	static void Shutdown();
	static bool CanSavePackage(UPackage* Package);
	static bool CanSave(const TArray<FDialogueGraphIssue>& Issues);
};
