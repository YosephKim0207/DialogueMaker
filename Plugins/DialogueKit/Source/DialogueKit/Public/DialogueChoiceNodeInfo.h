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
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Choice", meta = (ToolTip = "선택 문구나 배열 순서와 독립적인 고유 선택지 ID입니다. 선택 이력과 지역화 등에 사용하며 직접 수정하지 않습니다."))
	FGuid ChoiceId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Choice", meta = (ToolTip = "플레이어에게 보여줄 선택지 문구입니다. 다음 진행 경로는 이 선택지에 대응하는 출력 핀에 연결합니다."))
	FText Text;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Choice", meta = (ToolTip = "조건을 만족하는 일반 선택지가 없을 때 표시할 기본 선택지입니다. 노드당 최대 하나이며 이 항목의 Conditions는 사용하지 않습니다."))
	bool bDefault = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Choice", meta = (ToolTip = "이 선택지를 표시할 조건입니다. 불충족이면 숨깁니다. 빈 목록은 통과하며 기본 선택지에서는 평가하지 않습니다.", EditCondition = "!bDefault", EditConditionHides))
	FDialogueConditionSet Conditions;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Choice", meta = (ToolTip = "이 선택지를 골랐을 때 게임에 요청할 상태 변경 묶음입니다. 게임 효과 제공자가 실제 적용하며 효과가 있으면 저장 경계도 요청합니다."))
	TArray<FDialogueEffect> Effects;
};

UCLASS(BlueprintType)
class DIALOGUEKIT_API UDialogueChoiceNodeInfo : public UDialogueNodeInfoBase
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DialogueKit|Choice", meta = (ToolTip = "플레이어에게 제시할 선택지 목록입니다. 항목마다 문구·조건·효과와 대응 출력 핀을 가집니다. 기본 선택지는 최대 하나입니다."))
	TArray<FDialogueOption> Options;

	// 효과가 없는 선택도 명시적으로 저장 경계를 요청할 수 있다. 효과가 있으면 항상 요청한다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DialogueKit|Choice", meta = (ToolTip = "효과가 없는 선택에도 저장 경계를 요청합니다. 효과가 있으면 이 설정과 무관하게 요청합니다. 실제 디스크 저장 완료 여부는 게임 저장 조율자가 관리합니다."))
	bool bSaveOnSelection = false;

	// 명시적인 저작 변경에서만 새 ID를 발급한다. 런타임 조회는 ID를 수정하지 않는다.
	void EnsureOptionIds();
	// 기본 선택지 최대 하나, 고유 ID, 출력 수의 일치를 확인한다.
	bool HasValidOptions() const;
};
