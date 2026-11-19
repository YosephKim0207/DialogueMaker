#pragma once

#include "CoreMinimal.h"
#include "DialogueGraphType.generated.h"

// 기존 enum 이름과 숫자를 유지하여 저장된 대화 유형을 그대로 읽는다.
UENUM(BlueprintType)
enum class EDialogueGraphType : uint8
{
	Quest = 0 UMETA(DisplayName = "퀘스트", ToolTip = "퀘스트 관련 대화 분류입니다. 퀘스트 실행이나 최우선 지정을 자동 수행하지 않습니다."),
	Story = 10 UMETA(DisplayName = "스토리", ToolTip = "서사 진행 대화 분류입니다. Chapter/Scene 소속과 진입 조건은 별도로 지정합니다."),
	DailyChat = 100 UMETA(DisplayName = "일상", ToolTip = "일상 대화 분류입니다. 기본 대화 또는 반복 가능 상태로 자동 지정하지 않습니다.")
};
