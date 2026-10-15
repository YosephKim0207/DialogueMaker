#pragma once
#include "CoreMinimal.h"
#include "DialogueSampleSaveCoordinator.h"
#include "DialogueCaptureFailureProbe.generated.h"

// 잘못된 큐 세대를 한 번 주입하여 최초 자료 보존과 재시도를 검증한다.
UCLASS()
class UDialogueCaptureFailureProbe : public UDialogueSampleSaveCoordinator
{
	GENERATED_BODY()
public:
	bool bFailNext = true;
	bool bReady = true;
	TFunction<void()> OnReady;
	virtual bool CanCaptureDialogueCheckpoint_Implementation() const override
	{
		if (OnReady) OnReady();
		return bReady && Super::CanCaptureDialogueCheckpoint_Implementation();
	}
	virtual FDialogueCheckpointAcceptance CaptureDialogueCheckpoint_Implementation(const FDialogueCheckpointSnapshot& Snapshot) override
	{
		if (bFailNext)
		{
			bFailNext = false;
			FDialogueCheckpointRequest Request = GetSaveQueue()->MakeRequest(Snapshot);
			Request.QueueGenerationId = FGuid::NewGuid();
			return RequestDialogueCheckpoint_Implementation(Request);
		}
		return Super::CaptureDialogueCheckpoint_Implementation(Snapshot);
	}
};
