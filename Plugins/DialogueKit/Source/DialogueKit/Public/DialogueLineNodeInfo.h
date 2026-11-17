#pragma once

#include "CoreMinimal.h"
#include "DialogueNodeInfo.h"
#include "DialogueLineNodeInfo.generated.h"

// 기존 화자/본문/초상화 읽기 API와 열람 버전을 재사용하는 단일 발화다.
UCLASS(BlueprintType, HideCategories = ("DialogueKit|DialogueNodeInfo|Quest", "DialogueKit|DialogueNodeInfo|State"))
class DIALOGUEKIT_API UDialogueLineNodeInfo : public UDialogueNodeInfo
{
	GENERATED_BODY()
public:
	// 새 Line은 내부 선택 배열을 편집하거나 진행 슬롯으로 사용하지 않는다.
	UDialogueLineNodeInfo() { bStandaloneLine = true; }
	// 비어 있으면 기존 화자/표정 기반 초상화 조회를 사용한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DialogueKit|Line|Portrait", meta = (ToolTip = "이 Line에 직접 표시할 초상화 이미지입니다. 비어 있으면 화자 ID와 표정으로 기존 초상화 자료를 찾습니다."))
	TSoftObjectPtr<UTexture2D> PortraitImage;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DialogueKit|Line|Portrait", meta = (ToolTip = "초상화의 기준 배치 위치에서 추가로 이동할 X/Y 오프셋입니다. 기본 대화 화면의 배치에 적용합니다."))
	FVector2D PortraitOffset = FVector2D::ZeroVector;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DialogueKit|Line|Portrait", meta = (ToolTip = "초상화를 좌우로 반전합니다. 원본 이미지 에셋은 변경하지 않습니다."))
	bool bMirrorPortrait = false;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DialogueKit|Line|Text", meta = (ToolTip = "글자 하나를 표시하는 데 걸리는 초 단위 시간입니다. 0이면 즉시 표시합니다. 이 값을 편집하면 Line Duration은 0으로 바뀝니다.", ClampMin = "0", Units = "s"))
	float SecondsPerCharacter = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DialogueKit|Line|Text", meta = (ToolTip = "본문 전체를 표시하는 데 걸리는 초 단위 시간입니다. 자동으로 다음 대사로 넘어가는 대기 시간이 아닙니다. 편집하면 Seconds Per Character는 0으로 바뀌며 0은 즉시 표시입니다.", ClampMin = "0", Units = "s"))
	float LineDuration = 0.0f;
	// 각 Line의 재개 연출을 명시한다. 비어 있는 BGM은 무음이며 같은 소스는 연속 재생한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DialogueKit|Line|Audio", meta = (ToolTip = "이 Line에서 재생할 배경음입니다. 비어 있으면 무음이며 직전 Line과 같은 음원은 이어서 재생합니다. 기본 대화 화면 기준입니다."))
	TSoftObjectPtr<class USoundBase> BackgroundMusic;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DialogueKit|Line|Audio", meta = (ToolTip = "이 Line을 표시할 때 재생할 효과음입니다. 비어 있으면 재생하지 않습니다. 기본 대화 화면 기준입니다."))
	TSoftObjectPtr<class USoundBase> SoundEffect;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DialogueKit|Line|Audio", meta = (ToolTip = "이 Line의 음성입니다. 비어 있으면 재생하지 않습니다. 출력 또는 음성 재생 중 진행 입력은 우선 전체 표시/스킵에 소비됩니다."))
	TSoftObjectPtr<class USoundBase> Voice;
	void SetTextTiming(bool bWholeLine, float Seconds);
	void CopyContinuationDefaultsFrom(const UDialogueLineNodeInfo& Source);
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& Event) override;
#endif
};
