#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Struct/ItemRow.h"
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
class DIALOGUEMAKER_API UQuestBase : public UObject
{
	GENERATED_BODY()
public:
	void SyncQuestProgress();	
	FName GetQuestID() const;
	FGameplayTag GetQuestBaseTag() const;
	bool IsLastQuestStep(int32 CheckIndex) const;

private:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Quest|ID", meta = (AllowPrivateAccess=true))
	FName QuestID;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest|Tag", meta = (AllowPrivateAccess=true))
	FGameplayTag QuestBaseTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest|Progress", meta = (AllowPrivateAccess=true))
	TArray<FQuestStep> QuestSteps;
};
