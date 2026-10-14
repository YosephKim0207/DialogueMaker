#pragma once

#include "CoreMinimal.h"
#include "DialogueBlockRunner.h"
#include "DialoguePresentationProbe.generated.h"

// UMG와 같은 동적 delegate 바인딩 경로를 화면 없이 검증한다.
UCLASS()
class UDialoguePresentationProbe : public UObject
{
	GENERATED_BODY()
public:
	UPROPERTY()
	FDialoguePresentation LastPresentation;
	int32 Events = 0;
	UFUNCTION()
	void Receive(const FDialoguePresentation& Presentation)
	{
		LastPresentation = Presentation;
		++Events;
	}
};
