#pragma once

#include "CoreMinimal.h"
#include "DialogueNodeInfoBase.h"
#include "DialogueEffectContract.h"
#include "DialogueStateChangeNodeInfo.generated.h"

UCLASS(BlueprintType)
class DIALOGUEKIT_API UDialogueStateChangeNodeInfo : public UDialogueNodeInfoBase
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DialogueKit|StateChange", meta = (ToolTip = "이 노드에 도달하면 게임 효과 제공자에게 적용을 요청할 상태 변경 묶음입니다. 전량 성공 또는 실패 시 무변경이 계약이며 성공 후 저장 경계를 요청합니다."))
	TArray<FDialogueEffect> Effects;
};
