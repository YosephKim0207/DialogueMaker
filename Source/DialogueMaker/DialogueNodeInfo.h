#pragma once

#include "CoreMinimal.h"
#include "DialogueGraph.h"
#include "DialogueNodeInfoBase.h"
#include "QuestBase.h"
#include "Struct/DialogueStructure.h"
#include "DialogueNodeInfo.generated.h"

UCLASS(BlueprintType)
class DIALOGUEMAKER_API UDialogueNodeInfo : public UDialogueNodeInfoBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	const FText& GetTitle() const;
	
	UFUNCTION(BlueprintCallable)
	const FText& GetDialogueText() const;

	UFUNCTION(BlueprintCallable)
	const TArray<FDialogueChoice>& GetDialogueChoices() const;

	UFUNCTION(BlueprintCallable)
	void AddDialogueChoice(const FDialogueChoice& DialogueChoice);

	UFUNCTION(BlueprintCallable)
	void RemoveDialogueChoiceAt(int32 Index);
	
	bool IsDialogueAlreadyShown() const;
	void SetShownCondition(const bool NewCondition);
	
private:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	FText Title;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	ENPCID SpeakerID;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	FText DialogueText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TArray<FDialogueChoice> DialogueChoices;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	bool bIsShown;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UQuestBase> QuestToGive;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UQuestBase> QuestToClear;
};
