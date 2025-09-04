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
	TSubclassOf<UQuestBase> QuestBase;

	// TODO 퀘스트 클리어, 퀘스트 부여, 퀘스트 진행도 업데이트, 퀘스트 보상 등의 데이터
	UPROPERTY(EditAnywhere)
	FGameplayTagContainer AddGameplayTags;
};
