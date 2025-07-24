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
	FGuid NextDialogueId;
	// TODO 대화가 이번에 끝나고 다음 이벤트 때 재개되는 경우 bool로 쓰던지 GameplayTag 활용하던지
	// TODO 대화에서 이벤트 발생시 GameplayTag 활용하기
};