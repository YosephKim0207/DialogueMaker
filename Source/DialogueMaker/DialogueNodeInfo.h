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
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText Title;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	ENPCID SpeakerID;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText DialogueText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FDialogueChoice> DialogueResponses;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest")
	TSubclassOf<UQuestBase> QuestToGive;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest")
	TSubclassOf<UQuestBase> QuestToClear;
};
