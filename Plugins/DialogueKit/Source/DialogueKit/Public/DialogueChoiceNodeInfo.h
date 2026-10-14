#pragma once

#include "CoreMinimal.h"
#include "DialogueNodeInfoBase.h"
#include "DialogueConditionTypes.h"
#include "DialogueEffectContract.h"
#include "DialogueChoiceNodeInfo.generated.h"

USTRUCT(BlueprintType)
struct DIALOGUEKIT_API FDialogueOption
{
	GENERATED_BODY()
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Choice")
	FGuid ChoiceId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Choice")
	FText Text;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Choice")
	bool bDefault = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Choice", meta = (EditCondition = "!bDefault", EditConditionHides))
	FDialogueConditionSet Conditions;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Choice")
	TArray<FDialogueEffect> Effects;
};

UCLASS(BlueprintType)
class DIALOGUEKIT_API UDialogueChoiceNodeInfo : public UDialogueNodeInfoBase
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DialogueKit|Choice")
	TArray<FDialogueOption> Options;

	// 효과가 없는 선택도 명시적으로 저장 경계를 요청할 수 있다. 효과가 있으면 항상 요청한다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DialogueKit|Choice")
	bool bSaveOnSelection = false;

	// 명시적인 저작 변경에서만 새 ID를 발급한다. 런타임 조회는 ID를 수정하지 않는다.
	void EnsureOptionIds();
	// 기본 선택지 최대 하나, 고유 ID, 출력 수의 일치를 확인한다.
	bool HasValidOptions() const;
};
