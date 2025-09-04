#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "QuestBase.generated.h"

USTRUCT(BlueprintType)
struct FQuestStep
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tag")
	FGameplayTag ClearTag;

	UPROPERTY(EditAnywhere, Category = "Reward", meta = (RowType = "/Script/DialogueMaker.ItemRow", TitleProperty = "DisplayName"))
	TArray<FDataTableRowHandle> RewardItems;
};

UCLASS()
class DIALOGUEMAKER_API UQuestBase : public UPrimaryDataAsset
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
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Quest|ID", meta = (AllowPrivateAccess=true))
	FName QuestID;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest|Tag", meta = (AllowPrivateAccess=true))
	FGameplayTag QuestRootTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest|Progress", meta = (AllowPrivateAccess=true))
	TArray<FQuestStep> QuestSteps;
};

