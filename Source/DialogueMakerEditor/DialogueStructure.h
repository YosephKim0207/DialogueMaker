#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "DialogueStructure.generated.h"

USTRUCT(BlueprintType)
struct FDialogueChoice
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText ResponseText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGuid NextDialogueId;
};

USTRUCT(BlueprintType)
struct FDialogueStructure : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGuid CurrentDialogueId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText SpeakerName;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText DialogueText;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FDialogueChoice> Choices;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGuid NextDialogue;
	//TODO 대화에서 이벤트 발생시 GameplayTag 활용하기
};