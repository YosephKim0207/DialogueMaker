#pragma once

#include "CoreMinimal.h"
#include "DialogueNodeInfoBase.h"
#include "GameplayTagContainer.h"
#include "QuestBase.h"
#include "DialogueEndNodeInfo.generated.h"

UENUM(BlueprintType)
enum class EDialogueNodeAction : uint8
{
	None,
	StartQuest,
	AdvanceQuest,
	EndQuest,
};

UCLASS(BlueprintType)
class DIALOGUEMAKER_API UDialogueEndNodeInfo : public UDialogueNodeInfoBase
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere)
	EDialogueNodeAction Action = EDialogueNodeAction::None;

	UPROPERTY(EditAnywhere)
	FString ActionDetails;
	
	UPROPERTY(EditAnywhere)
	TSoftObjectPtr<UQuestBase> QuestBase;

	UPROPERTY(EditAnywhere)
	FGameplayTag ClearTag;
};
