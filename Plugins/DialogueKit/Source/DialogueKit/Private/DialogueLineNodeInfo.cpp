#include "DialogueLineNodeInfo.h"

// 마지막 수정한 모드만 활성화한다. 0은 즉시 출력 설정이다.
void UDialogueLineNodeInfo::SetTextTiming(bool bWholeLine, float Seconds)
{
	const float SafeSeconds = FMath::IsFinite(Seconds) ? FMath::Max(0.0f, Seconds) : 0.0f;
	SecondsPerCharacter = bWholeLine ? 0.0f : SafeSeconds;
	LineDuration = bWholeLine ? SafeSeconds : 0.0f;
}

// 새 발화 생성에서 표현 설정만 복사하며 이미지 에셋 자체를 복제하지 않는다.
void UDialogueLineNodeInfo::CopyContinuationDefaultsFrom(const UDialogueLineNodeInfo& Source)
{
	CopySpeakerPresentationFrom(Source);
	PortraitImage = Source.PortraitImage;
	PortraitOffset = Source.PortraitOffset;
	bMirrorPortrait = Source.bMirrorPortrait;
	SetTextTiming(Source.LineDuration > 0.0f, Source.LineDuration > 0.0f ? Source.LineDuration : Source.SecondsPerCharacter);
}

#if WITH_EDITOR
// Details에서 한 시간 값을 바꾸면 반대 모드를 비운다.
void UDialogueLineNodeInfo::PostEditChangeProperty(FPropertyChangedEvent& Event)
{
	if (Event.GetPropertyName() == GET_MEMBER_NAME_CHECKED(UDialogueLineNodeInfo, SecondsPerCharacter))
	{
		SetTextTiming(false, SecondsPerCharacter);
	}
	else if (Event.GetPropertyName() == GET_MEMBER_NAME_CHECKED(UDialogueLineNodeInfo, LineDuration))
	{
		SetTextTiming(true, LineDuration);
	}
	Super::PostEditChangeProperty(Event);
}
#endif
