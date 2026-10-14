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
class DIALOGUEKIT_API UDialogueEndNodeInfo : public UDialogueNodeInfoBase
{
	GENERATED_BODY()

public:
	// 정상 완료는 캐시 갱신이며 저장 요청은 명시 설정한 경우에만 보낸다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DialogueKit|End")
	bool bSaveOnEnd = false;

    UPROPERTY(EditAnywhere, Category = "DialogueKit|DialogueEndNodeInfo|Action")
    EDialogueNodeAction Action = EDialogueNodeAction::None;

    UPROPERTY(EditAnywhere, Category = "DialogueKit|DialogueEndNodeInfo|Action")
    FString ActionDetails;

    UPROPERTY(EditAnywhere, Category = "DialogueKit|DialogueEndNodeInfo|Action", meta = (ToolTip = "Quest가 없고, 대화 종료시 처리할 Tag가 있는 경우 사용"))
    FGameplayTag ClearTag;

    UPROPERTY(EditAnywhere, Category = "DialogueKit|DialogueEndNodeInfo|Quest")
    TSoftObjectPtr<UQuestBase> QuestBase;

    UPROPERTY(VisibleAnywhere, Category = "DialogueKit|DialogueEndNodeInfo|Quest")
    FGameplayTag QuestRootTag;

    UPROPERTY(EditAnywhere, Category = "DialogueKit|DialogueEndNodeInfo|Quest", meta = (GetOptions = "GetQuestStepTagOptions"))
    FName SelectedQuestStepTag;

    UPROPERTY(VisibleAnywhere, Category = "DialogueKit|DialogueEndNodeInfo|Quest")
    FQuestStep SelectedQuestStep;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;

	UFUNCTION()
	TArray<FName> GetQuestStepTagOptions() const;
#endif
};
