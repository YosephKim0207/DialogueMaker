#pragma once
#include "CoreMinimal.h"
#include "DialogueEditorLayout.generated.h"

// 편집기 배치만 저장한다. 실행 노드 본문과 게임 저장 데이터는 중복하지 않는다.
USTRUCT()
struct DIALOGUEKIT_API FDialogueLayoutNode
{
	GENERATED_BODY()
	UPROPERTY() FGuid Id;
	UPROPERTY() bool bComment = false;
	UPROPERTY() FVector2D Position = FVector2D::ZeroVector;
	UPROPERTY() FVector2D Size = FVector2D(400, 200);
	UPROPERTY() FString Text;
	UPROPERTY() FText Description;
	UPROPERTY() FLinearColor Color = FLinearColor::White;
	UPROPERTY() int32 FontSize = 18;
	UPROPERTY() bool bGroupMovement = true;
	UPROPERTY() FGuid InputId;
	UPROPERTY() FGuid OutputId;
};

USTRUCT()
struct DIALOGUEKIT_API FDialogueLayoutEdge
{
	GENERATED_BODY()
	UPROPERTY() FGuid OutputId;
	UPROPERTY() FGuid InputId;
};

USTRUCT()
struct DIALOGUEKIT_API FDialogueEditorLayout
{
	GENERATED_BODY()
	UPROPERTY() int32 Version = 1;
	UPROPERTY() bool bPresent = false;
	UPROPERTY() TArray<FDialogueLayoutNode> Nodes;
	UPROPERTY() TArray<FDialogueLayoutEdge> Edges;
};
