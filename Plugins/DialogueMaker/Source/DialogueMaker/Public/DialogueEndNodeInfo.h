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

	UPROPERTY(EditAnywhere, meta = (ToolTip = "Quest가 없고, 대화 종료시 처리할 Tag가 있는 경우 사용"))
	FGameplayTag ClearTag;
	
	UPROPERTY(EditAnywhere, Category = "Quest")
	TSoftObjectPtr<UQuestBase> QuestBase;
	
	UPROPERTY(VisibleAnywhere, Category = "Quest")
	FGameplayTag QuestRootTag;

	UPROPERTY(EditAnywhere, Category = "Quest", meta = (GetOptions = "GetQuestStepTagOptions"))
	FName SelectedQuestStepTag;

	UPROPERTY(VisibleAnywhere, Category = "Quest")
	FQuestStep SelectedQuestStep;
	
#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
	
	UFUNCTION()
	TArray<FName> GetQuestStepTagOptions() const;
#endif
};
