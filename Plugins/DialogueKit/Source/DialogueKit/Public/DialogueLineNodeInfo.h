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
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DialogueKit|Line|Portrait")
	TSoftObjectPtr<UTexture2D> PortraitImage;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DialogueKit|Line|Portrait")
	FVector2D PortraitOffset = FVector2D::ZeroVector;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DialogueKit|Line|Portrait")
	bool bMirrorPortrait = false;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DialogueKit|Line|Text", meta = (ClampMin = "0", Units = "s"))
	float SecondsPerCharacter = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DialogueKit|Line|Text", meta = (ClampMin = "0", Units = "s"))
	float LineDuration = 0.0f;
	void SetTextTiming(bool bWholeLine, float Seconds);
	void CopyContinuationDefaultsFrom(const UDialogueLineNodeInfo& Source);
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& Event) override;
#endif
};
