#pragma once

#include "CoreMinimal.h"
#include "DialogueNodeInfoBase.h"
#include "GameplayTagContainer.h"
#include "QuestBase.h"
#include "DialogueBranchNodeInfoBase.generated.h"

UCLASS()
class DIALOGUEMAKER_API UDialogueBranchNodeInfoBase : public UDialogueNodeInfoBase
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere)
	int32 RequiredLevel = 0;

	UPROPERTY(EditAnywhere)
	TSubclassOf<UQuestBase> RequiredQuest;

	UPROPERTY(EditAnywhere)
	FGameplayTag RequiredTag;

	UPROPERTY(EditAnywhere)
	FGameplayTagQuery RequiredTagQuery;

	UPROPERTY(EditAnywhere)
	FGameplayTagContainer RequiredTagContainer;
};
