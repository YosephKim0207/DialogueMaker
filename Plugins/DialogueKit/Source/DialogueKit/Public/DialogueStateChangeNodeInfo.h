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
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DialogueKit|StateChange")
	TArray<FDialogueEffect> Effects;
};
