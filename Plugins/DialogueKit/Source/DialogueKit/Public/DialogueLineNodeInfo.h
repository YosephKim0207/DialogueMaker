#pragma once

#include "CoreMinimal.h"
#include "DialogueNodeInfo.h"
#include "DialogueLineNodeInfo.generated.h"

// 기존 화자/본문/초상화 읽기 API와 열람 버전을 재사용하는 단일 발화다.
UCLASS(BlueprintType, HideCategories = ("DialogueKit|DialogueNodeInfo|Quest", "DialogueKit|DialogueNodeInfo|State"))
class DIALOGUEKIT_API UDialogueLineNodeInfo : public UDialogueNodeInfo
{
	GENERATED_BODY()
public:
	// 새 Line은 내부 선택 배열을 편집하거나 진행 슬롯으로 사용하지 않는다.
	UDialogueLineNodeInfo() { bStandaloneLine = true; }
};
