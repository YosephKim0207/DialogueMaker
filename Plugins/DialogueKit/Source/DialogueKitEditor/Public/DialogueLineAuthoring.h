#pragma once
#include "CoreMinimal.h"
#include "DialogueEdGraphSchema.h"
#include "DialogueLineAuthoring.generated.h"

class UDialogueLineNodeInfo;
class UDialogueFlowEdGraphNode;

class DIALOGUEKITEDITOR_API FDialogueLineAuthoring
{
public:
	static TArray<UDialogueFlowEdGraphNode*> FindNearestLines(const UEdGraphPin* FromPin);
	static UDialogueFlowEdGraphNode* CreateContinuation(UEdGraph* Graph, UEdGraphPin* FromPin, UDialogueFlowEdGraphNode* Source, FVector2D Location);
	static void ShowTimingDialog(UEdGraph* Graph);
	static bool ApplyTiming(UEdGraph* Graph, bool bWholeLine, float Seconds);
};

USTRUCT()
struct FContinueLineAction : public FNewNodeAction
{
	GENERATED_BODY()
	FContinueLineAction();
	virtual UEdGraphNode* PerformAction(UEdGraph* Graph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode = true) override;
};
