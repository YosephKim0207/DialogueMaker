#pragma once

#include "CoreMinimal.h"
#include "DialogueNodeInfo.h"
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
	UDialogueRuntimePin* InputPin;

	UPROPERTY()
	TArray<UDialogueRuntimePin*> OutputPins;

	UPROPERTY()
	FVector2D Position;

	UPROPERTY()
	UDialogueNodeInfo* NodeInfo;
};

UCLASS()
class DIALOGUEMAKER_API UDialogueRuntimeGraph : public UObject
{
	GENERATED_BODY()
	
public:
	UPROPERTY()
	TArray<UDialogueRuntimeNode*> Nodes;
};