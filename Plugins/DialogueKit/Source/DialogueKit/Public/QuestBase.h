#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "QuestBase.generated.h"

USTRUCT(BlueprintType)
struct FQuestStep
{
	GENERATED_BODY()
	
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DialogueKit|FQuestStep|Tag", meta = (ToolTip = "구형 퀘스트 단계의 완료를 식별하는 태그입니다. End의 Selected Quest Step Tag 목록에 표시됩니다."))
    FGameplayTag ClearTag;

    UPROPERTY(EditAnywhere, Category = "DialogueKit|FQuestStep|Reward", meta = (ToolTip = "구형 퀘스트 단계의 보상 아이템 행 참조 목록입니다. 원본 퀘스트 에셋에서 설정하며 신규 Block의 Effects와는 별개입니다.", RowType = "/Script/DialogueMaker.ItemRow", TitleProperty = "DisplayName"))
    TArray<FDataTableRowHandle> RewardItems;
};

UCLASS()
class DIALOGUEKIT_API UQuestBase : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:
	FName GetQuestID() const;
	FGameplayTag GetQuestRootTag() const;
	bool IsLastQuestStep(int32 CheckIndex) const;

	const TArray<FQuestStep> GetQuestSteps() const;
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

#if WITH_EDITOR
	const FQuestStep GetQuestStepByClearTag(const FName& SearchTargetTagName);
#endif
	
private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DialogueKit|QuestBase|Quest|ID", meta = (AllowPrivateAccess=true))
    FName QuestID;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DialogueKit|QuestBase|Quest|Tag", meta = (AllowPrivateAccess=true))
    FGameplayTag QuestRootTag;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DialogueKit|QuestBase|Quest|Progress", meta = (AllowPrivateAccess=true))
    TArray<FQuestStep> QuestSteps;
};