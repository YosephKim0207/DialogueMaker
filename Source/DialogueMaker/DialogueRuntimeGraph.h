#pragma once

#include "CoreMinimal.h"
#include "DialogueNodeInfoBase.h"
#include "DialogueNodeType.h"
#include "DialogueRuntimeGraph.generated.h"

UCLASS()
class DIALOGUEMAKER_API UDialogueRuntimePin : public UObject
{
	GENERATED_BODY()
	
public:
	UPROPERTY()
	FName PinName;

	UPROPERTY()
	FGuid PinId;

	UPROPERTY()
	TArray<UDialogueRuntimePin*> Connections;
};

UCLASS()
class DIALOGUEMAKER_API UDialogueRuntimeNode : public UObject
{
	GENERATED_BODY()
	
public:
	UPROPERTY()
	EDialogueType DialogueNodeType = EDialogueType::DialogueNode;
	
	UPROPERTY()
	UDialogueRuntimePin* InputPin;

	UPROPERTY()
	TArray<UDialogueRuntimePin*> OutputPins;

	UPROPERTY()
	FVector2D Position;

	UPROPERTY()
	UDialogueNodeInfoBase * NodeInfo;
};

UCLASS()
class DIALOGUEMAKER_API UDialogueRuntimeGraph : public UObject
{
	GENERATED_BODY()
	
public:
	UPROPERTY()
	TArray<UDialogueRuntimeNode*> Nodes;
};