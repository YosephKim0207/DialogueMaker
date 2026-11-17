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
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DialogueKit|End", meta = (ToolTip = "Block 종료 시 저장 경계를 요청합니다. 해제하면 정상 종료 상태는 캐시에 반영되지만 이 옵션에 의한 저장 요청은 보내지 않습니다. 저장 완료와는 별개입니다."))
	bool bSaveOnEnd = false;

    UPROPERTY(EditAnywhere, Category = "DialogueKit|DialogueEndNodeInfo|Action", meta = (ToolTip = "구형 종료 액션 분류입니다. None은 Clear Tag 추가에 사용합니다. StartQuest/AdvanceQuest/EndQuest의 실제 후속 처리는 현재 미구현이며 신규 Block 실행기도 이 액션을 실행하지 않습니다."))
    EDialogueNodeAction Action = EDialogueNodeAction::None;

    UPROPERTY(EditAnywhere, Category = "DialogueKit|DialogueEndNodeInfo|Action", meta = (ToolTip = "구형 종료 액션의 부가 문자열입니다. 현재 기본 실행 코드에서 사용하지 않으므로 입력만으로 효과가 발생하지 않습니다."))
    FString ActionDetails;

    UPROPERTY(EditAnywhere, Category = "DialogueKit|DialogueEndNodeInfo|Action", meta = (ToolTip = "구형 종료 경로에서 Action이 None일 때 플레이어 진행에 추가할 태그입니다. 이름과 달리 태그를 제거하는 기능이 아닙니다. 신규 Block 실행기는 사용하지 않습니다."))
    FGameplayTag ClearTag;

    UPROPERTY(EditAnywhere, Category = "DialogueKit|DialogueEndNodeInfo|Quest", meta = (ToolTip = "구형 종료 액션이 참조할 퀘스트 에셋입니다. 신규 Block의 게임 상태 변경은 효과 제공자로 연결합니다."))
    TSoftObjectPtr<UQuestBase> QuestBase;

    UPROPERTY(VisibleAnywhere, Category = "DialogueKit|DialogueEndNodeInfo|Quest", meta = (ToolTip = "선택한 구형 퀘스트의 루트 태그입니다. Quest Base 변경 시 갱신되는 읽기 전용 정보입니다."))
    FGameplayTag QuestRootTag;

    UPROPERTY(EditAnywhere, Category = "DialogueKit|DialogueEndNodeInfo|Quest", meta = (ToolTip = "Quest Base에 정의된 단계의 Clear Tag를 선택합니다. 선택 결과는 아래 Selected Quest Step에 표시됩니다. 구형 퀘스트 연동용입니다.", GetOptions = "GetQuestStepTagOptions"))
    FName SelectedQuestStepTag;

    UPROPERTY(VisibleAnywhere, Category = "DialogueKit|DialogueEndNodeInfo|Quest", meta = (ToolTip = "선택한 구형 퀘스트 단계의 태그와 보상 자료입니다. 읽기 전용이며 원본 퀘스트 에셋에서 편집합니다."))
    FQuestStep SelectedQuestStep;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;

	UFUNCTION()
	TArray<FName> GetQuestStepTagOptions() const;
#endif
};
