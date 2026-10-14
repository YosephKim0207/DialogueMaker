#pragma once

#include "CoreMinimal.h"
#include "DialogueNodeInfoBase.h"
#include "DialogueNodeType.h"
#include "DialogueRuntimeGraph.generated.h"

UCLASS()
class DIALOGUEKIT_API UDialogueRuntimePin : public UObject
{
	GENERATED_BODY()
	
public:
	UPROPERTY()
	FName PinName;

	UPROPERTY()
	FGuid PinId;

	// Choice 항목과 출력 핀의 대응이다. 일반 진행 핀은 invalid를 유지한다.
	UPROPERTY()
	FGuid ChoiceId;

	UPROPERTY()
	FGuid OwnerNodeGuid;

	UPROPERTY()
	FGuid LinkedToNodeGuid;

	UPROPERTY()
	TArray<UDialogueRuntimePin*> Connections;
};

UCLASS()
class DIALOGUEKIT_API UDialogueRuntimeNode : public UObject
{
	GENERATED_BODY()
	
public:
	UPROPERTY()
	EDialogueType DialogueNodeType = EDialogueType::DialogueNode;

	UPROPERTY()
	FGuid NodeGuid;
	
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
class DIALOGUEKIT_API UDialogueRuntimeGraph : public UObject
{
	GENERATED_BODY()
	
public:
	UPROPERTY()
	TArray<UDialogueRuntimeNode*> Nodes;

	// ID/소유/선택 핀 대응을 검사하며 누락 ID를 자동 수정하지 않는다.
	bool ValidateIdentifiers(TArray<FString>& OutIssues) const;
};
